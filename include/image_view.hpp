/*
 *
 * Copyright (C) 2001-2002 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

#ifndef kwm11162001_matrix_view_hpp
#define kwm11162001_matrix_view_hpp

#include "matrix_view_iterators.hpp"
#include "dimensions.hpp"
#include "matrix.hpp"
#include "accessor.hpp"
#include "vigra_iterators.hpp"
#include "vigra/utilities.hxx"

#include <exception>
#include <stdexcept>
#include <string>
#include <cstring>
#include <cstdio>
#include <utility>

/*
  MATRIXVIEW
  ----------
  This is the "standard" view on a matrix data object. The goal
  of this class is to convert a 1-dimensional data structure
  (something like an array) into a 2-dimensional image that may
  or may not encompass the entire data available.

  DATA LAYOUT
  -----------
  The image data is arranged in row-major format, meaning that
  the rows are concatenated together to form an array. The end
  of one row is followed by the begining of the next. The distance
  between the rows is called the stride. The index of any given row
  can be computed by multiplying the row number with the stride. Within
  each row, the each column is stored sequentially. The following diagram
  demonstrates.

  |stride |
  -------------------------
  |1|2|3|4|1|2|3|4|1|2|3|4| (the numbers refer to the column)
  -------------------------
  row 1    row 2   row 3

  VIEWS
  -----
  The goal of decoupling the 2-dimensional interface from the data is
  to allow a "view" on the data to be a very lightweight object. This
  allows it to be passed by value with little worry about performance. This
  also allows the processing of a portion of an image as if it were an entire
  image. This same effect can be acheived by storing sets of Vigra style 2D
  iterators, but the class interface is easily to manipulate in many situations
  and it is much simpler to wrap for python.

  ITERATOR CACHING
  ----------------
  All of the operations to get and set pixels are done through iterators. To
  help with performance, iterators pointing to the beginning and end of the
  view dimensions are pre-calculated and stored (one const and one non-const).
  This should help speed up begin() and end() in exchange for a modest increase
  in the overall size of the object.

  INTERFACES
  ----------
  This class presents several interfaces in an effort to ease porting of older
  code or integration with other libraries. The simplist access is through
  get and set methods:
    pixel_value = matrix.get(row, col);
    matrix.set(row, col, value);
  The disadvantage of this method is that each access requires a multiply and
  add.  A similar interface to the get/set method is the operator[] interface. This
  presents an interface similar to a C multi-dimensional array. For example:
    pixel_value = matrix[row][column];
    matrix[row][column] = value;
*/

namespace Gamera {

  using namespace vigra;

  template<class T>
  class MatrixView
    : public MatrixBase<typename T::value_type> {
  public:
    // standard STL typedefs
    typedef typename T::value_type value_type;
    typedef typename T::pointer pointer;
    typedef typename T::reference reference;
    typedef typename T::difference_type difference_type;

    // Gamera specific
    typedef T data_type;
    typedef MatrixAccessor<value_type> accessor;

    // Vigra typedefs
    typedef value_type PixelType;

    // convenience typedefs
    typedef MatrixView self;
    typedef MatrixBase<typename T::value_type> base_type;

    //
    // CONSTRUCTORS
    //
    MatrixView() : base_type() {
      m_matrix_data = 0;
    }
    // range check is optional at construction to allow for cases
    // the data is not correctly sized at creation time. See
    // dense_matrix.hpp for a situation where this occurs. KWM
    MatrixView(T& matrix_data, size_t offset_y,
	       size_t offset_x, size_t nrows, size_t ncols,
	       bool do_range_check = true)
      : base_type(offset_y, offset_x, nrows, ncols) {
      m_matrix_data = &matrix_data;
      if (do_range_check) {
	range_check();
	calculate_iterators();
      }
    }
    MatrixView(T& matrix_data, const Rect<size_t>& rect,
	       bool do_range_check = true)
      : base_type(rect) {
      m_matrix_data = &matrix_data;
      if (do_range_check) {
	range_check();
	calculate_iterators();
      }
    }
    MatrixView(T& matrix_data, const Point<size_t>& upper_left,
	       const Point<size_t>& lower_right, bool do_range_check = true)
      : base_type(upper_left, lower_right) {
      m_matrix_data = &matrix_data;
      if (do_range_check) {
	range_check();
	calculate_iterators();
      }
    }
    MatrixView(T& matrix_data, const Point<size_t>& upper_left,
	       const Size<size_t>& size, bool do_range_check = true)
      : base_type(upper_left, size) {
      m_matrix_data = &matrix_data;
      if (do_range_check) {
	range_check();
	calculate_iterators();
      }
    }
    MatrixView(T& matrix_data, const Point<size_t>& upper_left,
	       const Dimensions<size_t>& dim, bool do_range_check = true)
      : base_type(upper_left, dim) {
      m_matrix_data = &matrix_data;
      if (do_range_check) {
	range_check();
	calculate_iterators();
      }
    }
    //
    // COPY CONSTRUCTORS
    //
    MatrixView(const self& other, size_t offset_y,
	       size_t offset_x, size_t nrows, size_t ncols)
      : base_type(offset_y, offset_x, nrows, ncols) {
      m_matrix_data = other.data();
      range_check();
      calculate_iterators();
    }
    MatrixView(const self& other, const Rect<size_t>& rect)
      : base_type(rect) {
      m_matrix_data = other.data();
      range_check();
      calculate_iterators();
    }
    MatrixView(const self& other, const Point<size_t>& upper_left,
	       const Point<size_t>& lower_right)
      : base_type(upper_left, lower_right) {
      m_matrix_data = other.data();
      range_check();
      calculate_iterators();
    }
    MatrixView(const self& other, const Point<size_t>& upper_left,
	       const Size<size_t>& size)
      : base_type(upper_left, size) {
      m_matrix_data = other.data();
      range_check();
      calculate_iterators();
    }
    MatrixView(const self& other, const Point<size_t>& upper_left,
	       const Dimensions<size_t>& dim)
      : base_type(upper_left, dim) {
      m_matrix_data = other.data();
      range_check();
      calculate_iterators();
    }
    //
    //  FUNCTION ACCESS
    //
    value_type get(size_t row, size_t col) const {
      return m_accessor(m_begin + (row * m_matrix_data->stride()) + col);
    }
    void set(size_t row, size_t col, value_type value) {
      m_accessor.set(m_begin + (row * m_matrix_data->stride()) + col, value);
    }

    //
    // Misc
    //
    T* data() const { return m_matrix_data; }
    self parent() { return self(*m_matrix_data, 0, 0, m_matrix_data->nrows(),
				m_matrix_data->ncols()); }
    self& matrix() { return *this; }

    //
    // Iterators
    //
    typedef MatrixViewDetail::RowIterator<self,
      typename T::iterator> row_iterator;
    row_iterator row_begin() {
      return row_iterator(this, m_begin); }
    row_iterator row_end() {
      return row_iterator(this, m_end); }

    typedef MatrixViewDetail::ColIterator<self, typename T::iterator> col_iterator;
    col_iterator col_begin() {
      return col_iterator(this, m_begin); }
    col_iterator col_end() {
      return col_iterator(this, m_begin + ncols()); }

    //
    // Const Iterators
    //
    typedef MatrixViewDetail::ConstRowIterator<const self,
      typename T::const_iterator> const_row_iterator;
    const_row_iterator row_begin() const {
      return const_row_iterator(this, m_const_begin); }
    const_row_iterator row_end() const {
      return const_row_iterator(this, m_const_end); }

    typedef MatrixViewDetail::ConstColIterator<const self,
      typename T::const_iterator> const_col_iterator;
    const_col_iterator col_begin() const {
      return const_col_iterator(this, m_const_begin); }
    const_col_iterator col_end() const {
      return const_col_iterator(this, m_const_begin + ncols()); }


    //
    // 2D iterators
    //
    typedef Gamera::ImageIterator<MatrixView, typename T::iterator> Iterator;
    Iterator upperLeft() { return Iterator(this, m_begin, m_matrix_data->stride()); }
    Iterator lowerRight() { return Iterator(this, m_begin, m_matrix_data->stride())
			      + Diff2D(ncols(), nrows()); }
    typedef Gamera::ConstImageIterator<const MatrixView, typename T::const_iterator> ConstIterator;
    ConstIterator upperLeft() const { return ConstIterator(this, m_const_begin,
							   m_matrix_data->stride()); }
    ConstIterator lowerRight() const {
      return ConstIterator(this, m_const_begin, m_matrix_data->stride()) + Diff2D(ncols(), nrows());
    }

    //
    // Vector iterator
    //
    typedef MatrixViewDetail::VecIterator<self, row_iterator, col_iterator> vec_iterator;
    vec_iterator vec_begin() { return vec_iterator(row_begin()); }
    vec_iterator vec_end() { return vec_iterator(row_end()); }

    typedef MatrixViewDetail::ConstVecIterator<self,
      const_row_iterator, const_col_iterator> const_vec_iterator;
    const_vec_iterator vec_begin() const {
      return const_vec_iterator(row_begin());
    }
    const_vec_iterator vec_end() const {
      return const_vec_iterator(row_end());
    }

    //
    // OPERATOR ACCESS
    //
    typename T::iterator operator[](size_t n) const {
      return m_begin + (n * data()->stride()); }
  protected:
    // redefine the dimensions change function from Rect
    virtual void dimensions_change() {
      range_check();
      calculate_iterators();
    }
    void calculate_iterators() {
      m_begin = m_matrix_data->begin()
        // row offset
        + (m_matrix_data->stride() * offset_y())
        // col offset
        + offset_x();
      m_end = m_matrix_data->begin()
        // row offset
        + (m_matrix_data->stride() * (offset_y() + nrows()))
        // column offset
        + offset_x();
      const T* cmd = static_cast<const T*>(m_matrix_data);
      m_const_begin = cmd->begin()
        // row offset
        + (m_matrix_data->stride() * offset_y())
        // col offset
        + offset_x();
      m_const_end = cmd->begin()
        // row offset
        + (m_matrix_data->stride() * (offset_y() + nrows()))
        // column offset
        + offset_x();
    }
    void range_check() {
      if (offset_y() + nrows() > m_matrix_data->nrows() ||
	  offset_x() + ncols() > m_matrix_data->ncols()) {
	char error[1024];
	sprintf(error, "Matrix view dimensions out of range for data\n");
	sprintf(error, "%s\tnrows %d\n", error, (int)nrows());
	sprintf(error, "%s\toffset_y %d\n", error, (int)offset_y());
	sprintf(error, "%s\tdata nrows %d\n", error, (int)m_matrix_data->nrows());
	sprintf(error, "%s\tncols %d\n", error, (int)ncols());
	sprintf(error, "%s\toffset_x %d\n", error, (int)offset_x());
	sprintf(error, "%s\tdata ncols %d\n", error,(int)m_matrix_data->ncols());
	std::cerr << error << std::endl;
	throw std::range_error(error);
      }
    }
    T* m_matrix_data;
    typename T::iterator m_begin, m_end;
    typename T::const_iterator m_const_begin, m_const_end;
    accessor m_accessor;
  };

}

#endif
