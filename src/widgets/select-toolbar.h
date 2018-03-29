#ifndef SEEN_SELECT_TOOLBAR_H
#define SEEN_SELECT_TOOLBAR_H

/** \file
 * Selector aux toolbar
 */
/*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <bulia@dr.com>
 *
 * Copyright (C) 2003 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

class SPDesktop;

namespace Gtk {
class Toolbar;
}

//typedef struct _GtkActionGroup GtkActionGroup;
//typedef struct _GObject GObject;

// void sp_select_toolbox_prep(SPDesktop *desktop, GtkActionGroup* mainActions, GObject* holder);

namespace Inkscape {
class Verb;

namespace UI {
namespace Widget {
class SelectToolbar : public Gtk::Toolbar {
private:
    SPDesktop *_desktop;
    Gtk::ToolButton * create_toolbutton_for_verb(unsigned int  verb_code);

public:
    SelectToolbar(SPDesktop *desktop);

    static GtkWidget *create(SPDesktop *desktop);
};
}
}
}

#endif /* !SEEN_SELECT_TOOLBAR_H */

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
