/*
 *
 * Copyright (C) 2001 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef mgd010103_spanning_tree_hpp
#define mgd010103_spanning_tree_hpp

#include "graphlib.hpp"
#include "graph.hpp"

extern "C" {
  PyObject* graph_create_spanning_tree(PyObject* self, PyObject* pyobject);
  PyObject* graph_create_minimum_spanning_tree(PyObject* so, PyObject* args);
}
GraphObject* graph_create_spanning_tree(GraphObject* so, Node* root);
GraphObject* graph_create_minimum_spanning_tree(GraphObject* so);

#define SPANNING_TREE_METHODS \
  { "create_spanning_tree", graph_create_spanning_tree, METH_O, \
    "Returns a new graph with a spanning tree of all nodes reachable from the given node" }, \
  { "create_minimum_spanning_tree", graph_create_minimum_spanning_tree, METH_NOARGS, \
    "Creates a minimum spanning tree (in place)" },

#endif
