/*
 * Inkscape::Widgets::LayerSelector - layer selector widget
 *
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *
 * Copyright (C) 2004 MenTaLguY
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "config.h"
#include "helper/sp-intl.h"

#include <cstring>
#include <algorithm>
#include <functional>
#include <gtkmm/liststore.h>

#include "desktop-handles.h"
#include "selection.h"

#include "widgets/layer-selector.h"
#include "widgets/document-tree-model.h"
#include "widgets/shrink-wrap-button.h"
#include "widgets/icon.h"

#include "util/list.h"
#include "util/reverse-list.h"
#include "util/filter-list.h"

#include "sp-object.h"
#include "sp-item.h"
#include "desktop.h"
#include "document.h"

#include "xml/repr.h"
#include "xml/sp-repr-event-vector.h"

namespace Inkscape {
namespace Widgets {

namespace {

class AlternateIcons : public Gtk::HBox {
public:
    AlternateIcons(unsigned size, gchar const *a, gchar const *b)
    : _a(NULL), _b(NULL)
    {
        if (a) {
            _a = Gtk::manage(Glib::wrap(sp_icon_new(size, a)));
            _a->set_no_show_all(true);
            add(*_a);
        }
        if (b) {
            _b = Gtk::manage(Glib::wrap(sp_icon_new(size, b)));
            _b->set_no_show_all(true);
            add(*_b);
        }
        setState(false);
    }

    bool state() const { return _state; }
    void setState(bool state) {
        _state = state;
        if (_state) {
            if (_a) {
                _a->hide();
            }
            if (_b) {
                _b->show();
            }
        } else {
            if (_a) {
                _a->show();
            }
            if (_b) {
                _b->hide();
            }
        }
    }

private:
    Gtk::Widget *_a; 
    Gtk::Widget *_b;
    bool _state;
};

}

/** LayerSelector constructor.  Creates lock and hide buttons, 
 *  initalizes the layer dropdown selector with a label renderer,
 *  and hooks up signal for setting the desktop layer when the
 *  selector is changed.
 */ 
LayerSelector::LayerSelector(SPDesktop *desktop)
: _desktop(NULL), _layer(NULL)
{
    AlternateIcons *label;

    label = Gtk::manage(new AlternateIcons(11, "visible", "hidden"));
    _visibility_toggle.add(*label);
    _visibility_toggle.signal_toggled().connect(
        sigc::compose(
            sigc::mem_fun(*label, &AlternateIcons::setState),
            sigc::mem_fun(_visibility_toggle, &Gtk::ToggleButton::get_active)
        )
    );
    _hide_toggled_connection = _visibility_toggle.signal_toggled().connect(
        sigc::compose(
            sigc::mem_fun(*this, &LayerSelector::_hideLayer),
            sigc::mem_fun(_lock_toggle, &Gtk::ToggleButton::get_active)
        )
    );

    _visibility_toggle.set_relief(Gtk::RELIEF_NONE);
    shrink_wrap_button(_visibility_toggle);
    _tooltips.set_tip(_visibility_toggle, _("Toggle current layer visibility"));
    pack_start(_visibility_toggle, Gtk::PACK_EXPAND_PADDING);

    label = Gtk::manage(new AlternateIcons(11, "lock_unlocked", "width_height_lock"));
    _lock_toggle.add(*label);
    _lock_toggle.signal_toggled().connect(
        sigc::compose(
            sigc::mem_fun(*label, &AlternateIcons::setState),
            sigc::mem_fun(_lock_toggle, &Gtk::ToggleButton::get_active)
        )
    );
    _lock_toggled_connection = _lock_toggle.signal_toggled().connect(
        sigc::compose(
            sigc::mem_fun(*this, &LayerSelector::_lockLayer),
            sigc::mem_fun(_lock_toggle, &Gtk::ToggleButton::get_active)
        )
    );

    _lock_toggle.set_relief(Gtk::RELIEF_NONE);
    shrink_wrap_button(_lock_toggle);
    _tooltips.set_tip(_lock_toggle, _("Lock or unlock current layer"));
    pack_start(_lock_toggle, Gtk::PACK_EXPAND_PADDING);

    _tooltips.set_tip(_selector, _("Current layer"));
    pack_start(_selector, Gtk::PACK_EXPAND_WIDGET);

    _layer_model = Gtk::ListStore::create(_model_columns);
    _selector.set_model(_layer_model);
    _selector.pack_start(_label_renderer);
    _selector.set_cell_data_func(
        _label_renderer,
        sigc::mem_fun(*this, &LayerSelector::_prepareLabelRenderer)
    );

    _selection_changed_connection = _selector.signal_changed().connect(
        sigc::mem_fun(*this, &LayerSelector::_setDesktopLayer)
    );
    setDesktop(desktop);
}

