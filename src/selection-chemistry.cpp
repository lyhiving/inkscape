#define __SP_SELECTION_CHEMISTRY_C__

/*
 * Miscellanous operations on selected items
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <config.h>

#include <string.h>
#include <gtk/gtk.h>
#include "svg/svg.h"
#include "xml/repr-private.h"
#include "document.h"
#include "inkscape.h"
#include "desktop.h"
#include "selection.h"
#include "desktop-handles.h"
#include "sp-item-transform.h" 
#include "sp-item-group.h"
#include "sp-path.h"
#include "helper/sp-intl.h"
#include "helper/sp-canvas.h"
#include "path-chemistry.h"
#include "desktop-affine.h"
#include "libnr/nr-matrix.h"
#include "libnr/nr-matrix-ops.h"
#include "style.h"
using NR::X;
using NR::Y;

#include "selection-chemistry.h"

/* fixme: find a better place */
GSList *clipboard = NULL;

static void sp_matrix_d_set_rotate (NRMatrix *m, double theta);

void sp_selection_delete()
{
	SPDesktop *desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;

	SPSelection *selection = SP_DT_SELECTION (desktop);

	// check if something is selected
	if (sp_selection_is_empty (selection)) {
		sp_view_set_statusf_flash (SP_VIEW (desktop), _("Nothing was deleted."));
		return;
	}

	GSList *selected = g_slist_copy ((GSList *) sp_selection_repr_list (selection));
	sp_selection_empty (selection);

	while (selected) {
		SPRepr *repr = (SPRepr *) selected->data;
		if (sp_repr_parent (repr)) sp_repr_unparent (repr);
		selected = g_slist_remove (selected, selected->data);
	}

	sp_document_done (SP_DT_DOCUMENT (desktop));
}

/* fixme: sequencing */
void sp_selection_duplicate()
{
	SPDesktop *desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;

	SPSelection *selection = SP_DT_SELECTION(desktop);

	// check if something is selected
	if (sp_selection_is_empty (selection)) {
		sp_view_set_statusf_flash (SP_VIEW (desktop), _("Select some objects to duplicate."));
		return;
	}

	GSList *reprs = g_slist_copy ((GSList *) sp_selection_repr_list (selection));

	sp_selection_empty (selection);

	SPRepr *parent = ((SPRepr *) reprs->data)->parent;
	gboolean sort = TRUE;
	for (GSList *i = reprs->next; i; i = i->next) {
		if ((((SPRepr *) i->data)->parent) != parent) {
			// We can duplicate items from different parents, but we cannot do sorting in this case
			sort = FALSE;
		}
	}

	if (sort)
		reprs = g_slist_sort (reprs, (GCompareFunc) sp_repr_compare_position);

	GSList *newsel = NULL;

	while (reprs) {
		parent = ((SPRepr *) reprs->data)->parent;
		SPRepr *copy = sp_repr_duplicate ((SPRepr *) reprs->data);

		sp_repr_append_child (parent, copy);

		//item = (SPItem *) sp_document_add_repr (SP_DT_DOCUMENT (desktop), copy);
		//g_assert (item != NULL);

		newsel = g_slist_prepend (newsel, copy);
		reprs = g_slist_remove (reprs, reprs->data);
		sp_repr_unref (copy);
	}

	sp_document_done (SP_DT_DOCUMENT (desktop));

	sp_selection_set_repr_list (SP_DT_SELECTION (desktop), newsel);

	g_slist_free (newsel);
}

void sp_edit_clear_all()
{
	SPDesktop *dt = SP_ACTIVE_DESKTOP;
	if (!dt) return;
	SPDocument *doc = SP_DT_DOCUMENT (dt);
	sp_selection_set_empty (SP_DT_SELECTION (dt));

	GSList *items = sp_item_group_item_list (SP_GROUP (sp_document_root (doc)));

	while (items) {
		sp_repr_unparent (SP_OBJECT_REPR (items->data));
		items = g_slist_remove (items, items->data);
	}

	sp_document_done (doc);
}

