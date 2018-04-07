#ifndef INKSCAPE_LPE_MEASURE_SEGMENTS_H
#define INKSCAPE_LPE_MEASURE_SEGMENTS_H

/*
 * Author(s):
 *     Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2014 Author(s)
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#include <gtkmm.h>
#include "live_effects/effect.h"
#include "live_effects/parameter/enum.h"
#include "live_effects/parameter/originalitemarray.h"
#include "live_effects/parameter/bool.h"
#include "live_effects/parameter/colorpicker.h"
#include "live_effects/parameter/fontbutton.h"
#include "live_effects/parameter/message.h"
#include "live_effects/parameter/text.h"
#include "live_effects/parameter/unit.h"


namespace Inkscape {
namespace LivePathEffect {

enum OrientationMethod {
    OM_HORIZONTAL,
    OM_VERTICAL,
    OM_PARALLEL,
    OM_END
};

class LPEMeasureSegments : public Effect {
public:
    LPEMeasureSegments(LivePathEffectObject *lpeobject);
    virtual ~LPEMeasureSegments();
    virtual void doOnApply(SPLPEItem const* lpeitem);
    virtual void doBeforeEffect (SPLPEItem const* lpeitem);
    virtual void doOnRemove(SPLPEItem const* /*lpeitem*/);
    virtual Geom::PathVector doEffect_path (Geom::PathVector const & path_in);
    virtual void doOnVisibilityToggled(SPLPEItem const* /*lpeitem*/);
    virtual Gtk::Widget * newWidget();
    virtual void transform_multiply(Geom::Affine const& postmul, bool set);
    void createLine(Geom::Point start,Geom::Point end, Glib::ustring name, size_t counter, bool main, bool remove, bool arrows = false);
    void createTextLabel(Geom::Point pos, size_t counter, double length, Geom::Coord angle, bool remove, bool valid);
    void createArrowMarker(Glib::ustring mode);
    bool isWhitelist(size_t i, gchar * blacklist_str, bool whitelist);
    void on_my_switch_page(Gtk::Widget* page, guint page_number);
private:
    UnitParam unit;
    EnumParam<OrientationMethod> orientation;
    ColorPickerParam coloropacity;
    FontButtonParam fontbutton;
    ScalarParam precision;
    ScalarParam fix_overlaps;
    ScalarParam position;
    ScalarParam text_top_bottom;
    ScalarParam helpline_distance;
    ScalarParam helpline_overlap;
    ScalarParam line_width;
    ScalarParam scale;
    TextParam format;
    TextParam blacklist;
    BoolParam active_projection;
    BoolParam whitelist;
    BoolParam arrows_outside;
    BoolParam flip_side;
    BoolParam scale_sensitive;
    BoolParam local_locale;
    BoolParam all_items_position;
    BoolParam rotate_anotation;
    BoolParam hide_back;
    BoolParam hide_arrows;
    BoolParam smallx100;
    OriginalItemArrayParam linked_items;
    ScalarParam distance_projection;
    TextParam blacklist_nodes;
    BoolParam whitelist_nodes;
    BoolParam vertical_projection;
    BoolParam oposite_projection;
    MessageParam general;
    MessageParam projection;
    MessageParam options;
    MessageParam tips;
    Glib::ustring display_unit;
    bool locked_pagenumber;
    double doc_scale;
    double fontsize;
    double anotation_width;
    double previous_size;
    unsigned long rgb24;
    guint32 rgb32;
    double arrow_gap;
    guint pagenumber;
    gchar const* locale_base;
    Geom::Affine star_ellipse_fix;
    LPEMeasureSegments(const LPEMeasureSegments &);
    LPEMeasureSegments &operator=(const LPEMeasureSegments &);

};

} //namespace LivePathEffect
} //namespace Inkscape

#endif

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :