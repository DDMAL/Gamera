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

#define NP_VISITED2(a) ((a)->m_node_properties[2].Bool)

// This should always be a 64-bit unsigned
// If compiling on a platform with a larger available native integer length,
// you might want to use that instead
#ifdef _MSC_VER
typedef unsigned __int64 Bitfield;
#define BITFIELD_SIZE 64
#else
typedef unsigned long long Bitfield;
#define BITFIELD_SIZE 64
#endif

class Part {
public:
  inline Part(Bitfield _bits, double _score) :
    bits(_bits), score(_score), begin(), end() {};
  Bitfield bits;
  double score;
  size_t begin, end;
};
typedef std::vector<Part> Parts;
typedef std::vector<Bitfield> Solution;
typedef std::vector<Part> Parts;

inline Node* graph_optimize_partitions_find_root(Node* root, NodeVector& subgraph) {
  // Find the node with the minimum edges
  NodeQueue node_queue;
  node_queue.push(root);
  size_t min_edges = std::numeric_limits<size_t>::max();
  NP_VISITED(root) = true;
  while (!node_queue.empty()) {
    Node* node;
    node = node_queue.front();
    node_queue.pop();
    subgraph.push_back(node);
    if (node->m_edges.size() < min_edges) {
      min_edges = node->m_edges.size();
      root = node;
    }
    for (EdgeList::iterator j = node->m_edges.begin();
	 j != node->m_edges.end(); ++j) {
      Node* to_node = (*j)->traverse(node);
      if (!NP_VISITED(to_node)) {
	node_queue.push(to_node);
	NP_VISITED(to_node) = true;
      }
    }
  }
  return root;
}

inline void graph_optimize_partitions_number_parts(Node* root, NodeVector& subgraph) {
  // Now visit the graph in the correct order
  NodeQueue node_queue;
  node_queue.push(root);
  NP_VISITED2(root) = true;
  size_t count = 0;
  while (!node_queue.empty()) {
    Node* node;
    node = node_queue.front();
    node_queue.pop();
    subgraph.push_back(node);
    NP_NUMBER(node) = count++;
    for (EdgeList::iterator j = node->m_edges.begin();
	 j != node->m_edges.end(); ++j) {
      Node* to_node = (*j)->traverse(node);
      if (!NP_VISITED2(to_node)) {
	node_queue.push(to_node);
	NP_VISITED2(to_node) = true;
      }
    }
  }
}

inline void graph_optimize_partitions_evaluate_parts(Node* node, const size_t max_size,
						     const size_t subgraph_size,
						     NodeList& node_stack, Bitfield bits,
						     const PyObject* eval_func, Parts& parts) {
  size_t node_number = NP_NUMBER(node);
  node_stack.push_back(node);
  bits |= (Bitfield)1 << node_number;

  // Get the score for this part by building a Python list and
  // passing it to Python
  PyObject* result = PyList_New(node_stack.size());
  size_t j = 0;
  for (NodeList::iterator i = node_stack.begin();
       i != node_stack.end(); ++i, ++j)
    PyList_SET_ITEM(result, j, (*i)->m_data);

  PyObject* tuple = Py_BuildValue("(O)", result);
  PyObject* evalobject = PyObject_CallObject(const_cast<PyObject*>(eval_func), tuple);

  double eval;
  if (evalobject == NULL)
    eval = -1.0;
  else {
    if (PyFloat_Check(evalobject))
      eval = PyFloat_AsDouble(evalobject);
    else
      eval = -1.0;
  }

  parts.push_back(Part(bits, eval));

  if ((node_stack.size() < max_size) && 
      (NP_NUMBER(node) != subgraph_size - 1)) {
    for (EdgeList::iterator i = node->m_edges.begin();
	 i != node->m_edges.end(); ++i) {
      Node* to_node = (*i)->traverse(node);
      if (NP_NUMBER(to_node) > node_number)
	graph_optimize_partitions_evaluate_parts
	  (to_node, max_size, subgraph_size, node_stack, bits, eval_func, parts);
    }
  }

  node_stack.pop_back();
  return;
}

inline void graph_optimize_partitions_find_skips(Parts &parts) {
  for (size_t i = 0; i < parts.size(); ++i) {
    Part& root = parts[i];
    size_t j = i;
    for (; j < parts.size(); ++j)
      if (!(root.bits & parts[j].bits))
	break;
    root.begin = j;
    Bitfield temp = root.bits << 1;
    size_t k = j;
    for (; k < parts.size(); ++k)
      if (!(temp & parts[k].bits))
	break;
    root.end = k;
  }
}

