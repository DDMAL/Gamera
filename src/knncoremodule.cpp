/*
 *
 * Copyright (C) 2002 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

/*
  This module implements the low-level parts of the kNN classifier object. This
  implements the generic classifier interface for Gamera.
*/
#include "gameramodule.hpp"
#include "knn.hpp"
#include <algorithm>
#include <string.h>
#include <Python.h>
#include <assert.h>
#include <stdio.h>

using namespace Gamera;
using namespace Gamera::kNN;

extern "C" {
  void initknncore(void);
  static PyObject* knn_new(PyTypeObject* pytype, PyObject* args,
			   PyObject* kwds);
  static void knn_dealloc(PyObject* self);
  static PyObject* knn_instantiate_from_images(PyObject* self, PyObject* args);
  static PyObject* knn_classify(PyObject* self, PyObject* args);
  static PyObject* knn_classify_with_images(PyObject* self, PyObject* args);
  static PyObject* knn_get_num_k(PyObject* self);
  static int knn_set_num_k(PyObject* self, PyObject* v);
  static PyObject* knn_get_distance_type(PyObject* self);
  static int knn_set_distance_type(PyObject* self, PyObject* v);
  static PyObject* knn_leave_one_out(PyObject* self, PyObject* args);
  static PyObject* knn_serialize(PyObject* self, PyObject* args);
  static PyObject* knn_unserialize(PyObject* self, PyObject* args);
}

static PyTypeObject KnnType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

/*
  This enum is for selecting between the various methods of
  computing the distance between two floating-point feature
  vectors.
*/
enum DistanceType {
  CITY_BLOCK,
  EUCLIDEAN,
  FAST_EUCLIDEAN
};

/*
  The KnnObject holds all of the information needed by knn. Unlike
  many of the parts of Gamera, there is a significant amount of
  functionality implemented in this module rather than just a
  wrapper around a C++ objects/code.
*/
struct KnnObject {
  PyObject_HEAD
  // the number of features in each feature vector
  size_t num_features;
  // the total number of feature vectors stored in the database
  size_t num_feature_vectors;
  /*
    The feature vectors. A flat array of doubles (of size num_features
    * num_feature_vectors) is used to store the feature vectors for
    performance reasons (the memory access will be faster than using
    a multi-dimensional data structure). This does not complicate the
    implementation because this array is a fixed size (it is only
    used for non-interactive classification).
  */
  double* feature_vectors;
  // The id_names for the feature vectors
  char** id_names;
  // The current weights applied to the distance calculation
  double* weight_vector;
  /*
    The normalization applied to the feature vectors prior to distance
    calculation.
  */
  Normalize* normalize;
  /*
    Temporary storage for the normalized version of the unknown feature
    vector. This is simply to avoid allocating memory for each call to
    classify (and could potentially increase our cache hit rate, but who
    really knows).
  */
  double* normalized_unknown;
  // k - this is k-NN after all
  size_t num_k;
  // the distance type currently being used.
  DistanceType distance_type;
};


PyMethodDef knn_methods[] = {
  { "classify_with_images", knn_classify_with_images, METH_VARARGS,
    "classify an unknown image using a list of images." },
  { "instantiate_from_images", knn_instantiate_from_images, METH_VARARGS,
    "" },
  { "classify", knn_classify, METH_VARARGS,
    "" },
  { "leave_one_out", knn_leave_one_out, METH_VARARGS, "" },
  { "serialize", knn_serialize, METH_VARARGS, "" },
  { "unserialize", knn_unserialize, METH_VARARGS, "" },
  { NULL }
};

PyGetSetDef knn_getset[] = {
  { "num_k", (getter)knn_get_num_k, (setter)knn_set_num_k,
    "The value of k used for classification.", 0 },
  { "distance_type", (getter)knn_get_distance_type, (setter)knn_set_distance_type,
    "The type of distance calculation used.", 0 },
  { NULL }
};

// for type checking images - see initknn.
static PyTypeObject* imagebase_type;

