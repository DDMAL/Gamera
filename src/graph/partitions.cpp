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

#include "partitions.hpp"

#define NP_NUMBER(a) ((a)->m_node_properties[1].SizeT)
#define NP_AVAIL(a) ((a)->m_node_properties[2].Bool)
#define NP_B(a) ((a)->m_node_properties[3].NodeSet_)
#define EP_PARTITION_COUNTER(a) ((a)->m_edge_properties[1].Bool)
#define EP_IN_CYCLE(a) ((a)->m_edge_properties[2].SizeT)

typedef std::vector<size_t> Cycles;

struct PartitionIterator : IteratorObject {
  int init(GraphObject* graph, Node* root, bool ids, double m, double b);
private:
  inline void find_subgraph(GraphObject* graph, Node* root);
  inline void find_cycles(Node* root);
  bool cycle(Node* s, Node* v, EdgeList& path);
  inline void unmark(Node* u);
public:
  static PyObject* next(IteratorObject* self);
private:
  static PyObject* next_no_edges(PartitionIterator* so);
  size_t m_npartitions;
  size_t m_i;
  size_t m_ones;
  size_t m_allowable_ones;
  bool m_no_edges;
  bool m_ids;
  Node* m_root;
  NodeList* m_subgraph;
  EdgeList* m_edges;
  Cycles* m_cycles;
  Cycles* m_cycle_sizes;
};  

int PartitionIterator::init(GraphObject* graph, Node* root, bool ids,
			    double m, double b) {
  m_ids = ids;
  m_subgraph = new NodeList();
  m_edges = new EdgeList();
  m_root = root;

  find_subgraph(graph, root);
  
  // Special case: if we haven't found any edges, we don't
  // need to do cycle detection, so we skip it
  if (m_edges->empty()) {
    m_no_edges = true;
    m_i = m_npartitions = 0;
    return 1;
  }

  m_cycles = new Cycles();
  m_cycle_sizes = new Cycles();
  m_cycles->reserve(m_subgraph->size());
  m_cycle_sizes->reserve(m_subgraph->size());
  find_cycles(root);

  size_t nedges = m_edges->size();
  m_npartitions = (size_t)pow(2, nedges);
  m_i = 0;
  m_ones = 0;
  m_allowable_ones = size_t(double(nedges) * m + b);
  std::cerr << nedges << " " << m_allowable_ones << " " << m_cycles->size() << "\n";
  return 1; 
}

void PartitionIterator::find_subgraph(GraphObject* graph, Node* root) {
  for (NodeVector::iterator i = graph->m_nodes->begin();
       i != graph->m_nodes->end(); ++i) {
    NP_VISITED(*i) = false;
    for (EdgeList::iterator j = (*i)->m_edges.begin();
	 j != (*i)->m_edges.end(); ++j) {
      EP_VISITED(*j) = false;
    }
  }

  size_t count = 1;
  NodeStack node_stack;
  node_stack.push(root);
  NP_VISITED(root) = true;
  while (!node_stack.empty()) {
    Node* node;
    node = node_stack.top();
    node_stack.pop();
    m_subgraph->push_back(node);
    NP_NUMBER(node) = count++;
    for (EdgeList::iterator j = node->m_edges.begin();
	 j != node->m_edges.end(); ++j) {
      Node* to_node = (*j)->traverse(node);
      if (!EP_VISITED(*j)) {
	EP_VISITED(*j) = true;
	EP_PARTITION_COUNTER(*j) = false;
	EP_IN_CYCLE(*j) = 0;
	m_edges->push_back(*j);
      }
      if (!NP_VISITED(to_node)) {
	node_stack.push(to_node);
	NP_VISITED(to_node) = true;
      }
    }
  }
}

void PartitionIterator::find_cycles(Node* root) {
  for (NodeList::iterator j = m_subgraph->begin();
       j != m_subgraph->end(); ++j) {
    NP_B(*j) = new NodeSet();
    NP_VISITED(*j) = false;
  }

  EdgeList path;
  for (NodeList::iterator i = m_subgraph->begin();
       i != m_subgraph->end(); ++i) {
    for (NodeList::iterator j = m_subgraph->begin();
	 j != m_subgraph->end(); ++j) {
      NP_AVAIL(*j) = true;
      NP_B(*j)->clear();
    }
    for (EdgeList::iterator j = m_edges->begin();
	 j != m_edges->end(); ++j) {
      EP_VISITED(*j) = false;
    }
    Node* s = *i;
    cycle(s, s, path);
    NP_VISITED(s) = true;
  }

  for (NodeList::iterator j = m_subgraph->begin();
       j != m_subgraph->end(); ++j)
    delete NP_B(*j);
}

bool PartitionIterator::cycle(Node* s, Node* v,
			      EdgeList& path) {
  bool flag = false;
  NP_AVAIL(v) = false;
  for (EdgeList::iterator i = v->m_edges.begin();
       i != v->m_edges.end(); ++i) {
    Node* w = (*i)->traverse(v);
    if (!NP_VISITED(w) && !EP_VISITED(*i)) {
      path.push_back(*i);
      EP_VISITED(*i) = true;
      if (w == s) {
	if (path.size() > 2) {
	  size_t marker = 1 << m_cycles->size();
	  for (EdgeList::iterator j = path.begin();
	       j != path.end(); ++j)
	    EP_IN_CYCLE(*j) |= marker;
	  m_cycles->push_back(0);
	  m_cycle_sizes->push_back(path.size() - 1);
	}
	flag = true;
      } else if NP_AVAIL(w)
	flag |= cycle(s, w, path);
      path.pop_back();
    }
  }
  if (flag)
    unmark(v);
  else 
    for (EdgeList::iterator i = v->m_edges.begin();
	 i != v->m_edges.end(); ++i) {
      Node* w = (*i)->traverse(v);
      if (!NP_VISITED(w))
	NP_B(w)->insert(v);
    }
  return flag;
}