void sp_edit_select_all()
{
	SPDesktop *dt = SP_ACTIVE_DESKTOP;
	if (!dt) return;
	SPDocument *doc = SP_DT_DOCUMENT(dt);
	SPSelection *selection = SP_DT_SELECTION(dt);

	GSList *items = sp_item_group_item_list(SP_GROUP(sp_document_root(doc)));
	while (items) {
		SPRepr *repr = SP_OBJECT_REPR (items->data);
		if (!sp_selection_repr_selected (selection, repr))
			sp_selection_add_repr (selection, repr);
		items = g_slist_remove (items, items->data);
	}
	sp_document_done (doc);
}

static void
sp_group_cleanup (SPGroup *group)
{
	GSList *l = NULL;
	for (SPObject *child = group->children; child != NULL; child = child->next) {
		sp_object_ref (child, NULL);
		l = g_slist_prepend (l, child);
	}

	while (l) {
		if (SP_IS_GROUP (l->data)) {
			sp_group_cleanup (SP_GROUP (l->data));
		} else if (SP_IS_PATH (l->data)) {
			sp_path_cleanup (SP_PATH (l->data));
		}
		sp_object_unref (SP_OBJECT (l->data), NULL);
		l = g_slist_remove (l, l->data);
	}


	if (!strcmp (sp_repr_name (SP_OBJECT_REPR (group)), "g")) {
		gint numitems;
		numitems = 0;
		for (SPObject *child = group->children; child != NULL; child = child->next) {
			if (SP_IS_ITEM (child)) numitems += 1;
		}
		if (numitems <= 1) {
			sp_item_group_ungroup (group, NULL);
		}
	}
}

void sp_edit_cleanup(gpointer, gpointer)
{
	SPDocument *doc = SP_ACTIVE_DOCUMENT;
	if (!doc) return;
	if (SP_ACTIVE_DESKTOP) {
		sp_selection_empty (SP_DT_SELECTION (SP_ACTIVE_DESKTOP));
	}

	SPGroup *root = SP_GROUP (SP_DOCUMENT_ROOT (doc));

	sp_group_cleanup (root);

	sp_document_done (doc);
}

/* fixme: sequencing */

void sp_selection_group()
{
	SPDesktop *desktop = SP_ACTIVE_DESKTOP;

	if (desktop == NULL) return;

	SPSelection *selection = SP_DT_SELECTION(desktop);

	// check if something is selected
	if (sp_selection_is_empty (selection)) {
		sp_view_set_statusf_flash (SP_VIEW (desktop), _("Select two or more objects to group."));
		return;
	}

	GSList const *l = sp_selection_repr_list(selection);

	// check if at least two objects are selected
	if (l->next == NULL) {
		sp_view_set_statusf_flash (SP_VIEW (desktop), _("Select at least two objects to group."));
		return;
	}

	// check if all selected objects have common parent
	GSList *reprs = g_slist_copy((GSList *) sp_selection_repr_list(selection));
	SPRepr *parent = ((SPRepr *) reprs->data)->parent;
	for (GSList *i = reprs->next; i; i = i->next) {
		if ((((SPRepr *) i->data)->parent) != parent) {
			sp_view_set_statusf_error (SP_VIEW (desktop), _("You cannot group objects from different groups or layers."));
			return;
		}
	}

	GSList *p = g_slist_copy((GSList *) l);

	sp_selection_empty(SP_DT_SELECTION(desktop));

	p = g_slist_sort (p, (GCompareFunc) sp_repr_compare_position);

	SPRepr *group = sp_repr_new("g");

	while (p) {
		SPRepr *spnew;
		SPRepr *current = (SPRepr *) p->data;
		spnew = sp_repr_duplicate (current);
		sp_repr_unparent (current);
		sp_repr_append_child (group, spnew);
		sp_repr_unref (spnew);
		p = g_slist_remove (p, current);
	}

	// add the new group to the group members' common parent
	sp_repr_append_child (parent, group);
	sp_document_done (SP_DT_DOCUMENT (desktop));

	sp_selection_set_repr (selection, group);
	sp_repr_unref (group);
}

