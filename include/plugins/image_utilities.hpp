/*
 *
 * Copyright (C) 2001-2005
 * Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

#ifndef kwm12032001_image_utilities
#define kwm12032001_image_utilities

#include "gamera.hpp"
#include "gameramodule.hpp"
#include "gamera_limits.hpp"
#include "vigra/resizeimage.hxx"
#include "plugins/logical.hpp"
#include <exception>
#include <math.h>
#include <algorithm>


namespace Gamera {
  
  /*
    This copies all of the misc attributes of an image (like
    label for Ccs or scaling).
  */
  template<class T, class U>
  void image_copy_attributes(const T& src, U& dest) {
    dest.scaling(src.scaling());
    dest.resolution(src.resolution());
  }

  /*
    These are full specializations for ConnectedComponents. This
    could be done with partial specialization, but that is broken
    on so many compilers it is easier just to do it manually :/
  */
  template<>
  void image_copy_attributes(const Cc& src, Cc& dest) {
    dest.scaling(src.scaling());
    dest.resolution(src.resolution());
    dest.label(src.label());
  }

  template<>
  void image_copy_attributes(const RleCc& src, Cc& dest) {
    dest.scaling(src.scaling());
    dest.resolution(src.resolution());
    dest.label(src.label());
  }

  template<>
  void image_copy_attributes(const Cc& src, RleCc& dest) {
    dest.scaling(src.scaling());
    dest.resolution(src.resolution());
    dest.label(src.label());
  }

  template<>
  void image_copy_attributes(const RleCc& src, RleCc& dest) {
    dest.scaling(src.scaling());
    dest.resolution(src.resolution());
    dest.label(src.label());
  }


  /*
    image_copy_fill

    This function copies the contents from one image to another of the same
    size. Presumably the pixel types of the two images are the same, but
    a cast is performed allowing any two pixels types with the approprate
    conversion functions defined (or built-in types) to be copied. The storage
    formats of the image do not need to match.
   */
  template<class T, class U>
  void image_copy_fill(const T& src, U& dest) {
    if ((src.nrows() != dest.nrows()) | (src.ncols() != dest.ncols()))
      throw std::range_error("image_copy_fill: src and dest image dimensions must match!");
    typename T::const_row_iterator src_row = src.row_begin();
    typename T::const_col_iterator src_col;
    typename U::row_iterator dest_row = dest.row_begin();
    typename U::col_iterator dest_col;
    ImageAccessor<typename T::value_type> src_acc;
    ImageAccessor<typename U::value_type> dest_acc;
    for (; src_row != src.row_end(); ++src_row, ++dest_row)
      for (src_col = src_row.begin(), dest_col = dest_row.begin(); src_col != src_row.end();
	   ++src_col, ++dest_col)
	dest_acc.set((typename U::value_type)src_acc.get(src_col), dest_col);
    image_copy_attributes(src, dest);
  }

  /*
    simple_image_copy

    This functions creates a new image of the same pixel type and storage format
    as the source image. If the image is a ConnectedComponent a OneBitImageView is
    returned rather that a ConnectedComponent (which is why the ImageFactory is used).
  */
  template<class T>
  typename ImageFactory<T>::view_type* simple_image_copy(const T& src) {
    typename ImageFactory<T>::data_type* dest_data =
      new typename ImageFactory<T>::data_type(src.size(), src.origin());
    typename ImageFactory<T>::view_type* dest =
      new typename ImageFactory<T>::view_type(*dest_data);
    try {
      image_copy_fill(src, *dest);
    } catch (std::exception e) {
      delete dest;
      delete dest_data;
      throw;
    }
    return dest;
  }

  

  /*
    image_copy

    This function creates a new image with the specified storage_format and
    copies the contents from the provided image. If the image is a ConnectedComponent a
    OneBit*ImageView is returned rather that a ConnectedComponent (which is why the
    ImageFactory is used).
  */
  template<class T>
  Image* image_copy(T &a, int storage_format) {
    if (storage_format == DENSE) {
      typename ImageFactory<T>::dense_data_type* data =
	new typename ImageFactory<T>::dense_data_type(a.size(), a.origin());
      typename ImageFactory<T>::dense_view_type* view =
	new typename ImageFactory<T>::dense_view_type(*data);
      try {
	image_copy_fill(a, *view);
      } catch (std::exception e) {
	delete view;
	delete data;
	throw;
      }
      return view;
    } else {
      typename ImageFactory<T>::rle_data_type* data =
	new typename ImageFactory<T>::rle_data_type(a.size(), a.origin());
      typename ImageFactory<T>::rle_view_type* view =
	new typename ImageFactory<T>::rle_view_type(*data);
      try {
	image_copy_fill(a, *view);
      } catch (std::exception e) {
	delete view;
	delete data;
	throw;
      }
      return view;
    }
  }


  /*
    union_images

    This function creates a new image that is the summation of all of the images
    in the passed-in list.
  */
  template<class T, class U>
  void _union_image(T& a, const U& b) {
    size_t ul_y = std::max(a.ul_y(), b.ul_y());
    size_t ul_x = std::max(a.ul_x(), b.ul_x());
    size_t lr_y = std::min(a.lr_y(), b.lr_y());
    size_t lr_x = std::min(a.lr_x(), b.lr_x());
    
    if (ul_y >= lr_y || ul_x >= lr_x)
      return;
    for (size_t y = ul_y, ya = y-a.ul_y(), yb=y-b.ul_y(); y <= lr_y; ++y, ++ya, ++yb)
      for (size_t x = ul_x, xa = x-a.ul_x(), xb=x-b.ul_x(); x <= lr_x; ++x, ++xa, ++xb) {
	if (is_black(a.get(Point(xa, ya))) || is_black(b.get(Point(xb, yb)))) {
	  a.set(Point(xa, ya), black(a));
	} else
	  a.set(Point(xa, ya), white(a));
      }
  }

  Image *union_images(ImageVector &list_of_images) {
    size_t min_x, min_y, max_x, max_y;
    min_x = min_y = std::numeric_limits<size_t>::max();
    max_x = max_y = 0;

    // Determine bounding box
    for (ImageVector::iterator i = list_of_images.begin();
	 i != list_of_images.end(); ++i) {
      Image* image = (*i).first;
      min_x = std::min(min_x, image->ul_x());
      min_y = std::min(min_y, image->ul_y());
      max_x = std::max(max_x, image->lr_x());
      max_y = std::max(max_y, image->lr_y()); 
    }

    size_t ncols = max_x - min_x + 1; 
    size_t nrows = max_y - min_y + 1;
    OneBitImageData *dest_data = new OneBitImageData(Dim(ncols, nrows), Point(min_x, min_y));
    OneBitImageView *dest = new OneBitImageView(*dest_data);
    // std::fill(dest->vec_begin(), dest->vec_end(), white(*dest));
    
    try {
      for (ImageVector::iterator i = list_of_images.begin();
	   i != list_of_images.end(); ++i) {
	Image* image = (*i).first;
	switch((*i).second) {
	case ONEBITIMAGEVIEW:
	  _union_image(*dest, *((OneBitImageView*)image));
	  break;
	case CC:
	  _union_image(*dest, *((Cc*)image));
	  break;
	case ONEBITRLEIMAGEVIEW:
	  _union_image(*dest, *((OneBitRleImageView*)image));
	  break;
	case RLECC:
	  _union_image(*dest, *((RleCc*)image));
	  break;
	default:
	  throw std::runtime_error
	    ("There is an Image in the list that is not a OneBit image.");
	}
      }
    } catch (std::exception e) {
      delete dest;
      delete dest_data;
      throw;
    }
    
    return dest;
  }

  template<class T>
  Image* resize(T& image, const Dim& dim, int resize_quality) {
    typename T::data_type* data = new typename T::data_type
      (dim, image.origin());
    ImageView<typename T::data_type>* view = 
      new ImageView<typename T::data_type>(*data);
    /*
      Images with nrows or ncols == 1 cannot be scaled. This is a hack that
      just returns an image with the same color as the upper-left pixel
    */
    if (image.nrows() <= 1 || image.ncols() <= 1 || 
	view->nrows() <= 1 || view->ncols() <= 1) {
      std::fill(view->vec_begin(), view->vec_end(), image.get(Point(0, 0)));
      return view;
    }
    if (resize_quality == 0) {
      resizeImageNoInterpolation(src_image_range(image), dest_image_range(*view));
    } else if (resize_quality == 1) {
      resizeImageLinearInterpolation(src_image_range(image), dest_image_range(*view));
    } else {
      resizeImageSplineInterpolation(src_image_range(image), dest_image_range(*view));
    }
    image_copy_attributes(image, *view);
    return view;
  }
  
