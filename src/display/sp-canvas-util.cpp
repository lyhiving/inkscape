#define __SP_CANVAS_UTILS_C__

/*
 * Helper stuff for SPCanvas
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <string.h>

#include "libnr/nr-matrix-div.h"
#include "libnr/nr-matrix-ops.h"
#include "sp-canvas-util.h"

void
sp_canvas_update_bbox (SPCanvasItem *item, int x1, int y1, int x2, int y2)
{
	sp_canvas_request_redraw (item->canvas, (int)item->x1, (int)item->y1, (int)item->x2, (int)item->y2);
	item->x1 = x1;
	item->y1 = y1;
	item->x2 = x2;
	item->y2 = y2;
	sp_canvas_request_redraw (item->canvas, (int)item->x1, (int)item->y1, (int)item->x2, (int)item->y2);
}

void
sp_canvas_item_reset_bounds (SPCanvasItem *item)
{
	item->x1 = 0.0;
	item->y1 = 0.0;
	item->x2 = 0.0;
	item->y2 = 0.0;
}

void
sp_canvas_buf_ensure_buf (SPCanvasBuf *buf)
{
	if (!buf->is_buf) {
		unsigned int r, g, b;
		int x, y;
		r = buf->bg_color >> 16;
		g = (buf->bg_color >> 8) & 0xff;
		b = buf->bg_color & 0xff;
		for (y = buf->rect.y0; y < buf->rect.y1; y++) {
			guchar *p;
			p = buf->buf + (y - buf->rect.y0) * buf->buf_rowstride;
			for (x = buf->rect.x0; x < buf->rect.x1; x++) {
				*p++ = r;
				*p++ = g;
				*p++ = b;
			}
		}
		buf->is_buf = 1;
	}
}

void
sp_canvas_clear_buffer (SPCanvasBuf *buf)
{
	unsigned char r, g, b;

	r = (buf->bg_color >> 16) & 0xff;
	g = (buf->bg_color >> 8) & 0xff;
	b = buf->bg_color & 0xff;

	if ((r != g) || (r != b)) {
		int x, y;
		for (y = buf->rect.y0; y < buf->rect.y1; y++) {
			unsigned char *p;
			p = buf->buf + (y - buf->rect.y0) * buf->buf_rowstride;
			for (x = buf->rect.x0; x < buf->rect.x1; x++) {
				*p++ = r;
				*p++ = g;
				*p++ = b;
			}
		}
	} else {
		int y;
		for (y = buf->rect.y0; y < buf->rect.y1; y++) {
			memset (buf->buf + (y - buf->rect.y0) * buf->buf_rowstride, r, 3 * (buf->rect.x1 - buf->rect.x0));
		}
	}
}

NR::Matrix sp_canvas_item_i2p_affine (SPCanvasItem * item)
{
	g_assert (item != NULL); // this may be overly zealous - it is
				 // plausible that this gets called
				 // with item == 0
	
	return item->xform;
}

NR::Matrix  sp_canvas_item_i2i_affine (SPCanvasItem * from, SPCanvasItem * to)
{
	g_assert (from != NULL);
	g_assert (to != NULL);

	return sp_canvas_item_i2w_affine(from) / sp_canvas_item_i2w_affine(to);
}

void sp_canvas_item_set_i2w_affine (SPCanvasItem * item,  NR::Matrix const &i2w)
{
	g_assert (item != NULL);

	sp_canvas_item_affine_absolute(item, i2w / sp_canvas_item_i2w_affine(item->parent));
}

void sp_canvas_item_move_to_z (SPCanvasItem * item, gint z)
{
	g_assert (item != NULL);

	gint current_z = sp_canvas_item_order (item);

	if (current_z == -1) // not found in its parent
		return;

	if (z == current_z)
		return;

	if (z > current_z)
		sp_canvas_item_raise (item, z - current_z);

	sp_canvas_item_lower (item, current_z - z);
}

gint
sp_canvas_item_compare_z (SPCanvasItem * a, SPCanvasItem * b)
{
	const gint o_a = sp_canvas_item_order (a);
	const gint o_b = sp_canvas_item_order (b);

	if (o_a > o_b) return -1;
	if (o_a < o_b) return 1;

	return 0;
}