void sp_selection_ungroup()
{
	SPDesktop *desktop = SP_ACTIVE_DESKTOP;
	if (!desktop) return;

	if (sp_selection_is_empty(SP_DT_SELECTION(desktop))) {
		sp_view_set_statusf_flash (SP_VIEW (desktop), _("Select a group to ungroup."));
		return;
	}

	// get a copy of current selection
	GSList *new_select = NULL;
	bool ungrouped = false;
	for (GSList *items = g_slist_copy((GSList *) sp_selection_item_list(SP_DT_SELECTION(desktop)));
	     items != NULL;
	     items = items->next)
	{
		SPItem *group = (SPItem *) items->data;

		/* We do not allow ungrouping <svg> etc. (lauris) */
		if (strcmp (sp_repr_name (SP_OBJECT_REPR (group)), "g")) {
			// keep the non-group item in the new selection
			new_select = g_slist_prepend (new_select, group);
			continue;
		}

		GSList *children = NULL;
		/* This is not strictly required, but is nicer to rely on group ::destroy (lauris) */
		sp_item_group_ungroup (SP_GROUP (group), &children);
		ungrouped = true;
		// Add ungrouped items to the new selection.
		new_select = g_slist_concat (new_select, children);
	}

	if (new_select) { // set new selection
		sp_selection_empty(SP_DT_SELECTION(desktop));
		sp_selection_set_item_list(SP_DT_SELECTION(desktop), new_select);
		g_slist_free(new_select);
	}
	if (!ungrouped) {
		sp_view_set_statusf_flash (SP_VIEW (desktop), _("No groups to ungroup in the selection."));
	}
}

static SPGroup *
sp_item_list_common_parent_group (const GSList *items)
{
	if (!items) return NULL;
	SPObject *parent = SP_OBJECT_PARENT (items->data); 
	/* Strictly speaking this CAN happen, if user selects <svg> from XML editor */
	if (!SP_IS_GROUP (parent)) return NULL;
	for (items = items->next; items; items = items->next) {
		if (SP_OBJECT_PARENT (items->data) != parent) return NULL;
	}

	return SP_GROUP (parent);
}

void sp_selection_raise()
{
	SPDesktop *dt = SP_ACTIVE_DESKTOP;
	if (!dt) return;
	GSList const *items = sp_selection_item_list(SP_DT_SELECTION(dt));
	if (!items) return;
	SPGroup const *group = sp_item_list_common_parent_group(items);
	if (!group) return;
	SPRepr *grepr = SP_OBJECT_REPR(group);

	/* construct reverse-ordered list of selected children */
	GSList *rev = NULL;
	SPObject *child;
	for (child = group->children; child; child = child->next) {
		if (g_slist_find ((GSList *) items, child)) {
			rev = g_slist_prepend (rev, child);
		}
	}

	while (rev) {
		child = SP_OBJECT (rev->data);
		for (SPObject *newref = child->next; newref; newref = newref->next) {
			if (SP_IS_ITEM (newref)) {
				if (!g_slist_find ((GSList *) items, newref)) {
					/* Found available position */
					sp_repr_change_order (grepr, SP_OBJECT_REPR (child), SP_OBJECT_REPR (newref));
				}
				break;
			}
		}
		rev = g_slist_remove (rev, child);
	}

	sp_document_done (SP_DT_DOCUMENT (dt));
}

void sp_selection_raise_to_top()
{
	SPDesktop *desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;
	SPDocument *document = SP_DT_DOCUMENT(SP_ACTIVE_DESKTOP);
	SPSelection *selection = SP_DT_SELECTION(SP_ACTIVE_DESKTOP);

	if (sp_selection_is_empty (selection)) return;

	GSList *rl = g_slist_copy((GSList *) sp_selection_repr_list(selection));

	for (GSList *l = rl; l != NULL; l = l->next) {
		SPRepr *repr = (SPRepr *) l->data;
		sp_repr_set_position_absolute (repr, -1);
	}

	g_slist_free (rl);

	sp_document_done (document);
}

