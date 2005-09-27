/*
 *
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

#ifndef __image_algorithm_hh__
#define __image_algorithm_hh__

#include <algorithm>
#include <vector>

#include "gamera.hpp"

/*
  Image Algorithm

  This file contains a variety of utility algorithms for Gamera matrices.

  Author
  ------
  Karl MacMillan karlmac@peabody.jhu.edu

  History
  -------
  - Started 6/12/01

*/

namespace Gamera {

  // Print a image to the console
  template<class T>
  void print_image(const T& image) {
    typename T::const_row_iterator i = image.row_begin();
    typename T::const_row_iterator::iterator j;
    std::cout << "[" << std::endl;
    for (; i != image.row_end(); i++) {
      j = i.begin();
      for (; j != i.end(); j++) {
	std::cout << *j << " ";
      }
      std::cout << std::endl;
    }
    std::cout << "]" << std::endl;
  }
};

#endif