/*
  Create a new kNN object and initialize all of the data.
*/
static PyObject* knn_new(PyTypeObject* pytype, PyObject* args,
			 PyObject* kwds) {
  KnnObject* o;
  o = (KnnObject*)pytype->tp_alloc(pytype, 0);
  o->num_features = 0;
  o->num_feature_vectors = 0;
  o->feature_vectors = 0;
  o->id_names = 0;
  o->num_k = 1;
  o->distance_type = CITY_BLOCK;
  o->weight_vector = 0;
  o->normalize = 0;
  o->normalized_unknown = 0;

  /*
    Initialize the weights.
  */
  o->weight_vector = new double[o->num_features];
  std::fill(o->weight_vector, o->weight_vector + o->num_features, 1.0);
  Py_INCREF(Py_None);
  /*
    Initialize the normalized unknown fv
  */
  o->normalized_unknown = new double[o->num_features];

  return (PyObject*)o;
}

/*
  Convenience function to delete all of the dynamic data used for
  classification.
*/
static void knn_delete_feature_data(KnnObject* o) {
  if (o->feature_vectors != 0) {
    delete o->feature_vectors;
    o->feature_vectors = 0;
  }
  if (o->id_names != 0) {
    for (size_t i = 0; i < o->num_feature_vectors; ++i) {
      if (o->id_names[i] != 0)
	delete o->id_names[i];
    }
    delete o->id_names;
    o->id_names = 0;
  }
  if (o->normalize != 0) {
    delete o->normalize;
    o->normalize = 0;
  }
  if (o->weight_vector != 0) {
    delete o->weight_vector;
    o->weight_vector = 0;
  }
  if (o->normalized_unknown != 0) {
    delete o->normalized_unknown;
    o->normalized_unknown = 0;
  }
  o->num_features = 0;
  o->num_feature_vectors = 0;
}

/*
  Create and initialize all of the classification data with the given
  number of features and number of feature vectors. Throughout this
  object it is assumed that the number of feature vectors is fixed. This
  is reasonable because if you need to classify using a changing set of
  known images classify_with_images is a much easier choice. Because
  we can assume a fixed number of feature vectors it makes allocation
  easier and also allows certain features (like normalization) to become
  a lot easier.
*/
static void knn_create_feature_data(KnnObject* o, size_t num_features,
				    size_t num_feature_vectors) {
  o->num_features = num_features;
  o->num_feature_vectors = num_feature_vectors;
  assert(o->num_features > 0 && o->num_feature_vectors > 0);

  o->feature_vectors = new double[o->num_features * o->num_feature_vectors];
  o->id_names = new char*[o->num_feature_vectors];
  for (size_t i = 0; i < o->num_feature_vectors; ++i)
    o->id_names[i] = 0;
  o->normalize = new Normalize(o->num_features);
  o->weight_vector = new double[o->num_features];
  std::fill(o->weight_vector, o->weight_vector + o->num_features, 1.0);
  o->normalized_unknown = new double[o->num_features];
}

// destructor for Python
static void knn_dealloc(PyObject* self) {
  KnnObject* o = (KnnObject*)self;
  knn_delete_feature_data(o);
  self->ob_type->tp_free(self);
}

/*
  A string comparison functor used by the kNearestNeighbors
  object.
*/
struct ltstr {
  bool operator()(const char* s1, const char* s2) const {
    return strcmp(s1, s2) < 0;
  }
};

/*
  get the feature vector from an image. image argument _must_ an image - no
  type checking is performed.
*/
inline int image_get_fv(PyObject* image, double** buf, int* len) {
  ImageObject* x = (ImageObject*)image;

  if (PyObject_CheckReadBuffer(x->m_features) < 0) {
    return -1;
  }

  if (PyObject_AsReadBuffer(x->m_features, (const void**)buf, len) < 0) {
    PyErr_SetString(PyExc_TypeError, "knn: Could not use image as read buffer.");
    return -1;
  }
  if (*len == 0) {
    return -1;
  }
  *len = *len / sizeof(double);
  return 0;
}


