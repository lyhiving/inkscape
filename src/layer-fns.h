/*
 * assorted functions related to layers
 *
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *
 * Copyright (C) 2004 MenTaLguY
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef SEEN_INKSCAPE_LAYER_FNS_H
#define SEEN_INKSCAPE_LAYER_FNS_H

class SPObject;

namespace Inkscape {

SPObject *create_layer(SPObject *parent);

SPObject *next_layer(SPObject *root, SPObject *current);

SPObject *previous_layer(SPObject *root, SPObject *current);

}

#endif
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
