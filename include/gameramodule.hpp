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

#ifndef KWM06292002_imagemodule
#define KWM06292002_imagemodule

#include "gamera.hpp"
#include <Python.h>

/*
  This file holds the C++ interface for the Python objects that wrap
  the core Gamera C++ objects. Each object has the struct for the Python
  object, a type-checking function, and a factory to create an instance
  of the Python type from the corresponding C++ type. Only the Python
  instance struct is exported here - the type struct is not exported, but
  is available via the object->tp_type field in the instance struct.
*/

struct SizeObject {
  PyObject_HEAD
  Size* m_x;
};

extern PyTypeObject* get_SizeType();
extern bool is_SizeObject(PyObject* x);
extern PyObject* create_SizeObject(const Size& d);

struct DimensionsObject {
  PyObject_HEAD
  Dimensions* m_x;
};

extern PyTypeObject* get_DimensionsType();
extern bool is_DimensionsObject(PyObject* x);
extern PyObject* create_DimensionsObject(const Dimensions& d);

struct PointObject {
  PyObject_HEAD
  Point* m_x;
};

extern PyTypeObject* get_PointType();
extern bool is_PointObject(PyObject* x);
extern PyObject* create_PointObject(const Point& p);

struct RectObject {
  PyObject_HEAD
  Rect* m_x;
};

extern PyTypeObject* get_RectType();
bool is_RectObject(PyObject* x);

struct ImageDataObject {
  PyObject_HEAD
  ImageDataBase* m_x;
  int m_pixel_type;
  int m_storage_format;
};

bool is_ImageDataObject(PyObject* x);
extern PyObject* create_ImageDataObject(int nrows, int ncols,
					int page_offset_y, int page_offset_x,
					int pixel_type, int storage_format);

struct ImageObject {
  RectObject m_parent; // we inherit from Rect
  PyObject* m_data; // an ImageDataObject for ref counting
  /*
    Classification related members
  */
  PyObject* m_features; // an array of doubles (Python array module)
  PyObject* m_id_name; // a list of strings for the classified ids
  PyObject* m_children_images; // list of images
  PyObject* m_classification_state; // how (or whether) an image is classified
  PyObject* m_scaling; // scaing value for the features
  PyObject* m_region_maps; // RegionMap object - see the object docs
  PyObject* m_region_map; // Current global region map
  PyObject* m_action_depth; // for limiting recursions for "actions"
};

  
namespace Gamera {
  namespace Python {
    /*
      Enumeration for all of the image types, pixel types, and storage
      types.
    */
    enum PixelTypes {
      ONEBIT,
      GREYSCALE,
      GREY16,
      RGB,
      FLOAT
    };
    
    enum ImageTypes {
      VIEW,
      STATIC,
      CC
    };
    
    enum StorageTypes {
      DENSE,
      RLE
    };

    enum ClassificationStates {
      UNCLASSIFIED,
      AUTOMATIC,
      HEURISTIC,
      MANUAL
    };
  }
}

#endif