/*
  get the id_name from an image. The image argument _must_ be an image -
  no type checking is performed.
*/
inline int image_get_id_name(PyObject* image, char** id_name, int* len) {
  ImageObject* x = (ImageObject*)image;
  // PyList_Size shoule type check the argument
  if (PyList_Size(x->m_id_name) < 1) {
    PyErr_SetString(PyExc_TypeError, "knn: id_name not a list or list is empty.");
    return -1;
  }
  PyObject* id_tuple = PyList_GET_ITEM(x->m_id_name, 0);
  if (PyTuple_Size(id_tuple) != 2) {
    PyErr_SetString(PyExc_TypeError, "knn: id_name is not a tuple or is the wrong size.");
    return -1;
  }
  PyObject* id = PyTuple_GET_ITEM(id_tuple, 1);
  *id_name = PyString_AsString(id);
  if (*id_name == 0) {
    PyErr_SetString(PyExc_TypeError, "knn: could not get string from id_name tuple.");
    return -1;
  }
  *len = PyString_GET_SIZE(id);
  return 0;
}

/*
  Take a list of images from Python and instatiate the internal data structures
  for knn - this is used for non-interactive classification using the classify
  method. The major difference between interactive classification and non-interactive
  classification (other than speed) is that the data is normalized for non-interactive
  classification. The feature vectors are normalized in place ahead of time, so when
  the classifier is serialized the data is saved normalized. This is appropriate because
  non-interactive classifiers cannot have feature vectors added or deleted by definition.
*/
static PyObject* knn_instantiate_from_images(PyObject* self, PyObject* args) {
  PyObject* images;
  KnnObject* o = (KnnObject*)self;
  if (PyArg_ParseTuple(args, "O", &images) <= 0) {
    return 0;
  }
  /*
    Unlike classify_with_images this method requires a list so that the
    size can be known ahead of time. One of the advantages of the non-interactive
    classifier is that the data structures can be more static, so knowing the
    size ahead of time is _much_ easier.
  */
  if (!PyList_Check(images)) {
    PyErr_SetString(PyExc_TypeError, "knn: images must be a list!");
    return 0;
  }

  // delete all the feature data and initialize the object
  knn_delete_feature_data(o);

  int images_size = PyList_Size(images);
  if (images_size == 0) {
    PyErr_SetString(PyExc_TypeError, "List must be greater than 0.");
    return 0;
  }

  /*
    Determine the number of features by querying the first image
    in the list.
  */
  PyObject* first_image = PyList_GET_ITEM(images, 0);
  if (!PyObject_TypeCheck(first_image, imagebase_type)) {
    PyErr_SetString(PyExc_TypeError, "knn: expected an image");
    return 0;
  }
  double* tmp_fv;
  int tmp_fv_len;
  if (image_get_fv(first_image, &tmp_fv, &tmp_fv_len) < 0) {
    PyErr_SetString(PyExc_TypeError, "knn: could not get features from image");
    return 0;
  }
  /*
    Create all of the data
  */
  knn_create_feature_data(o, tmp_fv_len, images_size);
  /*
    Copy the id_names and the features to the internal data structures.
  */
  double* current_features = o->feature_vectors;
  for (size_t i = 0; i < o->num_feature_vectors; ++i, current_features += o->num_features) {
    PyObject* cur_image = PyList_GetItem(images, i);
    if (image_get_fv(cur_image, &tmp_fv, &tmp_fv_len) < 0) {
      knn_delete_feature_data(o);
      PyErr_SetString(PyExc_TypeError, "knn: could not get features from image");
      return 0;
    }
    if (size_t(tmp_fv_len) != o->num_features) {
      knn_delete_feature_data(o);
      PyErr_SetString(PyExc_TypeError, "knn: feature vector lengths don't match");
      return 0;      
    }
    for (size_t feature = 0; feature < o->num_features; ++feature) {
      current_features[feature] = tmp_fv[feature];
    }
    o->normalize->add(tmp_fv, tmp_fv + o->num_features);
    char* tmp_id_name;
    int len;
    if (image_get_id_name(cur_image, &tmp_id_name, &len) < 0) {
      knn_delete_feature_data(o);
      PyErr_SetString(PyExc_TypeError, "knn: could not get id name");
      return 0;
    }
    o->id_names[i] = new char[len + 1];
    strncpy(o->id_names[i], tmp_id_name, len + 1);
  }
  /*
    Apply the normalization
  */
  o->normalize->compute_normalization();
  current_features = o->feature_vectors;
  for (size_t i = 0; i < o->num_feature_vectors; ++i, current_features += o->num_features) {
    o->normalize->apply(current_features, current_features + o->num_features);
  }
  return Py_None;
}