#ifdef GAMERA_DEPRECATED
  /*
resize(T& image, int nrows, int ncols, int resize_quality)

Reason: (x, y) coordinate consistency.

Use resize(image, Dim(ncols, nrows), resize_quality) instead.
  */
  template<class T>
  GAMERA_CPP_DEPRECATED
  Image* resize(T& image, int nrows, int ncols, int resize_quality) {
    return resize(image, Dim(ncols, nrows), resize_quality);
  }
#endif

  template<class T>
  Image* scale(T& image, double scaling, int resize_quality) {
    // nrows, ncols are cast to a double so that the multiplication happens
    // exactly as it does in Python
    return resize(image, 
		  Dim(size_t(ceil(double(image.ncols()) * scaling)),
		      size_t(ceil(double(image.nrows()) * scaling))),
		  resize_quality);
  }


  /*
    FloatVector histogram(GreyScale|Grey16 image);

    Histogram returns a histogram of the values in an image. The
    values in the histogram are percentages.
  */
  template<class T>
  FloatVector* histogram(const T& image) {
    // The histogram is the size of all of the possible values of
    // the pixel type.
    size_t l = std::numeric_limits<typename T::value_type>::max() + 1;
    FloatVector* values = new FloatVector(l);

    try {
      // set the list to 0
      std::fill(values->begin(), values->end(), 0);
      
      typename T::const_row_iterator row = image.row_begin();
      typename T::const_col_iterator col;
      ImageAccessor<typename T::value_type> acc;

      // create the histogram
      for (; row != image.row_end(); ++row)
	for (col = row.begin(); col != row.end(); ++col)
	  (*values)[acc.get(col)]++;

      // convert from absolute values to percentages
      double size = image.nrows() * image.ncols();
      for (size_t i = 0; i < l; i++) {
	(*values)[i] = (*values)[i] / size;
      }
    } catch (std::exception e) {
      delete values;
      throw;
    }
    return values;
  }

  /*
    Find the maximum pixel value for an image
  */

  // TODO: Test this

  template<class T>
  typename T::value_type find_max(const T& image) {
    if (image.nrows() <= 1 || image.ncols() <= 1)
      throw std::range_error("Image must have nrows and ncols > 0.");
    typename T::const_vec_iterator max = image.vec_begin();
    typename T::value_type value = NumericTraits<typename T::value_type>::min();
    for (; max != image.vec_end(); ++max)
      _my_max(*max, value);
    return value;
  }
  
  template<class T>
  void _my_max(const T& a, T& b) {
    if (a > b)
      b = a;
  }

  template<>
  void _my_max(const ComplexPixel& a, ComplexPixel& b) {
    if (a.real() > b.real())
      b = a;
  }

  /*
    Fill an image with white.
  */
  template<class T>
  void fill_white(T& image) {
    std::fill(image.vec_begin(), image.vec_end(), white(image));
  }

  /*
    Fill an image with any color
  */
  template <class T>
  void fill(T& m, typename T::value_type color) {
    typename T:: vec_iterator destcolor = m.vec_begin();
    for(; destcolor != m.vec_end(); destcolor++)
      *destcolor = color;
  }

  /*
    Pad an image with the default value
  */

  template <class T>
  typename ImageFactory<T>::view_type* pad_image_default(const T &src, size_t top, size_t right, size_t bottom, size_t left)
  {
    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;
    data_type* dest_data = new data_type
      (Dim(src.ncols() + right + left, src.nrows() + top + bottom),
       src.origin());
    view_type* dest_srcpart = new view_type
      (*dest_data, Point(src.ul_x() + left, src.ul_y() + top),
       src.dim());
    view_type* dest = new view_type(*dest_data);
    
    try {
      image_copy_fill(src, *dest_srcpart);
    } catch (std::exception e) {
      delete dest;
      delete dest_srcpart;
      delete dest_data;
      throw;
    }

    delete dest_srcpart;

    return(dest);
  }


  /*
    Pad an image with any color
  */

  template <class T>
  typename ImageFactory<T>::view_type* pad_image(const T &src, size_t top, size_t right, size_t bottom, size_t left, typename T::value_type value)
  {
    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;
    data_type* dest_data = new data_type
      (Dim(src.ncols()+right+left, src.nrows()+top+bottom), 
       src.origin());
    view_type* top_pad = new view_type
      (*dest_data, Point(src.ul_x() + left, src.ul_y()),
       Dim(src.ncols() + right, top));
    view_type* right_pad = new view_type
      (*dest_data, Point(src.ul_x()+src.ncols()+left, src.ul_y()+top),
       Dim(right, src.nrows()+bottom));
    view_type* bottom_pad = new view_type
      (*dest_data, Point(src.ul_x(), src.ul_y()+src.nrows()+top), 
       Dim(src.ncols()+left, bottom));
    view_type* left_pad = new view_type
      (*dest_data, src.origin(), 
       Dim(left, src.nrows()+top));
    view_type* dest_srcpart = new view_type
      (*dest_data, Point(src.offset_x()+left, src.offset_y()+top), 
       src.dim());
    view_type* dest = new view_type(*dest_data);
    
    try {
      fill(*top_pad, value);
      fill(*right_pad, value);
      fill(*bottom_pad, value);
      fill(*left_pad, value);
      image_copy_fill(src, *dest_srcpart);
    } catch (std::exception e) {
      delete top_pad;
      delete right_pad;
      delete bottom_pad;
      delete left_pad;
      delete dest_srcpart;
      delete dest;
      delete dest_data;
    }

    delete top_pad;
    delete right_pad;
    delete bottom_pad;
    delete left_pad;
    delete dest_srcpart;

    return(dest);
  }


  template<class T>
  void invert(T& image) {
    ImageAccessor<typename T::value_type> acc;
    typename T::vec_iterator in = image.vec_begin();
    for (; in != image.vec_end(); ++in)
      acc.set(invert(acc(in)), in);
  }

  /*
    Shearing
  */

  template<class T>
  inline void simple_shear(T begin, const T end, int distance) {
    // short-circuit
    if (distance == 0)
      return;
    typename T::value_type filler;
    // move down or right
    if (distance > 0) {
      filler = *begin;
      std::copy_backward(begin, end - distance, end);
      std::fill(begin, begin + distance, filler);
      // move up or left
    } else if (distance < 0) {
      filler = *(end - 1);
      std::copy(begin - distance, end, begin);
      std::fill(end + distance, end, filler);
    } // if distance == 0, do nothing
  }

  template<class T>
  void shear_column(T& mat, size_t column, int distance) {
    if (size_t(std::abs(distance)) >= mat.nrows())
      throw std::range_error("Tried to shear column too far");
    if (column >= mat.ncols())
      throw std::range_error("Column argument to shear_column out of range");
    simple_shear((mat.col_begin() + column).begin(),
		 (mat.col_begin() + column).end(), distance);
  }

  template<class T>
  void shear_row(T& mat, size_t row, int distance) {
    if (size_t(std::abs(distance)) >= mat.ncols())
      throw std::range_error("Tried to shear column too far");
    if (row >= mat.nrows())
      throw std::range_error("Column argument to shear_column out of range");
    simple_shear((mat.row_begin() + row).begin(),
		 (mat.row_begin() + row).end(), distance);
  }

  template<class T>
  Image *clip_image(T& m, const Rect* rect) {
    if (m.intersects(*rect)) {
      size_t ul_y = std::max(m.ul_y(), rect->ul_y());
      size_t ul_x = std::max(m.ul_x(), rect->ul_x());
      size_t lr_y = std::min(m.lr_y(), rect->lr_y());
      size_t lr_x = std::min(m.lr_x(), rect->lr_x());
      return new T(m, Point(ul_x, ul_y), 
		   Dim(lr_x - ul_x + 1, lr_y - ul_y + 1));
    } else {
      return new T(m, Point(m.ul_x(), m.ul_y()), Dim(1, 1));
    };
  }

  template<class T, class U>
  typename ImageFactory<T>::view_type* mask(const T& a, U &b) {
    if (a.nrows() != b.nrows() || a.ncols() != b.ncols())
      throw std::runtime_error("The image and the mask image must be the same size.");

    typename ImageFactory<T>::data_type* dest_data =
      new typename ImageFactory<T>::data_type(b.size(), b.origin());
    typename ImageFactory<T>::view_type* dest =
      new typename ImageFactory<T>::view_type(*dest_data);

    typename ImageFactory<T>::view_type a_view = 
      typename ImageFactory<T>::view_type(a, b.ul(), b.size());

    ImageAccessor<typename T::value_type> a_accessor;
    ImageAccessor<typename U::value_type> b_accessor;

    typename T::vec_iterator it_a, end;
    typename U::vec_iterator it_b;
    typename T::vec_iterator it_dest;
    
    try { 
      for (it_a = a_view.vec_begin(), end = a_view.vec_end(), 
	     it_b = b.vec_begin(), it_dest = dest->vec_begin();
	   it_a != end; ++it_a, ++it_b, ++it_dest) {
	if (is_black(b_accessor.get(it_b)))
	  a_accessor.set(a_accessor.get(it_a), it_dest);
	else
	  a_accessor.set(white(*dest), it_dest);
      }
    } catch (std::exception e) {
      delete dest;
      delete dest_data;
      throw;
    }

    return dest;
  }
  
  template<class T>
  struct _nested_list_to_image {
    ImageView<ImageData<T> >* operator()(PyObject* obj) {
      ImageData<T>* data = NULL;
      ImageView<ImageData<T> >* image = NULL;
      
      PyObject* seq = PySequence_Fast(obj, "Argument must be a nested Python iterable of pixels.");
      if (seq == NULL)
	throw std::runtime_error("Argument must be a nested Python iterable of pixels.");
      int nrows = PySequence_Fast_GET_SIZE(seq);
      if (nrows == 0) {
	Py_DECREF(seq);
	throw std::runtime_error("Nested list must have at least one row.");
      }
      int ncols = -1;
      
      try {
	for (size_t r = 0; r < (size_t)nrows; ++r) {
	  PyObject* row = PyList_GET_ITEM(obj, r);
	  PyObject* row_seq = PySequence_Fast(row, "");
	  if (row_seq == NULL) {
	    pixel_from_python<T>::convert(row);
	    row_seq = seq;
	    Py_INCREF(row_seq);
	    nrows = 1;
	  }
	  int this_ncols = PySequence_Fast_GET_SIZE(row_seq);
	  if (ncols == -1) {
	    ncols = this_ncols;
	    if (ncols == 0) {
	      Py_DECREF(seq);
	      Py_DECREF(row_seq);
	      throw std::runtime_error
		("The rows must be at least one column wide.");
	    }
	    data = new ImageData<T>(Dim(ncols, nrows));
	    image = new ImageView<ImageData<T> >(*data);
	  } else {
	    if (ncols != this_ncols) {
	      delete image;
	      delete data;
	      Py_DECREF(row_seq);
	      Py_DECREF(seq);
	      throw std::runtime_error
		("Each row of the nested list must be the same length.");
	    }
	  }
	  for (size_t c = 0; c < (size_t)ncols; ++c) {
	    PyObject* item = PySequence_Fast_GET_ITEM(row_seq, c);
	    T px = pixel_from_python<T>::convert(item);
	    image->set(Point(c, r), px);
	  }
	  Py_DECREF(row_seq);
	}
	Py_DECREF(seq);
      } catch (std::exception e) {
	if (image)
	  delete image;
	if (data)
	  delete data;
	throw;
      }
      return image;
    }
  };
    
  Image* nested_list_to_image(PyObject* obj, int pixel_type) {
    // If pixel_type == -1, attempt to do an auto-detect.
    if (pixel_type < 0) {
      PyObject* seq = PySequence_Fast(obj, "Must be a nested Python iterable of pixels.");
      if (seq == NULL)
	throw std::runtime_error("Must be a nested Python list of pixels.");
      if (PySequence_Fast_GET_SIZE(seq) == 0) {
	Py_DECREF(seq);
	throw std::runtime_error("Nested list must have at least one row.");
      }
      PyObject* row = PySequence_Fast_GET_ITEM(seq, 0);
      PyObject* pixel;
      PyObject* row_seq = PySequence_Fast(row, "");
      if (row_seq == NULL) {
	pixel = row;
      } else {
	if (PySequence_Fast_GET_SIZE(row_seq) == 0) {
	  Py_DECREF(seq);
	  Py_DECREF(row_seq);
	  throw std::runtime_error("The rows must be at least one column wide.");
	}
	pixel = PySequence_Fast_GET_ITEM(row_seq, 0);
      }
      Py_DECREF(seq);
      Py_DECREF(row_seq);
      if (PyInt_Check(pixel))
	pixel_type = GREYSCALE;
      else if (PyFloat_Check(pixel))
	pixel_type = FLOAT;
      else if (is_RGBPixelObject(pixel))
	pixel_type = RGB;
      if (pixel_type < 0)
	throw std::runtime_error
	  ("The image type could not automatically be determined from the list.  Please specify an image type using the second argument.");
    }
      
    switch (pixel_type) {
    case ONEBIT:
      _nested_list_to_image<OneBitPixel> func1;
      return (Image*)func1(obj);
    case GREYSCALE:
      _nested_list_to_image<GreyScalePixel> func2;
      return (Image*)func2(obj);
    case GREY16:
      _nested_list_to_image<Grey16Pixel> func3;
      return (Image*)func3(obj);
    case RGB:
      _nested_list_to_image<RGBPixel> func4;
      return (Image*)func4(obj);
    case Gamera::FLOAT:
      _nested_list_to_image<FloatPixel> func5;
      return (Image*)func5(obj);
    default:
      throw std::runtime_error("Second argument is not a valid image type number.");
    }
  }
 
  template<class T>
  PyObject* to_nested_list(T& m) {
    PyObject* rows = PyList_New(m.nrows());
    for (size_t r = 0; r < m.nrows(); ++r) {
      PyObject* row = PyList_New(m.ncols());
      for (size_t c = 0; c < m.ncols(); ++c) {
	PyObject* px = pixel_to_python(m.get(Point(c, r)));
	PyList_SET_ITEM(row, c, px);
      }
      PyList_SET_ITEM(rows, r, row);
    }
    return rows;
  }

  template<class T>
  void mirror_horizontal(T& m) {
    for (size_t r = 0; r < size_t(m.nrows()) / 2; ++r) {
      for (size_t c = 0; c < m.ncols(); ++c) {
	typename T::value_type tmp = m.get(Point(c, r));
	m.set(Point(c, r), m.get(Point(c, m.nrows() - r - 1)));
	m.set(Point(c, m.nrows() - r - 1), tmp);
      }
    }
  }

  template<class T>
  void mirror_vertical(T& m) {
    for (size_t r = 0; r < m.nrows(); ++r) {
      for (size_t c = 0; c < size_t(m.ncols() / 2); ++c) {
	typename T::value_type tmp = m.get(Point(c, r));
	m.set(Point(c, r), m.get(Point(m.ncols() - c - 1, r)));
	m.set(Point(m.ncols() - c - 1, r), tmp);
      }
    }
  }

  template<class T>
  double mse(T& a, T& b) {
    if (a.size() != b.size())
      throw std::runtime_error("Both images must be the same size.");
    typename T::vec_iterator it_a, it_b;
    double error = 0;
    for (it_a = a.vec_begin(), it_b = b.vec_begin();
	 it_a != a.vec_end(); ++it_a, ++it_b) {
      double rdiff = (double)it_a->red() - it_b->red();
      double bdiff = (double)it_a->blue() - it_b->blue();
      double gdiff = (double)it_a->green() - it_b->green();
      error += rdiff*rdiff + bdiff*bdiff + gdiff*gdiff;
    }
    return (error / (a.nrows() * a.ncols())) / 3.0;
  }

  template<class T>
  void reset_onebit_image(T &image) {
    typename T::vec_iterator i;
    for (i = image.vec_begin(); i != image.vec_end(); ++i) {
      if (i.get() > 0) i.set(1);
    }
  }

}
#endif