/**  Destructor - disconnects signal handler 
 */
LayerSelector::~LayerSelector() {
    setDesktop(NULL);
    _selection_changed_connection.disconnect();
}

void LayerSelector::startRenameLayer() {
}

namespace {

/** Helper function - detaches desktop from selector 
 */
gboolean detach(SPView *view, LayerSelector *selector) {
    selector->setDesktop(NULL);
    return FALSE;
}

}

/** Sets the desktop for the widget.  First disconnects signals
 *  for the current desktop, then stores the pointer to the
 *  given \a desktop, and attaches its signals to this one.
 *  Then it selects the current layer for the desktop.
 */
void LayerSelector::setDesktop(SPDesktop *desktop) {
    if ( desktop == _desktop ) {
        return;
    }

    if (_desktop) {
        _layer_changed_connection.disconnect();
        g_signal_handlers_disconnect_by_func(_desktop, (gpointer)&detach, this);
    }
    _desktop = desktop;
    if (_desktop) {
        // TODO we need a different signal for this, really..
        g_signal_connect_after(_desktop, "shutdown", GCallback(detach), this);

        _layer_changed_connection = _desktop->connectCurrentLayerChanged(
            sigc::mem_fun(*this, &LayerSelector::_selectLayer)
        );
        _selectLayer(_desktop->currentLayer());
    }
}

namespace {

class is_layer {
public:
    is_layer(SPDesktop *desktop) : _desktop(desktop) {}
    bool operator()(SPObject &object) const {
        return _desktop->isLayer(&object);
    }
private:
    SPDesktop *_desktop;
};

class column_matches_object {
public:
    column_matches_object(Gtk::TreeModelColumn<SPObject *> const &column,
                          SPObject &object)
    : _column(column), _object(object) {}
    bool operator()(Gtk::TreeModel::const_iterator const &iter) const {
        return (*iter)[_column] == &_object;
    }
private:
    Gtk::TreeModelColumn<SPObject *> const &_column;
    SPObject &_object;
};

}

/** Selects the given layer in the dropdown selector.  
 */
void LayerSelector::_selectLayer(SPObject *layer) {
    using Inkscape::Util::cons;
    using Inkscape::Util::reverse_list;

    _selection_changed_connection.block();

    while (!_layer_model->children().empty()) {
        Gtk::ListStore::iterator first_row(_layer_model->children().begin());
        _destroyEntry(first_row);
        _layer_model->erase(first_row);
    }

    SPObject *root(_desktop->currentRoot());

    if (_layer) {
        sp_object_unref(_layer, NULL);
        _layer = NULL;
    }

    if (layer) {
        _buildEntries(0, cons(*root,
            reverse_list<SPObject::ParentIterator>(layer, root)
        ));

        Gtk::TreeIter row(
            std::find_if(
                _layer_model->children().begin(),
                _layer_model->children().end(),
                column_matches_object(_model_columns.object, *layer)
            )
        );
        if ( row != _layer_model->children().end() ) {
            _selector.set_active(row);
        }

        _layer = layer;
        sp_object_ref(_layer, NULL);
    }

    if ( !layer || layer == root ) {
        _visibility_toggle.set_sensitive(false);
        _visibility_toggle.set_active(false);
        _lock_toggle.set_sensitive(false);
        _lock_toggle.set_active(false);
    } else {
        _visibility_toggle.set_sensitive(true);
        _visibility_toggle.set_active(( SP_IS_ITEM(layer) ? SP_ITEM(layer)->isHidden() : false ));
        _lock_toggle.set_sensitive(true);
        _lock_toggle.set_active(( SP_IS_ITEM(layer) ? SP_ITEM(layer)->isLocked() : false ));
    }

    _selection_changed_connection.unblock();
}

/** Sets the current desktop layer to the actively selected layer.
 */