/*
  non-interactive classification using the data created by
  instantiate from images.
*/
static PyObject* knn_classify(PyObject* self, PyObject* args) {
  KnnObject* o = (KnnObject*)self;
  if (o->feature_vectors == 0) {
      PyErr_SetString(PyExc_RuntimeError,
		      "knn: classify called before instantiate from images");
      return 0;          
  }
  PyObject* unknown;
  if (PyArg_ParseTuple(args, "O", &unknown) <= 0) {
    return 0;
  }

  if (!PyObject_TypeCheck(unknown, imagebase_type)) {
    PyErr_SetString(PyExc_TypeError, "knn: unknown must be an image");
    return 0;
  }
  double* fv;
  int fv_len;
  if (image_get_fv(unknown, &fv, &fv_len) < 0) {
    PyErr_SetString(PyExc_TypeError, "knn: could not get features");
    return 0;
  }
  if (size_t(fv_len) != o->num_features) {
    PyErr_SetString(PyExc_TypeError, "knn: features not the correct size");
    return 0;
  }
  
  // normalize the unknown
  o->normalize->apply(fv, fv + o->num_features, o->normalized_unknown);
  // create the kNN object
  kNearestNeighbors<char*, ltstr> knn(o->num_k);

  double* current_known = o->feature_vectors;

  for (size_t i = 0; i < o->num_feature_vectors; ++i, current_known += o->num_features) {
    double distance;
    if (o->distance_type == CITY_BLOCK) {
      distance = city_block_distance(current_known, current_known + o->num_features,
				     o->normalized_unknown, o->weight_vector);
    } else if (o->distance_type == FAST_EUCLIDEAN) {
      distance = fast_euclidean_distance(current_known, current_known + o->num_features,
					 o->normalized_unknown, o->weight_vector);
    } else {
      distance = euclidean_distance(current_known, current_known + o->num_features,
				    o->normalized_unknown, o->weight_vector);
    }

    knn.add(o->id_names[i], distance);
  }
  std::pair<char*, double> answer = knn.majority();
  PyObject* ans = PyTuple_New(2);
  PyTuple_SET_ITEM(ans, 0, PyFloat_FromDouble(answer.second));
  PyTuple_SET_ITEM(ans, 1, PyString_FromString(answer.first));
  PyObject* ans_list = PyList_New(1);
  PyList_SET_ITEM(ans_list, 0, ans);
  return ans_list;
}


/*
  Compute the distance between a known and an unknown feature
  vector with weights.
*/
inline int compute_distance(KnnObject* o, PyObject* known, PyObject* unknown,
			    double* distance) {
  double* known_buf, *unknown_buf;
  int known_len, unknown_len;

  if (image_get_fv(unknown, &unknown_buf, &unknown_len) < 0)
    return -1;

  if (image_get_fv(known, &known_buf, &known_len) < 0)
    return -1;

  if (unknown_len != known_len) {
    PyErr_SetString(PyExc_IndexError, "Array lengths do not match");
    return -1;
  }

  if (o->distance_type == CITY_BLOCK) {
    *distance = city_block_distance(known_buf, known_buf + known_len, unknown_buf,
				    o->weight_vector);
  } else if (o->distance_type == FAST_EUCLIDEAN) {
    *distance = euclidean_distance(known_buf, known_buf + known_len, unknown_buf,
				   o->weight_vector);
  } else {
    *distance = euclidean_distance(known_buf, known_buf + known_len, unknown_buf,
				   o->weight_vector);
  }
  return 0;
}

static PyObject* knn_classify_with_images(PyObject* self, PyObject* args) {
  KnnObject* o = (KnnObject*)self;
  PyObject* unknown, *iterator;
  if (PyArg_ParseTuple(args, "OO", &iterator, &unknown) <= 0) {
    return 0;
  }

  if (!PyIter_Check(iterator)) {
    PyErr_SetString(PyExc_TypeError, "Known features must be iterable.");
    return 0;
  }

  if (!PyObject_TypeCheck(unknown, imagebase_type)) {
    PyErr_SetString(PyExc_TypeError, "knn: unknown must be an image");
    return 0;
  }
  
  kNearestNeighbors<char*, ltstr> knn(o->num_k);
  PyObject* cur;
  while ((cur = PyIter_Next(iterator))) {
    if (!PyObject_TypeCheck(cur, imagebase_type)) {
      PyErr_SetString(PyExc_TypeError, "knn: non-image in known list");
    }
    double distance;
    if (compute_distance(o, cur, unknown, &distance) < 0)
      return 0;
    
    char* id_name;
    int len;
    if (image_get_id_name(cur, &id_name, &len) < 0)
      return 0;
    knn.add(id_name, distance);
    Py_DECREF(cur);
  }


  std::pair<char*, double> answer = knn.majority();
  PyObject* ans = PyTuple_New(2);
  PyTuple_SET_ITEM(ans, 0, PyFloat_FromDouble(answer.second));
  PyTuple_SET_ITEM(ans, 1, PyString_FromString(answer.first));
  PyObject* ans_list = PyList_New(1);
  PyList_SET_ITEM(ans_list, 0, ans);
  return ans_list;
}

static PyObject* knn_get_num_k(PyObject* self) {
  return Py_BuildValue("i", ((KnnObject*)self)->num_k);
}

static int knn_set_num_k(PyObject* self, PyObject* v) {
  if (!PyInt_Check(v)) {
    PyErr_SetString(PyExc_TypeError, "knn: expected an int.");
    return -1;
  }
  ((KnnObject*)self)->num_k = PyInt_AS_LONG(v);
  return 0;
}

static PyObject* knn_get_distance_type(PyObject* self) {
  return Py_BuildValue("i", ((KnnObject*)self)->distance_type);
}

static int knn_set_distance_type(PyObject* self, PyObject* v) {
  if (!PyInt_Check(v)) {
    PyErr_SetString(PyExc_TypeError, "knn: expected an int.");
    return -1;
  }
  ((KnnObject*)self)->distance_type = (DistanceType)PyInt_AS_LONG(v);
  return 0;
}

static double leave_one_out(KnnObject* o) {
  assert(o->feature_vectors != 0);
  kNearestNeighbors<char*, ltstr> knn(o->num_k);
  size_t total_correct = 0;
  for (size_t i = 0; i < o->num_feature_vectors; ++i) {
    for (size_t j = 0; j < o->num_feature_vectors; ++j) {
      if (i == j)
	continue;
      double distance;
      //double stop_distance = -1.0;
      if (o->distance_type == EUCLIDEAN) {
	distance = euclidean_distance(&o->feature_vectors[i],
				      &o->feature_vectors[i] + o->num_features,
				      &o->feature_vectors[j],
				      o->weight_vector);
      } else if (o->distance_type == FAST_EUCLIDEAN) {
	distance = fast_euclidean_distance(&o->feature_vectors[i],
					   &o->feature_vectors[i] + o->num_features,
					   &o->feature_vectors[j],
					   o->weight_vector);
      } else {
	distance = city_block_distance(&o->feature_vectors[i],
				       &o->feature_vectors[i] + o->num_features,
				       &o->feature_vectors[j],
				       o->weight_vector);
      }
      knn.add(o->id_names[j], distance);
    }
    std::pair<char*, double> answer = knn.majority();
    knn.reset();
    if (strcmp(answer.first, o->id_names[i]) == 0)
      total_correct++;
  }
  return double(total_correct) / o->num_feature_vectors;
}

/*
  Leave-one-out cross validation
*/
static PyObject* knn_leave_one_out(PyObject* self, PyObject* args) {
  KnnObject* o = (KnnObject*)self;
  if (o->feature_vectors == 0) {
    PyErr_SetString(PyExc_RuntimeError,
		    "knn: leave_one_out called before instantiate from images.");
    return 0;
  }
  return Py_BuildValue("f", leave_one_out(o));
}