void sp_selection_lower()
{
	SPDesktop *dt = SP_ACTIVE_DESKTOP;
	if (!dt) return;
	GSList const *items = sp_selection_item_list(SP_DT_SELECTION(dt));
	if (!items) return;
	SPGroup *group = sp_item_list_common_parent_group(items);
	if (!group) return;
	SPRepr *grepr = SP_OBJECT_REPR(group);

	/* Start from beginning */
	bool skip = true;
	SPObject *newref = NULL;
	SPObject *oldref = NULL;
	SPObject *child = group->children;
	while (child != NULL) {
		if (SP_IS_ITEM (child)) {
			/* We are item */
			skip = false;
			/* fixme: Remove from list (Lauris) */
			if (g_slist_find ((GSList *) items, child)) {
				/* Need lower */
				if (newref != oldref) {
					if (sp_repr_change_order (grepr, SP_OBJECT_REPR (child), (newref) ? SP_OBJECT_REPR (newref) : NULL)) {
						/* Order change succeeded */
						/* Next available position */
						newref = child;
						/* Oldref is just what it was */
						/* Continue from oldref */
						child = oldref->next;
					} else {
						/* Order change did not succeed */
						newref = oldref;
						oldref = child;
						child = child->next;
					}
				} else {
					/* Item position will not change */
					/* Other items will lower only following positions */
					newref = child;
					oldref = child;
					child = child->next;
				}
			} else {
				/* We were item, but not in list */
				newref = oldref;
				oldref = child;
				child = child->next;
			}
		} else {
			/* We want to refind newref only to skip initial non-items */
			if (skip) newref = child;
			oldref = child;
			child = child->next;
		}
	}

	sp_document_done (SP_DT_DOCUMENT (dt));
}

void sp_selection_lower_to_bottom()
{
	SPDesktop *desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;
	SPDocument *document = SP_DT_DOCUMENT(SP_ACTIVE_DESKTOP);
	SPSelection *selection = SP_DT_SELECTION(SP_ACTIVE_DESKTOP);

	if (sp_selection_is_empty (selection)) return;

	GSList *rl;
	rl = g_slist_copy((GSList *) sp_selection_repr_list(selection));
	rl = g_slist_reverse(rl);

	for (GSList *l = rl; l != NULL; l = l->next) {
		gint minpos;
		SPObject *pp, *pc;
		SPRepr *repr = (SPRepr *) l->data;
		pp = sp_document_lookup_id (document, sp_repr_attr (sp_repr_parent (repr), "id"));
		minpos = 0;
		g_assert (SP_IS_GROUP (pp));
		pc = SP_GROUP (pp)->children;
		while (!SP_IS_ITEM (pc)) {
			minpos += 1;
			pc = pc->next;
		}
		sp_repr_set_position_absolute (repr, minpos);
	}

	g_slist_free (rl);

	sp_document_done (document);
}

void
sp_undo (SPDesktop *desktop, SPDocument *doc)
{
	if (SP_IS_DESKTOP(desktop)) {
		if (!sp_document_undo (SP_DT_DOCUMENT (desktop)))
			sp_view_set_statusf_flash (SP_VIEW (desktop), _("Nothing to undo."));
	}
}

void
sp_redo (SPDesktop *desktop, SPDocument *doc)
{
	if (SP_IS_DESKTOP(desktop)) {
		if (!sp_document_redo (SP_DT_DOCUMENT (desktop)))
			sp_view_set_statusf_flash (SP_VIEW (desktop), _("Nothing to redo."));
	}
}

void sp_selection_cut()
{
	sp_selection_copy();
	sp_selection_delete();
}

void sp_selection_copy()
{
	SPDesktop *desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;

	SPSelection *selection = SP_DT_SELECTION(desktop);

	// check if something is selected
	if (sp_selection_is_empty (selection)) {
		sp_view_set_statusf_flash (SP_VIEW (desktop), _("Nothing was copied."));
		return;
	}

	GSList *sl;
	sl = g_slist_copy ((GSList *) sp_selection_repr_list (selection));
	sl = g_slist_sort (sl, (GCompareFunc) sp_repr_compare_position);

	/* Clear old clipboard */
	while (clipboard) {
		sp_repr_unref ((SPRepr *) clipboard->data);
		clipboard = g_slist_remove (clipboard, clipboard->data);
	}

	while (sl != NULL) {
		SPRepr *repr = (SPRepr *) sl->data;
		sl = g_slist_remove (sl, repr);
		SPCSSAttr *css = sp_repr_css_attr_inherited(repr, "style");
		SPRepr *copy = sp_repr_duplicate(repr);
		sp_repr_css_set (copy, css, "style");
		sp_repr_css_attr_unref (css);

		clipboard = g_slist_prepend (clipboard, copy);
	}

	clipboard = g_slist_reverse (clipboard);
}

void sp_selection_paste(bool in_place)
{
	SPDesktop *desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;
	g_assert (SP_IS_DESKTOP (desktop));

	SPSelection *selection = SP_DT_SELECTION(desktop);
	g_assert (selection != NULL);
	g_assert (SP_IS_SELECTION (selection));

	// check if something is in the clipboard
	if (clipboard == NULL) {
		sp_view_set_statusf_flash (SP_VIEW (desktop), _("Nothing in the clipboard."));
		return;
	}

	sp_selection_empty (selection);

	for (GSList *l = clipboard; l != NULL; l = l->next) {
		SPRepr *repr = (SPRepr *) l->data;
		SPRepr *copy = sp_repr_duplicate(repr);
		sp_document_add_repr (SP_DT_DOCUMENT (desktop), copy);
		sp_selection_add_repr (selection, copy);
		sp_repr_unref (copy);
	}

	if (!in_place) {
		sp_document_ensure_up_to_date (SP_DT_DOCUMENT (desktop));

		NRRect bbox;
		sp_selection_bbox (selection, &bbox);

		NR::Point m = sp_desktop_point (desktop);

		sp_selection_move_relative (selection, m[NR::X] - (bbox.x0 + bbox.x1)*0.5, m[NR::Y] - (bbox.y0 + bbox.y1)*0.5);
	}

	sp_document_done (SP_DT_DOCUMENT (desktop));
}

void sp_selection_paste_style()
{
	SPDesktop *desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;
	g_assert (SP_IS_DESKTOP (desktop));

	SPSelection *selection = SP_DT_SELECTION(desktop);
	g_assert (selection != NULL);
	g_assert (SP_IS_SELECTION (selection));

	// check if something is in the clipboard
	if (clipboard == NULL) {
		sp_view_set_statusf_flash (SP_VIEW (desktop), _("Nothing in the clipboard."));
		return;
	}

	// check if something is selected
	if (sp_selection_is_empty (selection)) {
		sp_view_set_statusf_flash (SP_VIEW (desktop), _("Select objects to paste style to."));
		return;
	}

	GSList *selected = g_slist_copy ((GSList *) sp_selection_item_list (selection));

	for (GSList *l = selected; l != NULL; l = l->next) {

		// take the style from first object on clipboard
		SPStyle *style = sp_style_new ();
		sp_style_read_from_repr (style, (SPRepr *) clipboard->data);

		// merge it with the current object's style
		sp_style_merge_from_style_string (style, sp_repr_attr (SP_OBJECT_REPR (l->data), "style"));

		// calculate the difference between the current object and its parent styles
		gchar *newcss = sp_style_write_difference (style, SP_OBJECT_STYLE (SP_OBJECT_PARENT (l->data)));

		// write the result to the object repr
		sp_repr_set_attr (SP_OBJECT_REPR(l->data), "style", (newcss && *newcss) ? newcss : NULL);
	}

	sp_document_done (SP_DT_DOCUMENT (desktop));
}

void sp_selection_apply_affine(SPSelection *selection, NR::Matrix const &affine)
{
	g_assert (SP_IS_SELECTION (selection));

	for (GSList *l = selection->items; l != NULL; l = l-> next) {
		SPItem *item = SP_ITEM(l->data);
		sp_item_set_i2d_affine(item, sp_item_i2d_affine(item) * affine);
		/* update repr -  needed for undo */
		sp_item_write_transform (item, SP_OBJECT_REPR (item), &item->transform);
		/* fixme: Check, whether anything changed */
		sp_object_read_attr (SP_OBJECT (item), "transform");
	}
}

void sp_selection_remove_transform()
{
	SPDesktop *desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;
	SPSelection *selection = SP_DT_SELECTION(desktop);
	if (!SP_IS_SELECTION (selection)) return;

	GSList const *l = sp_selection_repr_list (selection);
	while (l != NULL) {
		sp_repr_set_attr ((SPRepr*)l->data,"transform", NULL);
		l = l->next;
	}

	sp_document_done (SP_DT_DOCUMENT (desktop));
}

