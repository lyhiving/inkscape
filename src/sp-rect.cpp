#define __SP_RECT_C__

/*
 * SVG <rect> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <config.h>

#include <math.h>
#include <string.h>

#include <libnr/nr-matrix.h>
#include <libnr/nr-matrix-ops.h>

#include "svg/svg.h"
#include "attributes.h"
#include "style.h"
#include "document.h"
#include "dialogs/object-attributes.h"
#include "sp-root.h"
#include "sp-rect.h"
#include "helper/sp-intl.h"

#define noRECT_VERBOSE

static void sp_rect_class_init (SPRectClass *klass);
static void sp_rect_init (SPRect *rect);

static void sp_rect_build (SPObject *object, SPDocument *document, SPRepr *repr);
static void sp_rect_set (SPObject *object, unsigned int key, const gchar *value);
static void sp_rect_update (SPObject *object, SPCtx *ctx, guint flags);
static SPRepr *sp_rect_write (SPObject *object, SPRepr *repr, guint flags);

static gchar * sp_rect_description (SPItem * item);
static int sp_rect_snappoints(SPItem *item, NR::Point p[], int size);
static NR::Matrix sp_rect_set_transform (SPItem *item, NR::Matrix const &xform);

static void sp_rect_set_shape (SPShape *shape);

static SPShapeClass *parent_class;

GType
sp_rect_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo info = {
			sizeof (SPRectClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) sp_rect_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SPRect),
			16,	/* n_preallocs */
			(GInstanceInitFunc) sp_rect_init,
			NULL,	/* value_table */
		};
		type = g_type_register_static (SP_TYPE_SHAPE, "SPRect", &info, (GTypeFlags)0);
	}
	return type;
}