/*
  Serialize and unserialize save and restore the internal data of the kNN object
  to/from a fast and compact binary format. This allows a user to create a file that
  can be used to create non-interactive classifiers in a very fast way.

  FORMAT
  
  The format is designed to be as simple as possible. First is a header consisting
  of the file format version (currently 1), then the size and settings of the data,
  and finally the data.

  HEADER

  size             what
  ------------------------------------------
  unsigned long    version
  unsigned long    number of k
  unsigned long    number of feaures
  unsigned long    number of feature vectors

  DATA

  The data as stored a list of id_names followed by the feature vectors.
  The id_names are stored as:

  size             what
  ------------------------------------------
  unsigned long    length of string
  char[]           id_name

  There are, of course, num_feature_vectors id_names. Next is the data which is
  simply written directly - i.e. num_feature_vectors arrays of doubles of length
  num_features.

*/
static PyObject* knn_serialize(PyObject* self, PyObject* args) {
  KnnObject* o = (KnnObject*)self;
  char* filename;
  if (PyArg_ParseTuple(args, "s", &filename) <= 0) {
    return 0;
  }

  FILE* file = fopen(filename, "wb");
  if (file == 0) {
    PyErr_SetString(PyExc_RuntimeError, "knn: error opening file.");
    return 0;
  }

  if (o->feature_vectors == 0) {
    PyErr_SetString(PyExc_RuntimeError, "knn: serialize called before instatiate from images.");
    return 0;
  }

  // write the header info
  unsigned long version = 1;
  if (fwrite((const void*)&version, sizeof(unsigned long), 1, file) != 1) {
    PyErr_SetString(PyExc_RuntimeError, "knn: problem writing to a file.");
    return 0;
  }
  unsigned long num_k = o->num_k;
  if (fwrite((const void*)&num_k, sizeof(unsigned long), 1, file) != 1) {
    PyErr_SetString(PyExc_RuntimeError, "knn: problem writing to a file.");
    return 0;
  }
  unsigned long num_features = o->num_features;
  if (fwrite((const void*)&num_features, sizeof(unsigned long), 1, file) != 1) {
    PyErr_SetString(PyExc_RuntimeError, "knn: problem writing to a file.");
    return 0;
  }
  unsigned long num_feature_vectors = o->num_feature_vectors;
  if (fwrite((const void*)&num_feature_vectors, sizeof(unsigned long), 1, file) != 1) {
    PyErr_SetString(PyExc_RuntimeError, "knn: problem writing to a file.");
    return 0;
  }
  
  for (size_t i = 0; i < o->num_feature_vectors; ++i) {
    unsigned long len = strlen(o->id_names[i]) + 1; // include \0
    if (fwrite((const void*)&len, sizeof(unsigned long), 1, file) != 1) {
      PyErr_SetString(PyExc_RuntimeError, "knn: problem writing to a file.");
      return 0;
    }
    if (fwrite((const void*)o->id_names[i], sizeof(char), len, file) != len) {
      PyErr_SetString(PyExc_RuntimeError, "knn: problem writing to a file.");
      return 0;
    }
  }

  if (fwrite((const void*)o->normalize->get_norm_vector(),
	     sizeof(double), o->num_features, file) != o->num_features) {
    PyErr_SetString(PyExc_RuntimeError, "knn: problem writing to a file.");
    return 0;
  }

  if (fwrite((const void*)o->weight_vector, sizeof(double), o->num_features, file)
      != o->num_features) {
    PyErr_SetString(PyExc_RuntimeError, "knn: problem writing to a file.");
    return 0;
  }

  // write the data
  double* cur = o->feature_vectors;
  for (size_t i = 0; i < o->num_feature_vectors; ++i, cur += o->num_features) {
    if (fwrite((const void*)cur, sizeof(double), o->num_features, file)
	!= o->num_features) {
      PyErr_SetString(PyExc_RuntimeError, "knn: problem writing to a file.");
      return 0;
    }
  }
  fclose(file);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* knn_unserialize(PyObject* self, PyObject* args) {
  KnnObject* o = (KnnObject*)self;
  char* filename;
  if (PyArg_ParseTuple(args, "s", &filename) <= 0)
    return 0;

  FILE* file = fopen(filename, "rb");
  if (file == 0) {
    PyErr_SetString(PyExc_RuntimeError, "knn: error opening file.");
    return 0;
  }

  unsigned long version, num_k, num_features, num_feature_vectors;
  if (fread((void*)&version, sizeof(unsigned long), 1, file) != 1) {
    PyErr_SetString(PyExc_RuntimeError, "knn: problem reading file.");
    return 0;
  }
  if (version != 1) {
    PyErr_SetString(PyExc_RuntimeError, "knn: unknown version of knn file.");
    return 0;
  }
  if (fread((void*)&num_k, sizeof(unsigned long), 1, file) != 1) {
    PyErr_SetString(PyExc_RuntimeError, "knn: problem reading file.");
    return 0;
  }
  if (fread((void*)&num_features, sizeof(unsigned long), 1, file) != 1) {
    PyErr_SetString(PyExc_RuntimeError, "knn: problem reading file.");
    return 0;
  }
  if (fread((void*)&num_feature_vectors, sizeof(unsigned long), 1, file) != 1) {
    PyErr_SetString(PyExc_RuntimeError, "knn: problem reading file.");
    return 0;
  }

  knn_delete_feature_data(o);
  knn_create_feature_data(o, (size_t)num_features, (size_t)num_feature_vectors);
  o->num_k = num_k;

  for (size_t i = 0; i < o->num_feature_vectors; ++i) {
    unsigned long len;
    if (fread((void*)&len, sizeof(unsigned long), 1, file) != 1) {
      PyErr_SetString(PyExc_RuntimeError, "knn: problem reading file.");
      return 0;
    }
    o->id_names[i] = new char[len];
    if (fread((void*)o->id_names[i], sizeof(char), len, file) != len) {
      PyErr_SetString(PyExc_RuntimeError, "knn: problem reading file.");
      return 0;
    }
  }

  double* tmp_norm = new double[o->num_features];
  if (fread((void*)tmp_norm, sizeof(double), o->num_features, file) != o->num_features) {
    PyErr_SetString(PyExc_RuntimeError, "knn: problem reading file.");
    return 0;
  }
  o->normalize->set_norm_vector(tmp_norm, tmp_norm + o->num_features);
  delete tmp_norm;
  if (fread((void*)o->weight_vector, sizeof(double), o->num_features, file) != o->num_features) {
    PyErr_SetString(PyExc_RuntimeError, "knn: problem reading file.");
    return 0;
  }

  double* cur = o->feature_vectors;
  for (size_t i = 0; i < o->num_feature_vectors; ++i, cur += o->num_features) {
    if (fread((void*)cur, sizeof(double), o->num_features, file) != o->num_features) {
      PyErr_SetString(PyExc_RuntimeError, "knn: problem reading file.");
      return 0;
    }
  }

  fclose(file);
  Py_INCREF(Py_None);
  return Py_None;
}

PyMethodDef knn_module_methods[] = {
  { NULL }
};

DL_EXPORT(void) initknncore(void) {
  PyObject* m = Py_InitModule("gamera.knncore", knn_module_methods);
  PyObject* d = PyModule_GetDict(m);

  KnnType.ob_type = &PyType_Type;
  KnnType.tp_name = "gamera.knncore.kNN";
  KnnType.tp_basicsize = sizeof(KnnObject);
  KnnType.tp_dealloc = knn_dealloc;
  KnnType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  KnnType.tp_new = knn_new;
  KnnType.tp_getattro = PyObject_GenericGetAttr;
  KnnType.tp_alloc = PyType_GenericAlloc;
  KnnType.tp_free = _PyObject_Del;
  KnnType.tp_methods = knn_methods;
  KnnType.tp_getset = knn_getset;
  PyType_Ready(&KnnType);
  PyDict_SetItemString(d, "kNN", (PyObject*)&KnnType);
  PyDict_SetItemString(d, "CITY_BLOCK",
		       Py_BuildValue("i", CITY_BLOCK));
  PyDict_SetItemString(d, "EUCLIDEAN",
		       Py_BuildValue("i", EUCLIDEAN));
  PyDict_SetItemString(d, "FAST_EUCLIDEAN",
		       Py_BuildValue("i", FAST_EUCLIDEAN));

  /*
    We need to type check the images passed in so we need
    to have the image type around. By looking up the type
    at module init time we can save some overhead in the
    function calles. gamera.gameracore.Image is used because
    it is the base type for _all_ of the image classes in
    Gamera.
  */
  PyObject* mod = PyImport_ImportModule("gamera.gameracore");
  if (mod == 0) {
    PyErr_SetString(PyExc_RuntimeError, "Unable to load gameracore.\n");
    return;
  }
  PyObject* dict = PyModule_GetDict(mod);
  if (dict == 0) {
    PyErr_SetString(PyExc_RuntimeError, "Unable to get module dictionary\n");
    return;
  }
  imagebase_type = (PyTypeObject*)PyDict_GetItemString(dict, "Image");
}