void PartitionIterator::unmark(Node* u) {
  NP_AVAIL(u) = true;
  for (NodeSet::iterator i = NP_B(u)->begin();
       i != NP_B(u)->end(); ++i) {
    if (!NP_AVAIL(*i))
      unmark(*i);
  }
  NP_B(u)->clear();
}

PyObject* PartitionIterator::next(IteratorObject* self) {
  PartitionIterator* so = (PartitionIterator*)self;

  if (so->m_no_edges)
    return next_no_edges(so);

  if (so->m_i >= so->m_npartitions) {
    // Cleanup
    delete so->m_edges;
    delete so->m_subgraph;
    delete so->m_cycles;
    delete so->m_cycle_sizes;
    return 0;
  }
    
  for (NodeList::iterator i = so->m_subgraph->begin();
       i != so->m_subgraph->end(); ++i)
    NP_VISITED(*i) = false;

  PyObject* result = PyList_New(0);
  for (NodeList::iterator i = so->m_subgraph->begin();
       i != so->m_subgraph->end(); ++i) {
    size_t id = 0;
    if (!NP_VISITED(*i)) {
      PyObject* subresult = PyList_New(0);
      NodeStack node_stack;
      node_stack.push(*i);
      while (!node_stack.empty()) {
	Node* root = node_stack.top();
	node_stack.pop();
	NP_VISITED(root) = true;
	for (EdgeList::iterator j = root->m_edges.begin();
	     j != root->m_edges.end(); ++j) {
	  Node* inner_node = (*j)->traverse(root);
	  if (!NP_VISITED(inner_node)) {
	    if (EP_PARTITION_COUNTER(*j)) {
	      NP_VISITED(inner_node) = true;
	      node_stack.push(inner_node);
	    }
	  }
	}
	if (so->m_ids)
	  id |= 1 << NP_NUMBER(root);
	PyList_Append(subresult, root->m_data);
      }
      if (so->m_ids) {
	PyObject* tuple = PyTuple_New(2);
	PyTuple_SetItem(tuple, 0, PyInt_FromLong((long)id));
	PyTuple_SetItem(tuple, 1, subresult);
	PyList_Append(result, tuple);
	Py_DECREF(tuple);
      } else {
	PyList_Append(result, subresult);
	Py_DECREF(subresult);
      }
    }
  }
    
  // Do the addition
  while (so->m_i < so->m_npartitions) {
    for (EdgeList::iterator i = so->m_edges->begin();
	 i != so->m_edges->end(); ++i) {
      if (EP_PARTITION_COUNTER(*i)) {
	EP_PARTITION_COUNTER(*i) = false;
	so->m_ones--;
	if (EP_IN_CYCLE(*i)) {
	  size_t bit = 1;
	  for (size_t j = 0; j < so->m_cycles->size(); ++j) {
	    (*(so->m_cycles))[j] -= (EP_IN_CYCLE(*i) & bit);
	    bit <<= 1;
	  }
	}
      } else {
	EP_PARTITION_COUNTER(*i) = true;
	so->m_ones++;
	if (EP_IN_CYCLE(*i)) {
	  size_t bit = 1;
	  for (size_t j = 0; j < so->m_cycles->size(); ++j) {
	    (*(so->m_cycles))[j] += (EP_IN_CYCLE(*i) & bit);
	    bit <<= 1;
	  }
	}
	break;
      }
    }
    so->m_i++;

    // Constrain the maximum number of ones
    if (so->m_ones <= so->m_allowable_ones) {
      // Constrain the cycles -- not entirely sure this actually speeds
      // things up
      bool good_set = true;
      Cycles::iterator i = so->m_cycles->begin();
      Cycles::iterator j = so->m_cycle_sizes->begin();
      for (; i != so->m_cycles->end(); ++i, ++j)
	if (*i == *j) {
	  good_set = false;
	  break;
	}
      if (good_set)
	break;
    }
  }

  return result;
}

PyObject* PartitionIterator::next_no_edges(PartitionIterator* so) {
  PyObject* result = PyList_New(0);
  PyObject* subresult = PyList_New(0);
  PyList_Append(subresult, so->m_root->m_data);
  if (so->m_ids) {
    PyObject* tuple = PyTuple_New(2);
    PyTuple_SetItem(tuple, 0, PyInt_FromLong(1));
    PyTuple_SetItem(tuple, 1, subresult);
    PyList_Append(result, tuple);
    Py_DECREF(tuple);
  } else {
    PyList_Append(result, subresult);
    Py_DECREF(subresult);
  }
  so->m_no_edges = false;
  return result;
}

PyObject* graph_partitions(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  Node* root;
  int ids = 0;
  double m = 1, b = 0;
  if (PyArg_ParseTuple(args, "O|idd", &root, &ids, &m, &b) <= 0)
    return 0;
  root = graph_find_node(so, (PyObject*)root);
  if (root == NULL)
    return 0;
  PartitionIterator* iterator = iterator_new<PartitionIterator>();
  iterator->init(so, root, ids != 0, m, b);
  return (PyObject*)iterator;
}