static void
sp_rect_class_init (SPRectClass *klass)
{
	GObjectClass * object_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;
	SPShapeClass * shape_class;

	object_class = (GObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;
	item_class = (SPItemClass *) klass;
	shape_class = (SPShapeClass *) klass;

	parent_class = (SPShapeClass *)g_type_class_ref (SP_TYPE_SHAPE);

	sp_object_class->build = sp_rect_build;
	sp_object_class->write = sp_rect_write;
	sp_object_class->set = sp_rect_set;
	sp_object_class->update = sp_rect_update;

	item_class->description = sp_rect_description;
	item_class->snappoints = sp_rect_snappoints;
	item_class->set_transform = sp_rect_set_transform;

	shape_class->set_shape = sp_rect_set_shape;
}

static void
sp_rect_init (SPRect * rect)
{
	/* Initializing to zero is automatic */
	/* sp_svg_length_unset (&rect->x, SP_SVG_UNIT_NONE, 0.0, 0.0); */
	/* sp_svg_length_unset (&rect->y, SP_SVG_UNIT_NONE, 0.0, 0.0); */
	/* sp_svg_length_unset (&rect->width, SP_SVG_UNIT_NONE, 0.0, 0.0); */
	/* sp_svg_length_unset (&rect->height, SP_SVG_UNIT_NONE, 0.0, 0.0); */
	/* sp_svg_length_unset (&rect->rx, SP_SVG_UNIT_NONE, 0.0, 0.0); */
	/* sp_svg_length_unset (&rect->ry, SP_SVG_UNIT_NONE, 0.0, 0.0); */
}

static void
sp_rect_build (SPObject *object, SPDocument *document, SPRepr *repr)
{
	SPRect *rect;
	SPVersion version;

	rect = SP_RECT (object);

	if (((SPObjectClass *) parent_class)->build)
		((SPObjectClass *) parent_class)->build (object, document, repr);

	sp_object_read_attr (object, "x");
	sp_object_read_attr (object, "y");
	sp_object_read_attr (object, "width");
	sp_object_read_attr (object, "height");
	sp_object_read_attr (object, "rx");
	sp_object_read_attr (object, "ry");

	version = sp_object_get_sodipodi_version (object);

	if ( version.major == 0 && version.minor == 29 ) {
		if (rect->rx.set && rect->ry.set) {
			/* 0.29 treated 0.0 radius as missing value */
			if ((rect->rx.value != 0.0) && (rect->ry.value == 0.0)) {
				sp_repr_set_attr (repr, "ry", NULL);
				sp_object_read_attr (object, "ry");
			} else if ((rect->ry.value != 0.0) && (rect->rx.value == 0.0)) {
				sp_repr_set_attr (repr, "rx", NULL);
				sp_object_read_attr (object, "rx");
			}
		}
	}
}

static void
sp_rect_set (SPObject *object, unsigned int key, const gchar *value)
{
	SPRect *rect;

	rect = SP_RECT (object);

	/* fixme: We need real error processing some time */

	switch (key) {
	case SP_ATTR_X:
		if (!sp_svg_length_read (value, &rect->x)) {
			sp_svg_length_unset (&rect->x, SP_SVG_UNIT_NONE, 0.0, 0.0);
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG);
		break;
	case SP_ATTR_Y:
		if (!sp_svg_length_read (value, &rect->y)) {
			sp_svg_length_unset (&rect->y, SP_SVG_UNIT_NONE, 0.0, 0.0);
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG);
		break;
	case SP_ATTR_WIDTH:
		if (!sp_svg_length_read (value, &rect->width) || (rect->width.value < 0.0)) {
			sp_svg_length_unset (&rect->width, SP_SVG_UNIT_NONE, 0.0, 0.0);
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG);
		break;
	case SP_ATTR_HEIGHT:
		if (!sp_svg_length_read (value, &rect->height) || (rect->width.value < 0.0)) {
			sp_svg_length_unset (&rect->height, SP_SVG_UNIT_NONE, 0.0, 0.0);
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG);
		break;
	case SP_ATTR_RX:
		if (!sp_svg_length_read (value, &rect->rx) || (rect->rx.value < 0.0)) {
			sp_svg_length_unset (&rect->rx, SP_SVG_UNIT_NONE, 0.0, 0.0);
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG);
		break;
	case SP_ATTR_RY:
		if (!sp_svg_length_read (value, &rect->ry) || (rect->ry.value < 0.0)) {
			sp_svg_length_unset (&rect->ry, SP_SVG_UNIT_NONE, 0.0, 0.0);
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG);
		break;
	default:
		if (((SPObjectClass *) parent_class)->set)
			((SPObjectClass *) parent_class)->set (object, key, value);
		break;
	}
}

static void
sp_rect_update (SPObject *object, SPCtx *ctx, guint flags)
{
	if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {
		SPRect *rect;
		SPStyle *style;
		SPItemCtx *ictx;
		double d, w, h;
		rect = (SPRect *) object;
		style = object->style;
		ictx = (SPItemCtx *) ctx;
		d = 1.0 / NR_MATRIX_DF_EXPANSION (&ictx->i2vp);
		w = d * (ictx->vp.x1 - ictx->vp.x0);
		h = d * (ictx->vp.y1 - ictx->vp.y0);
		sp_svg_length_update (&rect->x, style->font_size.computed, style->font_size.computed * 0.5, w);
		sp_svg_length_update (&rect->y, style->font_size.computed, style->font_size.computed * 0.5, h);
		sp_svg_length_update (&rect->width, style->font_size.computed, style->font_size.computed * 0.5, w);
		sp_svg_length_update (&rect->height, style->font_size.computed, style->font_size.computed * 0.5, h);
		sp_svg_length_update (&rect->rx, style->font_size.computed, style->font_size.computed * 0.5, w);
		sp_svg_length_update (&rect->ry, style->font_size.computed, style->font_size.computed * 0.5, h);
		sp_shape_set_shape ((SPShape *) object);
		flags &= ~SP_OBJECT_USER_MODIFIED_FLAG_B; // since we change the description, it's not a "just translation" anymore
	}

	if (((SPObjectClass *) parent_class)->update)
		((SPObjectClass *) parent_class)->update (object, ctx, flags);
}

static SPRepr *
sp_rect_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPRect *rect;

	rect = SP_RECT (object);

	if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
		repr = sp_repr_new ("rect");
	}

	sp_repr_set_double (repr, "width", rect->width.computed);
	sp_repr_set_double (repr, "height", rect->height.computed);
	if (rect->rx.set) sp_repr_set_double (repr, "rx", rect->rx.computed);
	if (rect->ry.set) sp_repr_set_double (repr, "ry", rect->ry.computed);
	sp_repr_set_double (repr, "x", rect->x.computed);
	sp_repr_set_double (repr, "y", rect->y.computed);

	if (((SPObjectClass *) parent_class)->write)
		((SPObjectClass *) parent_class)->write (object, repr, flags);

	return repr;
}