inline void graph_optimize_partitions_find_solution(
  Parts &parts, const size_t begin, const size_t end, 
  Solution& best_solution, double &best_mean, Solution& partial_solution, 
  double partial_mean, const Bitfield bits, const Bitfield all_bits) {
  if (bits == all_bits) {
    partial_mean /= partial_solution.size();
    if (partial_mean > best_mean) {
      best_mean = partial_mean;
      best_solution = partial_solution; // Copy
    }
  }

  for (size_t i = begin; i < end; ++i) {
    const Part& root = parts[i];
    if (!(root.bits & bits)) { // If this part "fits into" the current part(s)
      partial_solution.push_back(root.bits);
      graph_optimize_partitions_find_solution
	(parts, root.begin, root.end, best_solution, best_mean,
	 partial_solution, partial_mean + root.score,
	 bits | root.bits, all_bits);
      partial_solution.pop_back();
    }
  }
}

PyObject* graph_optimize_partitions(const GraphObject* so, Node* root,
				    const PyObject* eval_func, const size_t max_size) {

  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i)
    NP_VISITED(*i) = NP_VISITED2(*i) = false;

  NodeVector subgraph;
  root = graph_optimize_partitions_find_root(root, subgraph);

  size_t size = subgraph.size();
  // We can't do the grouping if there's more than 64 nodes,
  // so just return them all.  Also, if there's only one node,
  // just trivially return it to save time.
  if (size > BITFIELD_SIZE || size == 1) {
    // Now, build a Python list of the solution
    PyObject* result = PyList_New(subgraph.size());
    for (size_t i = 0; i < subgraph.size(); ++i) {
      PyObject* subresult = PyList_New(1);
      Py_INCREF(subgraph[i]->m_data);
      PyList_SET_ITEM(subresult, 0, subgraph[i]->m_data);
      PyList_SET_ITEM(result, i, subresult);
    }
    return result;
  }

  subgraph.clear();
  subgraph.reserve(size);
  graph_optimize_partitions_number_parts(root, subgraph);

  // That gives us an idea of the number of nodes in the graph,
  // now go through and find the parts
  Parts parts;
  parts.reserve(size * max_size);
  for (NodeVector::iterator i = subgraph.begin();
       i != subgraph.end(); ++i) {
    NodeList node_stack;
    Bitfield bits = 0;
    graph_optimize_partitions_evaluate_parts(*i, max_size, size,
					     node_stack, bits, eval_func, parts);
  }

  graph_optimize_partitions_find_skips(parts);

  // Now, we find a solution
  Solution best_solution, partial_solution;
  best_solution.reserve(size); // Maximum size the solution can be
  partial_solution.reserve(size); // Maximum size the solution can be
  Bitfield all_bits = ((Bitfield)1 << size) - 1;
  double best_mean = 0;
  graph_optimize_partitions_find_solution(parts, 0, (*(parts.begin())).begin,
					  best_solution, best_mean,
					  partial_solution, 0.0,
					  0, all_bits);

  // Now, build a Python list of the solution
  PyObject* result = PyList_New(best_solution.size());
  for (size_t i = 0; i < best_solution.size(); ++i) {
    Bitfield solution_part = best_solution[i];
    size_t c = 0;
    for (size_t b=0; b < BITFIELD_SIZE; ++b) {
      if (((Bitfield)1 << b) & solution_part)
	++c;
    }
    // Count the set bits (Kernighan's method) so that we can allocate the
    // correct sized list from the get-go
    // (Kernighan's method does not seem to work on OS-X PPC, so I've
    // replaced it with the above)
    /*    size_t c = 0;
    for (; solution_part; c++)
    solution_part &= solution_part - 1; */
    PyObject* subresult = PyList_New(c);
    Bitfield k = (Bitfield)1;
    solution_part = best_solution[i];
    for (size_t j = 0, l = 0; k < solution_part; ++j, k <<= 1)
      if (solution_part & k) {
	PyObject* data = subgraph[j]->m_data;
	Py_INCREF(data);
	PyList_SET_ITEM(subresult, l++, data);
      }
    PyList_SET_ITEM(result, i, subresult);
  }

  return result;
}

PyObject* graph_optimize_partitions(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  PyObject* a, *eval_func;
  int max_size = 32;
  if (PyArg_ParseTuple(args, "OO|i", &a, &eval_func, &max_size) <= 0)
    return 0;
  Node* root = graph_find_node(so, (PyObject*)a);
  if (root == NULL)
    return 0;
  PyObject* result = graph_optimize_partitions(so, root, eval_func, max_size);
  return result;
}

