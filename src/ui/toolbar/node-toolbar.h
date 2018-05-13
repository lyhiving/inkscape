#ifndef SEEN_NODE_TOOLBAR_H
#define SEEN_NODE_TOOLBAR_H

/**
 * @file
 * Node aux toolbar
 */
/* Authors:
 *   MenTaLguY <mental@rydia.net>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Frank Felfe <innerspace@iname.com>
 *   John Cliff <simarilius@yahoo.com>
 *   David Turner <novalis@gnu.org>
 *   Josh Andler <scislac@scislac.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *   Tavmjong Bah <tavmjong@free.fr>
 *   Abhishek Sharma
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *
 * Copyright (C) 2004 David Turner
 * Copyright (C) 2003 MenTaLguY
 * Copyright (C) 1999-2011 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <gtkmm/adjustment.h>
#include <gtkmm/toolbar.h>

class SPDesktop;

namespace Inkscape {
class Selection;

namespace UI {
class PrefPusher;

namespace Widget {
class SpinButtonToolItem;
class UnitTracker;
}

namespace Toolbar {

class NodeToolbar : public Gtk::Toolbar {
private:
    SPDesktop *_desktop;
    Inkscape::UI::Widget::UnitTracker *_tracker;

    Glib::RefPtr<Gtk::Adjustment>  _x_coord_adj;
    Glib::RefPtr<Gtk::Adjustment>  _y_coord_adj;
    Inkscape::UI::Widget::SpinButtonToolItem *_x_coord_btn;
    Inkscape::UI::Widget::SpinButtonToolItem *_y_coord_btn;
    Gtk::ToggleToolButton         *_edit_clip_path_button;
    Gtk::ToggleToolButton         *_edit_mask_path_button;
    Gtk::ToggleToolButton         *_show_transform_handles_button;
    Gtk::ToggleToolButton         *_show_handles_button;
    Gtk::ToggleToolButton         *_show_helper_path_button;
    Gtk::ToolButton               *_next_pe_param_button;

    PrefPusher *_edit_clip_pusher;
    PrefPusher *_edit_mask_pusher;
    PrefPusher *_show_transform_handles_pusher;
    PrefPusher *_show_handles_pusher;
    PrefPusher *_show_helper_path_pusher;

    bool _freeze_flag; ///< Prevent signal handling if true

    // Signal handlers
    void on_insert_node_button_clicked();
    void on_insert_node_min_x_activated();
    void on_insert_node_max_x_activated();
    void on_insert_node_min_y_activated();
    void on_insert_node_max_y_activated();
    void on_delete_activated();
    void on_join_activated();
    void on_break_activated();
    void on_join_segment_activated();
    void on_delete_segment_activated();
    void on_cusp_activated();
    void on_smooth_activated();
    void on_symmetrical_activated();
    void on_auto_activated();
    void on_toline_activated();
    void on_tocurve_activated();
    void on_x_coord_adj_value_changed();
    void on_y_coord_adj_value_changed();

    void path_value_changed(Geom::Dim2 d);

    void create_insert_node_button();

    void selection_changed(Inkscape::Selection *selection);
    void selection_modified(Inkscape::Selection *selection, guint flags);
    void coord_changed(gpointer shape_editor);
    void watch_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec);

public:
    NodeToolbar(SPDesktop *desktop);
    ~NodeToolbar();
    static GtkWidget * create(SPDesktop *desktop);
};
}
}
}

#endif /* !SEEN_NODE_TOOLBAR_H */
/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