void
sp_selection_scale_absolute (SPSelection *selection, double x0, double x1, double y0, double y1)
{
	NRRect bbox;
	NRMatrix p2o, o2n, scale, final, s;
	double dx, dy, nx, ny;

	g_assert (SP_IS_SELECTION (selection));

	sp_selection_bbox (selection, &bbox);

	nr_matrix_set_translate (&p2o, -bbox.x0, -bbox.y0);

	dx = (x1-x0) / (bbox.x1 - bbox.x0);
	dy = (y1-y0) / (bbox.y1 - bbox.y0);
	nr_matrix_set_scale (&scale, dx, dy);

	nx = x0;
	ny = y0;
	nr_matrix_set_translate (&o2n, nx, ny);

	nr_matrix_multiply (&s, &p2o, &scale);
	nr_matrix_multiply (&final, &s, &o2n);

	sp_selection_apply_affine(selection, final);
}


void sp_selection_scale_relative(SPSelection *selection, NR::Point const &align, NR::scale const &scale)
{
	NR::translate const n2d(-align);
	NR::translate const d2n(align);
	NR::Matrix const final( n2d * scale * d2n );
	sp_selection_apply_affine(selection, final);
}

void
sp_selection_rotate_relative (SPSelection *selection, NR::Point const &center, gdouble angle_degrees)
{
	NRMatrix rotate, n2d, d2n, final, s;

	nr_matrix_set_translate (&n2d, -center[NR::X], -center[NR::Y]);
	nr_matrix_invert (&d2n, &n2d);
	sp_matrix_d_set_rotate (&rotate, angle_degrees);

	nr_matrix_multiply (&s, &n2d, &rotate);
	nr_matrix_multiply (&final, &s, &d2n);

	sp_selection_apply_affine(selection, final);
}

void
sp_selection_skew_relative (SPSelection *selection, NR::Point const &align, double dx, double dy)
{
	NRMatrix skew, n2d, d2n, final, s;

	nr_matrix_set_translate (&n2d, -align[NR::X], -align[NR::Y]);
	nr_matrix_invert (&d2n, &n2d);

	skew.c[0] = 1;
	skew.c[1] = dy;
	skew.c[2] = dx;
	skew.c[3] = 1;
	skew.c[4] = 0;
	skew.c[5] = 0;

	nr_matrix_multiply (&s, &n2d, &skew);
	nr_matrix_multiply (&final, &s, &d2n);

	sp_selection_apply_affine(selection, final);
}

void sp_selection_move_relative(SPSelection *selection, double dx, double dy)
{
	sp_selection_apply_affine(selection, NR::Matrix(NR::translate(dx, dy)));
}

void sp_selection_rotate_90()
{
	SPDesktop *desktop = SP_ACTIVE_DESKTOP;
	if (!SP_IS_DESKTOP(desktop)) return;
	SPSelection *selection = SP_DT_SELECTION(desktop);
	if sp_selection_is_empty(selection) return;
	GSList *l = selection->items;
	NR::rotate const rot_neg_90(NR::Point(0, -1));
	for (GSList *l2 = l ; l2 != NULL ; l2 = l2->next) {
		SPItem *item = SP_ITEM(l2->data);
		sp_item_rotate_rel(item, rot_neg_90);
	}

	sp_document_done (SP_DT_DOCUMENT (desktop));
}

void
sp_selection_rotate (SPSelection *selection, gdouble angle_degrees)
{
	NRRect bbox;
	sp_selection_bbox (selection, &bbox);
	NR::Point center = NR::Rect(bbox).midpoint();

	sp_selection_rotate_relative (selection, center, angle_degrees);

	if ( angle_degrees > 0 )
		sp_document_maybe_done (SP_DT_DOCUMENT (selection->desktop), "selector:rotate:ccw");
	else
		sp_document_maybe_done (SP_DT_DOCUMENT (selection->desktop), "selector:rotate:cw");
}

/**
\param  angle   the angle in "angular pixels", i.e. how many visible pixels must move the outermost point of the rotated object
*/
void
sp_selection_rotate_screen (SPSelection *selection, gdouble angle)
{
	gdouble zoom, zmove, zangle, r;

	NRRect bbox_compat;
	sp_selection_bbox (selection, &bbox_compat);
	NR::Rect bbox(bbox_compat);
	NR::Point center = bbox.midpoint();

	zoom = SP_DESKTOP_ZOOM (selection->desktop);
	zmove = angle / zoom;
	r = NR::L2(bbox.max() - center);

	zangle = 180 * atan2 (zmove, r) / M_PI;

	sp_selection_rotate_relative (selection, center, zangle);

	if (angle > 0)
		sp_document_maybe_done (SP_DT_DOCUMENT (selection->desktop), "selector:rotate:ccw");
	else
		sp_document_maybe_done (SP_DT_DOCUMENT (selection->desktop), "selector:rotate:cw");

}