void LayerSelector::_setDesktopLayer() {
    Gtk::ListStore::iterator selected(_selector.get_active());
    SPObject *layer=_selector.get_active()->get_value(_model_columns.object);
    if ( _desktop && layer ) {
        _layer_changed_connection.block();
        _desktop->setCurrentLayer(layer);
        _layer_changed_connection.unblock();
        SP_DT_SELECTION(_desktop)->clear();
        _selectLayer(_desktop->currentLayer());
    }
}

/** Creates rows in the _layer_model data structure for each item
 *  in \a hierarchy, to a given \a depth.
 */
void LayerSelector::_buildEntries(unsigned depth,
                                  Inkscape::Util::List<SPObject &> hierarchy)
{
    using Inkscape::Util::List;
    using Inkscape::Util::rest;

    _buildEntry(depth, *hierarchy);

    List<SPObject &> remainder(rest(hierarchy));
    if ( remainder && rest(remainder) ) {
        _buildEntries(depth+1, remainder);
    } else {
        _buildSiblingEntries(depth+1, *hierarchy, remainder);
    }
}

/** Creates entries in the _layer_model data structure for
 *  all siblings of the first child in \a parent.
 */
void LayerSelector::_buildSiblingEntries(
    unsigned depth, SPObject &parent,
    Inkscape::Util::List<SPObject &> hierarchy
) {
    using Inkscape::Util::List;
    using Inkscape::Util::rest;
    using Inkscape::Util::reverse_list_in_place;
    using Inkscape::Util::filter_list;

    Inkscape::Util::List<SPObject &> siblings(
        reverse_list_in_place(
            filter_list<SPObject::SiblingIterator>(
                is_layer(_desktop), parent.firstChild(), NULL
            )
        )
    );

    SPObject *layer( hierarchy ? &*hierarchy : NULL );

    while (siblings) {
        _buildEntry(depth, *siblings);
        if ( &*siblings == layer ) {
            _buildSiblingEntries(depth+1, *layer, rest(hierarchy));
        }
        ++siblings;
    }
}

namespace {

struct Callbacks {
    sigc::slot<void> update_row;
    sigc::slot<void> update_list;
};

void attribute_changed(SPRepr *repr, gchar const *name,
                       gchar const *old_value, gchar const *new_value,
                       bool is_interactive, void *data) 
{
    if ( !std::strcmp(name, "id") || !std::strcmp(name, "inkscape:label") ) {
        reinterpret_cast<Callbacks *>(data)->update_row();
    } else if ( !std::strcmp(name, "inkscape:groupmode") ) {
        reinterpret_cast<Callbacks *>(data)->update_list();
    }
}

void node_added(SPRepr *parent, SPRepr *child, SPRepr *ref, void *data) {
    gchar const *mode=sp_repr_attr(child, "inkscape:groupmode");
    if ( mode && !std::strcmp(mode, "layer") ) {
        reinterpret_cast<Callbacks *>(data)->update_list();
    }
}

void node_removed(SPRepr *parent, SPRepr *child, SPRepr *ref, void *data) {
    gchar const *mode=sp_repr_attr(child, "inkscape:groupmode");
    if ( mode && !std::strcmp(mode, "layer") ) {
        reinterpret_cast<Callbacks *>(data)->update_list();
    }
}

void node_reordered(SPRepr *parent, SPRepr *child,
                    SPRepr *old_ref, SPRepr *new_ref, void *data)
{
    gchar const *mode=sp_repr_attr(child, "inkscape:groupmode");
    if ( mode && !std::strcmp(mode, "layer") ) {
        reinterpret_cast<Callbacks *>(data)->update_list();
    }
}

void update_row_for_object(SPObject &object,
                           Gtk::TreeModelColumn<SPObject *> const &column,
                           Glib::RefPtr<Gtk::ListStore> const &model)
{
    Gtk::TreeIter row(
        std::find_if(
            model->children().begin(),
            model->children().end(),
            column_matches_object(column, object)
        )
    );
    if ( row != model->children().end() ) {
        model->row_changed(model->get_path(row), row);
    }
}

void rebuild_all_rows(sigc::slot<void, SPObject *> rebuild, SPDesktop *desktop)
{
    rebuild(desktop->currentLayer());
}

}

/** Builds and appends a row in the layer model object.
 */
