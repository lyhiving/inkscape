#define __SP_DROPPER_CONTEXT_C__

/*
 * Tool for picking colors from drawing
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <gdk/gdkkeysyms.h>

#include <libnr/nr-matrix.h>
#include <libnr/nr-pixblock.h>

#include "macros.h"
#include "helper/canvas-bpath.h"
#include "display/canvas-arena.h"
#include "enums.h"
#include "color.h"
#include "inkscape-private.h"
#include "desktop-affine.h"
#include "desktop-handles.h"

#include "dropper-context.h"
#include <libnr/nr-point-fns.h>
#include <libnr/nr-matrix-ops.h>
#include <algorithm>

#define C1 0.552
static const ArtBpath spdc_circle[] = {
	{ART_MOVETO, 0, 0, 0, 0, -1, 0},
	{ART_CURVETO, -1, C1, -C1, 1, 0, 1},
	{ART_CURVETO, C1, 1, 1, C1, 1, 0},
	{ART_CURVETO, 1, -C1, C1, -1, 0, -1},
	{ART_CURVETO, -C1, -1, -1, -C1, -1, 0},
	{ART_END, 0, 0, 0, 0, 0, 0}
};
#undef C1

static void sp_dropper_context_class_init (SPDropperContextClass *klass);
static void sp_dropper_context_init (SPDropperContext *dc);

static void sp_dropper_context_setup (SPEventContext *ec);
static void sp_dropper_context_finish (SPEventContext *ec);

static gint sp_dropper_context_root_handler (SPEventContext *ec, GdkEvent * event);

static SPEventContextClass *parent_class;

GType
sp_dropper_context_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPDropperContextClass),
			NULL, NULL,
			(GClassInitFunc) sp_dropper_context_class_init,
			NULL, NULL,
			sizeof (SPDropperContext),
			4,
			(GInstanceInitFunc) sp_dropper_context_init,
			NULL,	/* value_table */
		};
		type = g_type_register_static (SP_TYPE_EVENT_CONTEXT, "SPDropperContext", &info, (GTypeFlags)0);
	}
	return type;
}

static void
sp_dropper_context_class_init (SPDropperContextClass * klass)
{
	SPEventContextClass *ec_class = (SPEventContextClass *) klass;

	parent_class = (SPEventContextClass*)g_type_class_peek_parent (klass);

	ec_class->setup = sp_dropper_context_setup;
	ec_class->finish = sp_dropper_context_finish;
	ec_class->root_handler = sp_dropper_context_root_handler;
}

static void
sp_dropper_context_init (SPDropperContext *dc)
{
}

static void
sp_dropper_context_setup (SPEventContext *ec)
{
	SPDropperContext *dc = SP_DROPPER_CONTEXT (ec);

	if (((SPEventContextClass *) parent_class)->setup)
		((SPEventContextClass *) parent_class)->setup (ec);

	SPCurve *c = sp_curve_new_from_static_bpath ((ArtBpath *) spdc_circle);
	dc->area = sp_canvas_bpath_new (SP_DT_CONTROLS (ec->desktop), c);
	sp_curve_unref (c);
	sp_canvas_bpath_set_fill (SP_CANVAS_BPATH (dc->area), 0x00000000, (SPWindRule)0);
	sp_canvas_bpath_set_stroke (SP_CANVAS_BPATH (dc->area), 0x0000007f, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
	sp_canvas_item_hide (dc->area);
}

static void
sp_dropper_context_finish (SPEventContext *ec)
{
	SPDropperContext *dc = SP_DROPPER_CONTEXT (ec);

	if (dc->area) {
		gtk_object_destroy (GTK_OBJECT (dc->area));
		dc->area = NULL;
	}
}

static gint
sp_dropper_context_root_handler (SPEventContext *ec, GdkEvent *event)
{
	SPDropperContext *dc = (SPDropperContext *) ec;
	int ret = FALSE;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		if (event->button.button == 1) {
			dc->centre = NR::Point(event->button.x, event->button.y);
			dc->dragging = TRUE;
			ret = TRUE;
		}
		break;
	case GDK_MOTION_NOTIFY:
		if (dc->dragging) {
			const double rw = std::min(NR::L2(NR::Point(event->button.x, event->button.y) - dc->centre), 32.0);
			NR::Point cd = sp_desktop_w2d_xy_point (ec->desktop, dc->centre);
			NR::Matrix w2dt = sp_desktop_w2dt_affine (ec->desktop);
			const double scale = rw * NR_MATRIX_DF_EXPANSION (&w2dt);
			NR::Matrix const sm( NR::scale(scale, scale) * NR::translate(cd) );
			sp_canvas_item_affine_absolute (dc->area, sm);
			sp_canvas_item_show (dc->area);
			/* Get buffer */
			const int x0 = (int) floor (dc->centre[NR::X] - rw);
			const int y0 = (int) floor (dc->centre[NR::Y] - rw);
			const int x1 = (int) ceil (dc->centre[NR::X] + rw);
			const int y1 = (int) ceil (dc->centre[NR::Y] + rw);
			if ((x1 > x0) && (y1 > y0)) {
				NRPixBlock pb;
				SPColor color;
				nr_pixblock_setup_fast (&pb, NR_PIXBLOCK_MODE_R8G8B8A8P, x0, y0, x1, y1, TRUE);
				/* fixme: (Lauris) */
				sp_canvas_arena_render_pixblock (SP_CANVAS_ARENA (SP_DT_DRAWING (ec->desktop)), &pb);
				double W(0), R(0), G(0), B(0), A(0);
				for (int y = y0; y < y1; y++) {
					const unsigned char *s = NR_PIXBLOCK_PX (&pb) + (y - y0) * pb.rs;
					for (int x = x0; x < x1; x++) {
						const double dx = x - dc->centre[NR::X];
						const double dy = y - dc->centre[NR::Y];
						const double w = exp (-((dx * dx) + (dy * dy)) / (rw * rw));
						W += w;
						R += w * s[0];
						G += w * s[1];
						B += w * s[2];
						A += w * s[3];
						s += 4;
					}
				}
				nr_pixblock_release (&pb);
				R = (R + 0.001) / (255.0 * W);
				G = (G + 0.001) / (255.0 * W);
				B = (B + 0.001) / (255.0 * W);
				A = (A + 0.001) / (255.0 * W);
				R = CLAMP (R, 0.0, 1.0);
				G = CLAMP (G, 0.0, 1.0);
				B = CLAMP (B, 0.0, 1.0);
				A = CLAMP (A, 0.0, 1.0);
				sp_color_set_rgb_float (&color, R, G, B);
				inkscape_set_color (&color, A);
			}
			ret = TRUE;
		}
		break;
	case GDK_BUTTON_RELEASE:
		if (event->button.button == 1) {
			sp_canvas_item_hide (dc->area);
			dc->dragging = FALSE;
			ret = TRUE;
		}
		break;
	case GDK_KEY_PRESS:
		switch (event->key.keyval) {
		case GDK_Up: 
		case GDK_Down: 
		case GDK_KP_Up: 
		case GDK_KP_Down: 
			// prevent the zoom field from activation
			if (!MOD__CTRL_ONLY)
				ret = TRUE;
			break;
		default:
			break;
		}
	default:
		break;
	}

	if (!ret) {
		if (((SPEventContextClass *) parent_class)->root_handler)
			ret = ((SPEventContextClass *) parent_class)->root_handler (ec, event);
	}

	return ret;
}