static gchar *
sp_rect_description (SPItem * item)
{
	SPRect *rect;

	rect = SP_RECT (item);

	return g_strdup (_("Rectangle"));
}

#define C1 0.554

static void
sp_rect_set_shape (SPShape *shape)
{
	SPRect *rect;
	double x, y, w, h, w2, h2, rx, ry;
	SPCurve * c;

	rect = (SPRect *) shape;

	if ((rect->height.computed < 1e-18) || (rect->width.computed < 1e-18)) return;

	c = sp_curve_new ();

	x = rect->x.computed;
	y = rect->y.computed;
	w = rect->width.computed;
	h = rect->height.computed;
	w2 = w / 2;
	h2 = h / 2;
	if (rect->rx.set) {
		rx = CLAMP (rect->rx.computed, 0.0, rect->width.computed / 2);
	} else if (rect->ry.set) {
		rx = CLAMP (rect->ry.computed, 0.0, rect->width.computed / 2);
	} else {
		rx = 0.0;
	}
	if (rect->ry.set) {
		ry = CLAMP (rect->ry.computed, 0.0, rect->height.computed / 2);
	} else if (rect->rx.set) {
		ry = CLAMP (rect->rx.computed, 0.0, rect->height.computed / 2);
	} else {
		ry = 0.0;
	}

	if ((rx > 1e-18) && (ry > 1e-18)) {
		sp_curve_moveto (c, x + rx, y + 0.0);
		sp_curve_curveto (c, x + rx * (1 - C1), y + 0.0, x + 0.0, y + ry * (1 - C1), x + 0.0, y + ry);
		if (ry < h2) sp_curve_lineto (c, x + 0.0, y + h - ry);
		sp_curve_curveto (c, x + 0.0, y + h - ry * (1 - C1), x + rx * (1 - C1), y + h, x + rx, y + h);
		if (rx < w2) sp_curve_lineto (c, x + w - rx, y + h);
		sp_curve_curveto (c, x + w - rx * (1 - C1), y + h, x + w, y + h - ry * (1 - C1), x + w, y + h - ry);
		if (ry < h2) sp_curve_lineto (c, x + w, y + ry);
		sp_curve_curveto (c, x + w, y + ry * (1 - C1), x + w - rx * (1 - C1), y + 0.0, x + w - rx, y + 0.0);
		if (rx < w2) sp_curve_lineto (c, x + rx, y + 0.0);
	} else {
		sp_curve_moveto (c, x + 0.0, y + 0.0);
		sp_curve_lineto (c, x + 0.0, y + h);
		sp_curve_lineto (c, x + w, y + h);
		sp_curve_lineto (c, x + w, y + 0.0);
		sp_curve_lineto (c, x + 0.0, y + 0.0);
	}

	sp_curve_closepath_current (c);
	sp_shape_set_curve_insync (SP_SHAPE (rect), c, TRUE);
	sp_curve_unref (c);
}

/* fixme: Think (Lauris) */

void
sp_rect_position_set (SPRect * rect, gdouble x, gdouble y, gdouble width, gdouble height)
{
	g_return_if_fail (rect != NULL);
	g_return_if_fail (SP_IS_RECT (rect));

	rect->x.computed = x;
	rect->y.computed = y;
	rect->width.computed = width;
	rect->height.computed = height;

	sp_object_request_update (SP_OBJECT (rect), SP_OBJECT_MODIFIED_FLAG);
}

void
sp_rect_set_rx (SPRect * rect, gboolean set, gdouble value)
{
	g_return_if_fail (rect != NULL);
	g_return_if_fail (SP_IS_RECT (rect));

	rect->rx.set = set;
	if (set) rect->rx.computed = value;

	sp_object_request_update (SP_OBJECT (rect), SP_OBJECT_MODIFIED_FLAG);
}