void LayerSelector::_buildEntry(unsigned depth, SPObject &object) {
    SPReprEventVector *vector;

    Callbacks *callbacks=new Callbacks();

    callbacks->update_row = sigc::bind(
        sigc::ptr_fun(&update_row_for_object),
        object, _model_columns.object, _layer_model
    );

    SPObject *layer=_desktop->currentLayer();
    if ( &object == layer || &object == SP_OBJECT_PARENT(layer) ) {
        callbacks->update_list = sigc::bind(
            sigc::ptr_fun(&rebuild_all_rows),
            sigc::mem_fun(*this, &LayerSelector::_selectLayer),
            _desktop
        );

        SPReprEventVector events = {
            NULL, &node_added,
            NULL, &node_removed,
            NULL, &attribute_changed,
            NULL, NULL,
            NULL, &node_reordered
        };

        vector = new SPReprEventVector(events);
    } else {
        SPReprEventVector events = {
            NULL, NULL,
            NULL, NULL,
            NULL, &attribute_changed,
            NULL, NULL,
            NULL, NULL
        };

        vector = new SPReprEventVector(events);
    }

    Gtk::ListStore::iterator row(_layer_model->append());

    row->set_value(_model_columns.depth, depth);

    sp_object_ref(&object, NULL);
    row->set_value(_model_columns.object, &object);

    sp_repr_ref(SP_OBJECT_REPR(&object));
    row->set_value(_model_columns.repr, SP_OBJECT_REPR(&object));

    row->set_value(_model_columns.callbacks, reinterpret_cast<void *>(callbacks));

    sp_repr_add_listener(SP_OBJECT_REPR(&object), vector, callbacks);
}

/** Removes a row from the _model_columns object, disconnecting listeners
 *  on the slot.
 */
void LayerSelector::_destroyEntry(Gtk::ListStore::iterator const &row) {
    Callbacks *callbacks=reinterpret_cast<Callbacks *>(row->get_value(_model_columns.callbacks));
    SPObject *object=row->get_value(_model_columns.object);
    if (object) {
        sp_object_unref(object, NULL);
    }
    SPRepr *repr=row->get_value(_model_columns.repr);
    if (repr) {
        sp_repr_remove_listener_by_data(repr, callbacks);
        sp_repr_unref(repr);
    }
    delete callbacks;
}

/** Formats the label for a given layer row 
 */
void LayerSelector::_prepareLabelRenderer(
    Gtk::TreeModel::const_iterator const &row
) {
    unsigned depth=(*row)[_model_columns.depth];
    SPObject *object=(*row)[_model_columns.object];
    bool label_defaulted(false);

    // TODO: when the currently selected row is removed,
    //       (or before one has been selected) something appears to
    //       "invent" an iterator with null data and try to render it;
    //       where does it come from, and how can we avoid it?
    if (object) {
        SPObject *layer=( _desktop ? _desktop->currentLayer() : NULL );
        SPObject *root=( _desktop ? _desktop->currentRoot() : NULL );

        gchar const *format;
        if ( layer && SP_OBJECT_PARENT(object) == SP_OBJECT_PARENT(layer) ||
             layer == root && SP_OBJECT_PARENT(object) == root
        ) {
            if ( object == layer && object != root ) {
                format="<small>%*s<b>%s</b></small>";
            } else {
                format="<small>%*s%s</small>";
            }
        } else {
            format="<small>%*s<small>%s</small></small>";
        }

        gchar const *label;
        if (depth) {
            label = object->label();
            if (!label) {
                label = object->defaultLabel();
                label_defaulted = true;
            }
        } else {
            label = "(root)";
        }

        gchar *text=g_markup_printf_escaped(format, depth*3, "", label);
        _label_renderer.property_markup() = text;
        g_free(text);
    } else {
        _label_renderer.property_markup() = "<small> </small>";
    }

    _label_renderer.property_ypad() = 1;
    _label_renderer.property_style() = ( label_defaulted ?
                                         Pango::STYLE_ITALIC :
                                         Pango::STYLE_NORMAL );
}

void LayerSelector::_lockLayer(bool lock) {
    if ( _layer && SP_IS_ITEM(_layer) ) {
        SP_ITEM(_layer)->setLocked(lock);
        sp_document_maybe_done(SP_DT_DOCUMENT(_desktop), "LayerSelector:lock");
    }
}

void LayerSelector::_hideLayer(bool hide) {
    if ( _layer && SP_IS_ITEM(_layer) ) {
        SP_ITEM(_layer)->setHidden(hide);
        sp_document_maybe_done(SP_DT_DOCUMENT(_desktop), "LayerSelector:hide");
    }
}

}
}

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=c++:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
