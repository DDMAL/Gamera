/*
 *
 * Copyright (C) 2001-2004 Ichiro Fujinaga, Michael Droettboom,
 * and Karl MacMillan
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

#ifndef mgd_arithmetic
#define mgd_arithmetic

#include <functional>
#include "gamera.hpp"

template<class T, class FUNCTOR>
typename ImageFactory<T>::view_type* 
arithmetic_combine(T& a, const T& b, const FUNCTOR& functor, bool in_place) {
  if (a.nrows() != b.nrows() || a.ncols() != b.ncols())
    throw std::runtime_error("Images must be the same size.");
  
  typedef typename T::value_type TVALUE;
  typedef typename ImageFactory<T>::view_type VIEW;

  if (in_place) {
    typename T::vec_iterator ia = a.vec_begin();
    typename T::const_vec_iterator ib = b.vec_begin();
    typename choose_accessor<T>::accessor ad = choose_accessor<T>::make_accessor(a);
    for (; ia != a.vec_end(); ++ia, ++ib) {
      ad.set(NumericTraits<TVALUE>::fromPromote
	     (functor(NumericTraits<TVALUE>::Promote(*ia),
		      NumericTraits<TVALUE>::Promote(*ib))),
	     ia);
    }

    // Returning NULL is converted to None by the wrapper mechanism
    return NULL;
  } else {
    typename ImageFactory<T>::data_type* dest_data =
      new typename ImageFactory<T>::data_type(a.size(), a.offset_y(), 
					    a.offset_x());
    VIEW* dest = new VIEW(*dest_data, a);
    typename T::vec_iterator ia = a.vec_begin();
    typename T::const_vec_iterator ib = b.vec_begin();
    typename VIEW::vec_iterator id = dest->vec_begin();
    typename choose_accessor<VIEW>::accessor ad = choose_accessor<VIEW>::make_accessor(*dest);

    // Vigra's combineTwoImages does not clip back to one of the standard
    // Gamera image types, so we have to do this differently ourselves.  MGD

    for (; ia != a.vec_end(); ++ia, ++ib, ++id) {
      ad.set(NumericTraits<TVALUE>::fromPromote
	     (functor(NumericTraits<TVALUE>::Promote(*ia),
		      NumericTraits<TVALUE>::Promote(*ib))),
	     id);
    }
    return dest;
  }
}

template<class T>
typename ImageFactory<T>::view_type* 
add_images(T& a, const T& b, bool in_place) {
  typedef typename T::value_type TVALUE;
  typedef typename NumericTraits<TVALUE>::Promote PROMOTE;
  return arithmetic_combine(a, b, std::plus<PROMOTE>(), in_place);
}

template<class T>
typename ImageFactory<T>::view_type* 
subtract_images(T& a, const T& b, bool in_place) {
  typedef typename T::value_type TVALUE;
  typedef typename NumericTraits<TVALUE>::Promote PROMOTE;
  return arithmetic_combine(a, b, std::minus<PROMOTE>(), in_place);
}

template<class T>
typename ImageFactory<T>::view_type* 
multiply_images(T& a, const T& b, bool in_place) {
  typedef typename T::value_type TVALUE;
  typedef typename NumericTraits<TVALUE>::Promote PROMOTE;
  return arithmetic_combine(a, b, std::multiplies<PROMOTE>(), in_place);
}

template<class T>
typename ImageFactory<T>::view_type* 
divide_images(T& a, const T& b, bool in_place) {
  typedef typename T::value_type TVALUE;
  typedef typename NumericTraits<TVALUE>::Promote PROMOTE;
  return arithmetic_combine(a, b, std::divides<PROMOTE>(), in_place);
}

#endif 