void
sp_rect_set_ry (SPRect * rect, gboolean set, gdouble value)
{
	g_return_if_fail (rect != NULL);
	g_return_if_fail (SP_IS_RECT (rect));

	rect->ry.set = set;
	if (set) rect->ry.computed = value;

	sp_object_request_update (SP_OBJECT (rect), SP_OBJECT_MODIFIED_FLAG);
}

static int sp_rect_snappoints(SPItem *item, NR::Point p[], int size)
{
	SPRect *rect = SP_RECT(item);

	/* We use corners of rect only. */
	NR::Coord const x0 = rect->x.computed;
	NR::Coord const y0 = rect->y.computed;
	NR::Coord const x1 = x0 + rect->width.computed;
	NR::Coord const y1 = y0 + rect->height.computed;

	NR::Matrix const i2d(sp_item_i2d_affine(item));

	/* Note: these are the transformed corner points, not the corners of the bounding box of
	   the transformed rect. */
	int i = 0;
	if (i < size) {
		p[i++] = NR::Point(x0, y0) * i2d;
	}
	if (i < size) {
		p[i++] = NR::Point(x1, y0) * i2d;
	}
	if (i < size) {
		p[i++] = NR::Point(x1, y1) * i2d;
	}
	if (i < size) {
		p[i++] = NR::Point(x0, y1) * i2d;
	}

	return i;
}

/*
 * Initially we'll do:
 * Transform x, y, set x, y, clear translation
 */

/* fixme: Use preferred units somehow (Lauris) */
/* fixme: Alternately preserve whatever units there are (lauris) */

static NR::Matrix
sp_rect_set_transform (SPItem *item, NR::Matrix const &xform)
{
	SPRect *rect;

	gdouble sw, sh;
	SPStyle *style;

	rect = SP_RECT (item);

	/* Calculate rect start in parent coords */
	NR::Point pos=NR::Point(rect->x.computed, rect->y.computed) * xform;

	/* Clear translation */
	NR::Matrix remaining(xform);
	remaining[4] = remaining[5] = 0.0;

	/* Scalers */
	sw = sqrt (remaining[0] * remaining[0] + remaining[1] * remaining[1]);
	sh = sqrt (remaining[2] * remaining[2] + remaining[3] * remaining[3]);
	if (sw > 1e-9) {
		remaining[0] = remaining[0] / sw;
		remaining[1] = remaining[1] / sw;
	} else {
		remaining[0] = 1.0;
		remaining[1] = 0.0;
	}
	if (sh > 1e-9) {
		remaining[2] = remaining[2] / sh;
		remaining[3] = remaining[3] / sh;
	} else {
		remaining[2] = 0.0;
		remaining[3] = 1.0;
	}

	/* fixme: Would be nice to preserve units here */
	rect->width = rect->width.computed * sw;
	rect->height = rect->height.computed * sh;
	if (rect->rx.set) {
		rect->rx = rect->rx.computed * sw;
	}
	if (rect->ry.set) {
		rect->ry = rect->ry.computed * sh;
	}

	/* Find start in item coords */
	pos = pos * remaining.inverse();
	rect->x = pos[NR::X];
	rect->y = pos[NR::Y];

	/* And last but not least */
	style = SP_OBJECT_STYLE (item);
	if (style->stroke.type != SP_PAINT_TYPE_NONE) {
		if (!NR_DF_TEST_CLOSE (sw, 1.0, NR_EPSILON) || !NR_DF_TEST_CLOSE (sh, 1.0, NR_EPSILON)) {
			double scale;
			/* Scale changed, so we have to adjust stroke width */
			scale = sqrt (fabs (sw * sh));
			style->stroke_width.computed *= scale;
			if (style->stroke_dash.n_dash != 0) {
				int i;
				for (i = 0; i < style->stroke_dash.n_dash; i++) style->stroke_dash.dash[i] *= scale;
				style->stroke_dash.offset *= scale;
			}
		}
	}

	sp_object_request_update(SP_OBJECT(item), SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);

	return remaining;
}


/*
  Local Variables:
  mode:c++
  c-file-style:"bsd"
  c-file-offsets:((innamespace . 0) (inline-open . 0))
  indent-tabs-mode:t
  fill-column:99
  End:
  vim: noexpandtab :
*/
