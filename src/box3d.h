#ifndef __SP_BOX3D_H__
#define __SP_BOX3D_H__

/*
 * SVG <box3d> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Maximilian Albert <Anhalter42@gmx.de>
 *
 * Copyright (C) 2007      Authors
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-item-group.h"
#include "proj_pt.h"
#include "axis-manip.h"

#define SP_TYPE_BOX3D            (box3d_get_type ())
#define SP_BOX3D(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_BOX3D, SPBox3D))
#define SP_BOX3D_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_BOX3D, Box3DClass))
#define SP_IS_BOX3D(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_BOX3D))
#define SP_IS_BOX3D_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_BOX3D))

class Box3DSide;
class Persp3D;
class Persp3DReference;

class SPBox3D : public SPGroup {
	public:
    gint z_orders[6]; // z_orders[i] holds the ID of the face at position #i in the group (from top to bottom)

    gchar *persp_href;
    Persp3DReference *persp_ref;

    Proj::Pt3 orig_corner0;
    Proj::Pt3 orig_corner7;

    Proj::Pt3 save_corner0;
    Proj::Pt3 save_corner7;

    Box3D::Axis swapped; // to indicate which coordinates are swapped during dragging

    gint my_counter; // for debugging only
	static SPBox3D * createBox3D(SPItem * parent);
};

class SPBox3DClass {
	public:
    SPGroupClass parent_class;
};

GType box3d_get_type (void);

void box3d_position_set (SPBox3D *box);
Proj::Pt3 box3d_get_proj_corner (SPBox3D const *box, guint id);
Geom::Point box3d_get_corner_screen (SPBox3D const *box, guint id, bool item_coords = true);
Proj::Pt3 box3d_get_proj_center (SPBox3D *box);
Geom::Point box3d_get_center_screen (SPBox3D *box);

void box3d_set_corner (SPBox3D *box, guint id, Geom::Point const &new_pos, Box3D::Axis movement, bool constrained);
void box3d_set_center (SPBox3D *box, Geom::Point const &new_pos, Geom::Point const &old_pos, Box3D::Axis movement, bool constrained);
void box3d_corners_for_PLs (const SPBox3D * box, Proj::Axis axis, Geom::Point &corner1, Geom::Point &corner2, Geom::Point &corner3, Geom::Point &corner4);
bool box3d_recompute_z_orders (SPBox3D *box);
void box3d_set_z_orders (SPBox3D *box);

int box3d_pt_lies_in_PL_sector (SPBox3D const *box, Geom::Point const &pt, int id1, int id2, Box3D::Axis axis);
int box3d_VP_lies_in_PL_sector (SPBox3D const *box, Proj::Axis vpdir, int id1, int id2, Box3D::Axis axis);

void box3d_relabel_corners(SPBox3D *box);
void box3d_check_for_swapped_coords(SPBox3D *box);

std::list<SPBox3D *> box3d_extract_boxes(SPObject *obj);

Persp3D *box3d_get_perspective(SPBox3D const *box);
void box3d_switch_perspectives(SPBox3D *box, Persp3D *old_persp, Persp3D *new_persp, bool recompute_corners = false);

SPGroup *box3d_convert_to_group(SPBox3D *box);


#endif /* __SP_BOX3D_H__ */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
