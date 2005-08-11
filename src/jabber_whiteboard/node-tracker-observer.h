/**
 * Convenience base class for XML::NodeObservers that need to extract data
 * from an XMLNodeTracker and queue up added or removed nodes for later
 * processing
 *
 * Authors:
 * David Yip <yipdw@rose-hulman.edu>
 *
 * Copyright (c) 2005 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef __WHITEBOARD_NODE_TRACKER_OBSERVER_H__
#define __WHITEBOARD_NODE_TRACKER_OBSERVER_H__

#include "xml/node-observer.h"

#include "jabber_whiteboard/node-tracker.h"
#include "jabber_whiteboard/typedefs.h"

namespace Inkscape {

namespace XML {

class Node;

}

namespace Whiteboard {

class NodeTrackerObserver : public XML::NodeObserver {
public:
	NodeTrackerObserver(XMLNodeTracker* xnt) : _xnt(xnt) { }
	virtual ~NodeTrackerObserver() { }

	// just reinforce the fact that we don't implement any of the 
	// notification methods here
    virtual void notifyChildAdded(XML::Node &node, XML::Node &child, XML::Node *prev)=0;

    virtual void notifyChildRemoved(XML::Node &node, XML::Node &child, XML::Node *prev)=0;

    virtual void notifyChildOrderChanged(XML::Node &node, XML::Node &child,
                                         XML::Node *old_prev, XML::Node *new_prev)=0;

    virtual void notifyContentChanged(XML::Node &node,
                                      Util::SharedCStringPtr old_content,
                                      Util::SharedCStringPtr new_content)=0;

    virtual void notifyAttributeChanged(XML::Node &node, GQuark name,
                                        Util::SharedCStringPtr old_value,
                                        Util::SharedCStringPtr new_value)=0;


	// ...but we do provide node tracking facilities
	KeyToNodeActionMap& getNodeActionMap()
	{
		return this->newnodes;
	}

	KeyToNodeActionMap getNodeActionMapCopy()
	{
		return this->newnodes;
	}

	void clearNodeBuffers()
	{
		this->newnodes.clear();
		this->newkeys.clear();
	}

protected:

	std::string _findOrGenerateNodeID(XML::Node& node)
	{
		NodeToKeyMap::iterator i = newkeys.find(&node);
		if (i != newkeys.end()) {
//			g_log(NULL, G_LOG_LEVEL_DEBUG, "(local) Found key %s for %p", i->second.c_str(), &node);
			return i->second;
		} else {
			std::string nodeid = this->_xnt->get(node);
			if (nodeid.empty()) {
	//			g_log(NULL, G_LOG_LEVEL_DEBUG, "Generating key for node %p", &node);
				nodeid = this->_xnt->generateKey();
				newnodes[nodeid] = SerializedEventNodeAction(NODE_ADD, &node);
				newkeys[&node] = nodeid;
			} else {
	//			g_log(NULL, G_LOG_LEVEL_DEBUG, "(tracker) Found key %s for %p", nodeid.c_str(), &node);
			}
			return nodeid;
		}
	}

	KeyToNodeActionMap newnodes;
	NodeToKeyMap newkeys;
	XMLNodeTracker* _xnt;

private:
	// noncopyable, nonassignable
	NodeTrackerObserver(NodeTrackerObserver const& other);
	NodeTrackerObserver& operator=(NodeTrackerObserver const& other);

};

}

}
#endif