void
sp_selection_scale (SPSelection *selection, gdouble grow)
{
	NR::Rect const bbox(sp_selection_bbox(selection));
	NR::Point const center(bbox.midpoint());
	double const max_len = bbox.maxExtent();

	if ( max_len + 2 * grow <= 0 ) {
		return;
	}

	double const times = 1.0 + grow / max_len;
	sp_selection_scale_relative(selection, center, NR::scale(times, times));

	sp_document_maybe_done(SP_DT_DOCUMENT(selection->desktop),
			       ( (grow > 0)
				 ? "selector:scale:larger"
				 : "selector:scale:smaller" ));
}

void
sp_selection_scale_screen (SPSelection *selection, gdouble grow_pixels)
{
	sp_selection_scale(selection,
			   grow_pixels / SP_DESKTOP_ZOOM(selection->desktop));
}

void
sp_selection_scale_times (SPSelection *selection, gdouble times)
{
	NR::Point const center(sp_selection_bbox(selection).midpoint());
	sp_selection_scale_relative(selection, center, NR::scale(times, times));
	sp_document_done(SP_DT_DOCUMENT(selection->desktop));
}

void
sp_selection_move (gdouble dx, gdouble dy)
{
	SPDesktop *desktop = SP_ACTIVE_DESKTOP;
	g_return_if_fail(SP_IS_DESKTOP (desktop));
	SPSelection *selection = SP_DT_SELECTION(desktop);
	if (!SP_IS_SELECTION(selection)) {
		return;
	}
	if (sp_selection_is_empty(selection)) {
		return;
	}

	sp_selection_move_relative (selection, dx, dy);

	if (dx == 0) {
		sp_document_maybe_done (SP_DT_DOCUMENT (desktop), "selector:move:vertical");
	} else if (dy == 0) {
		sp_document_maybe_done (SP_DT_DOCUMENT (desktop), "selector:move:horizontal");
	} else {
		sp_document_done (SP_DT_DOCUMENT (desktop));
	}
}

void
sp_selection_move_screen (gdouble dx, gdouble dy)
{
	SPDesktop *desktop;
	SPSelection *selection;
	gdouble zdx, zdy, zoom;

	desktop = SP_ACTIVE_DESKTOP;
	g_return_if_fail(SP_IS_DESKTOP (desktop));
	selection = SP_DT_SELECTION (desktop);
	if (!SP_IS_SELECTION(selection)) {
		return;
	}
	if (sp_selection_is_empty(selection)) {
		return;
	}

	// same as sp_selection_move but divide deltas by zoom factor
	zoom = SP_DESKTOP_ZOOM (desktop);
	zdx = dx / zoom;
	zdy = dy / zoom;
	sp_selection_move_relative (selection, zdx, zdy);

	if (dx == 0) {
		sp_document_maybe_done (SP_DT_DOCUMENT (desktop), "selector:move:vertical");
	} else if (dy == 0) {
		sp_document_maybe_done (SP_DT_DOCUMENT (desktop), "selector:move:horizontal");
	} else {
		sp_document_done (SP_DT_DOCUMENT (desktop));
	}
}

static void scroll_to_show_item(SPDesktop *desktop, SPItem *item);

void
sp_selection_item_next (void)
{
	SPDocument *document = SP_ACTIVE_DOCUMENT;
	SPDesktop *desktop = SP_ACTIVE_DESKTOP;
	g_return_if_fail(document != NULL);
	g_return_if_fail(desktop != NULL);
	if (!SP_IS_DESKTOP(desktop)) {
		return;
	}
	SPSelection *selection = SP_DT_SELECTION(desktop);
	g_return_if_fail( selection != NULL );

	// get item list
	GSList *children = NULL;
	if (SP_CYCLING == SP_CYCLE_VISIBLE) {
		NRRect dbox;
		sp_desktop_get_display_area (desktop, &dbox);
		children = sp_document_items_in_box(document, &dbox);
	} else {
		children = sp_item_group_item_list (SP_GROUP(sp_document_root(document)));
	}

	// compute next item
	if (children == NULL) {
		return;
	}
	SPItem *item = NULL;
	if (sp_selection_is_empty(selection)) {
		item = SP_ITEM(children->data);
	} else {
		GSList *l = g_slist_find(children, selection->items->data);
		if ( ( l == NULL ) || ( l->next == NULL ) ) {
			item = SP_ITEM(children->data);
		} else {
			item = SP_ITEM(l->next->data);
		}
	}

	// set selection to item
	if (item != NULL) {
		sp_selection_set_item(selection, item);
	} else {
		return;
	}

	g_slist_free (children);

	// adjust visible area to see whole new selection
	if (SP_CYCLING == SP_CYCLE_FOCUS) {
		scroll_to_show_item(desktop, item);
	}
}

/* TODO: Much copy & paste code here; see if can merge with sp_selection_item_next. */
void
sp_selection_item_prev (void)
{
	SPDocument *document = SP_ACTIVE_DOCUMENT;
	SPDesktop *desktop = SP_ACTIVE_DESKTOP;
	g_return_if_fail(document != NULL);
	g_return_if_fail(desktop != NULL);
	if (!SP_IS_DESKTOP(desktop)) {
		return;
	}
	SPSelection *selection = SP_DT_SELECTION(desktop);
	g_return_if_fail( selection != NULL );

	// get item list
	GSList *children = NULL;
	if (SP_CYCLING == SP_CYCLE_VISIBLE) {
		NRRect dbox;
		sp_desktop_get_display_area (desktop, &dbox);
		children = sp_document_items_in_box(document, &dbox);
	} else {
		children = sp_item_group_item_list (SP_GROUP(sp_document_root(document)));
	}

	// compute prev item
	if (children == NULL) {
		return;
	}
	SPItem *item = NULL;
	if (sp_selection_is_empty(selection)) {
		item = SP_ITEM(g_slist_last(children)->data);
	} else {
		GSList *l = children;
		while ((l->next != NULL) && (l->next->data != selection->items->data)) {
			l = l->next;
		}
		item = SP_ITEM (l->data);
	}

	// set selection to item
	if (item != NULL) {
		sp_selection_set_item(selection, item);
	} else {
		return;
	}

	g_slist_free (children);

	// adjust visible area to see whole new selection
	if (SP_CYCLING == SP_CYCLE_FOCUS) {
		scroll_to_show_item(desktop, item);
	}
}

/**
 * If \a item is not entirely visible then adjust visible area to centre on the centre on of
 * \a item.
 */
static void scroll_to_show_item(SPDesktop *desktop, SPItem *item)
{
	NRRect dbox;
	sp_desktop_get_display_area (desktop, &dbox);
	NRRect sbox;
	sp_item_bbox_desktop (item, &sbox);
	if ( dbox.x0 > sbox.x0  ||
	     dbox.y0 > sbox.y0  ||
	     dbox.x1 < sbox.x1  ||
	     dbox.y1 < sbox.y1 )
	{
		NR::Point const s_dt(( sbox.x0 + sbox.x1 ) / 2,
				     ( sbox.y0 + sbox.y1 ) / 2);
		NR::Point const s_w( s_dt * desktop->d2w );
		NR::Point const d_dt(( dbox.x0 + dbox.x1 ) / 2,
				     ( dbox.y0 + dbox.y1 ) / 2);
		NR::Point const d_w( d_dt * desktop->d2w );
		NR::Point const moved_w( d_w - s_w );
		gint const dx = (gint) moved_w[X];
		gint const dy = (gint) moved_w[Y];
		sp_desktop_scroll_world(desktop, dx, dy);
	}
}


static void
sp_matrix_d_set_rotate (NRMatrix *m, double theta_degrees)
{
	double s, c;
	s = sin(theta_degrees / 180.0 * M_PI);
	c = cos(theta_degrees / 180.0 * M_PI);
	m->c[0] = c;
	m->c[1] = s;
	m->c[2] = -s;
	m->c[3] = c;
	m->c[4] = 0.0;
	m->c[5] = 0.0;
}

