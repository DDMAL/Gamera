/*
 *
 * Copyright (C) 2001-2004
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

#ifndef mgd12032001_draw_hpp
#define mgd12032001_draw_hpp

#include <stack>

template<class T>
inline void _clip_points(T& image, size_t& y1, size_t& x1, size_t& y2, size_t& x2) {
  y1 = std::min(y1, image.nrows());
  y2 = std::min(y2, image.nrows());
  x1 = std::min(x1, image.nrows());
  x2 = std::min(x2, image.nrows());
}

/* 
Po-Han Lin's "Extremely Fast Line Algorithm"

Freely useable in non-commercial applications as long as credits to Po-Han Lin and link to http://www.edepot.com is provided in source code and can been seen in compiled executable. 
*/

template<class T>
void draw_line(T& image, double y1, double x1, double y2, double x2,
	       typename T::value_type value) {
  y1 -= (double)image.ul_y();
  y2 -= (double)image.ul_y();
  x1 -= (double)image.ul_x();
  x2 -= (double)image.ul_x();

  double ul_y = std::max(std::min(y1, double(image.nrows())), 0.0);
  double lr_y = std::max(std::min(y2, double(image.nrows())), 0.0);
  double ul_x = std::max(std::min(x1, double(image.nrows())), 0.0);
  double lr_x = std::max(std::min(x2, double(image.nrows())), 0.0);
  
  // The line doesn't pass through the image, so just skip
  if (lr_y - ul_y == 0.0 && lr_x - ul_x == 0.0)
    return;

  bool y_longer = false;
  double increment_val, end_val;
  double y_len = y2 - y1;
  double x_len = x2 - x1;

  std::cerr << y1 << " " << x1 << " " << y2 << " " << x2 << "\n";
  
  if (y_len > 0) {
    if (y1 < 0) {
      x1 += (-y1 * x_len) / y_len;
      y1 = 0;
    }
    if (y2 > image.nrows()) {
      x2 += ((y2 - image.nrows()) * x_len) / y_len;
      y2 = image.nrows() - 1;
    }
  } else {
    if (y2 < 0) {
      x2 += (-y2 * x_len) / y_len;
      y2 = 0;
    }
    if (y1 > image.nrows()) {
      x1 += ((y1 - image.nrows()) * x_len) / y_len;
      y1 = image.nrows() - 1;
    }
  }
  if (x_len > 0) {
    if (x1 < 0) {
      y1 += (-x1 * y_len) / x_len;
      x1 = 0;
    }
    if (x2 > image.ncols()) {
      y2 += ((x2 - image.ncols()) * y_len) / x_len;
      x2 = image.ncols() - 1;
    }
  } else {
    if (x2 < 0) {
      y2 += (-x2 * y_len) / x_len;
      x2 = 0;
    }
    if (x1 > image.ncols()) {
      y1 += ((x1 - image.ncols()) * y_len) / x_len;
      x1 = image.ncols() - 1;
    }
  }

  std::cerr << y1 << " " << x1 << " " << y2 << " " << x2 << "\n";
  
  double short_len = y2 - y1;
  double long_len = x2 - x1;
  if (abs(short_len) > abs(long_len)) {
    std::swap(short_len, long_len);
    y_longer = true;
  }

  end_val = long_len;
  if (long_len < 0) {
    increment_val = -1;
    long_len = -long_len;
  } else 
    increment_val = 1;

  double dec_inc;
  if (long_len == 0)
    dec_inc = short_len;
  else
    dec_inc = short_len / long_len;

  double j = 0;
  if (y_longer) {
    for (double i = 0; i < end_val; i += increment_val) {
      size_t y = size_t(y1 + i);
      size_t x = size_t(x1 + j);
      image.set(y, x, value);
      j += dec_inc;
    }
  } else {
    for (double i = 0; i < end_val; i += increment_val) {
      size_t y = size_t(y1 + j);
      size_t x = size_t(x1 + i);
      image.set(y, x, value);
      j += dec_inc;
    }
  }
}

template<class T>
void draw_line_points(T& image, Point& a, Point& b,
		      typename T::value_type value, double alpha = 1.0) {
  draw_line(image, a.y(), a.x(), b.y(), b.x(), value, alpha);
}


template<class T>
void draw_hollow_rect(T& image, size_t y1_, size_t x1_, size_t y2_, size_t x2_,
		      typename T::value_type value) {
  size_t x1, x2, y1, y2;
  if (x1_ > x2_)
    x1 = x2_, x2 = x1_;
  else
    x1 = x1_, x2 = x2_;

  if (y1_ > y2_)
    y1 = y2_, y2 = y1_;
  else
    y1 = y1_, y2 = y2_;

  _clip_points(image, y1, x1, y2, x2);

  for (size_t x = x1; x < x2; ++x) {
    image.set(y1, x, value);
    image.set(y2, x, value);
  }

  for (size_t y = y1; y < y2; ++y) {
    image.set(y, x1, value);
    image.set(y, x2, value);
  }
}

template<class T>
void draw_filled_rect(T& image, size_t y1_, size_t x1_, size_t y2_, size_t x2_,
		      typename T::value_type value) {
  size_t x1, x2, y1, y2;
  if (x1_ > x2_)
    x1 = x2_, x2 = x1_;
  else
    x1 = x1_, x2 = x2_;

  if (y1_ > y2_)
    y1 = y2_, y2 = y1_;
  else
    y1 = y1_, y2 = y2_;

  _clip_points(image, y1, x1, y2, x2);

  for (size_t y = y1; y < y2; ++y) 
    for (size_t x = x1; x < x2; ++x)
      image.set(y, x, value);
}

template<class T>
void draw_bezier(T& image, double ya, double xa, 
		 double yb, double xb, 
		 double yc, double xc,
		 typename T::value_type value) {
  double x_dist = abs(xb - xa);
  double y_dist = abs(yb - ya);
  double max = std::max(x_dist, y_dist);
  double step = 1.0 / ((double)max * 2.0);

  double y = ya;
  double x = xa;
  for (double t = 0.0; t <= 1.0; t += step) {
    double new_x = (pow((1 - t), 2) * xa + 
		    2 * t * (1 - t) * xc + 
		    pow(t, 2) * xb);
    double new_y = (pow((1 - t), 2) * ya + 
		    2 * t * (1 - t) * yc + 
		    pow(t, 2) * yb);
    draw_line(image, (size_t)y, (size_t)x, (size_t)new_y, (size_t)new_x, value);
    y = new_y; x = new_x;
  }
}

/* From John R. Shaw's QuickFill code which is based on
   "An Efficient Flood Visit Algorithm" by Anton Treuenfels,
   C/C++ Users Journal Vol 12, No. 8, Aug 1994 */

template<class T>
struct FloodFill {
  typedef std::stack<Point> Stack;

  inline static void travel(T& image, Stack& s,
			    const typename T::value_type& interior, 
			    const typename T::value_type& color,
			    const size_t left, const size_t right,
			    const size_t y) {
    if (left + 1 <= right) {
      typename T::value_type col1, col2;
      for (size_t x = left + 1; x <= right; ++x) {
	col1 = image.get(y, x-1);
	col2 = image.get(y, x);
	if (col1 == interior && col2 != interior) {
	  s.push(Point(x-1, y));
	}
      }
      if (col2 == interior) {
	s.push(Point(right, y));
      }
    }
  }

  static void fill_seeds(T& image, Stack& s, 
			 const typename T::value_type& interior, 
			 const typename T::value_type& color) {
    typedef typename T::value_type pixel_t;
    size_t left, right;
    while (!s.empty()) {
      Point p = s.top();
      s.pop();
      if (image.get(p) == interior) {
	for (right = p.x();
	     right < image.ncols();
	     ++right) {
	  if (image.get(p.y(), right) != interior)
	    break;
	  image.set(p.y(), (size_t)right, color);
	}
	--right;

	long int l = p.x() - 1;
	for (; l >= 0; --l) {
	  if (image.get(p.y(), l) != interior)
	    break;
	  image.set(p.y(), l, color);
	}
	left = (size_t)l + 1;

	if (left != right) {
	  if (p.y() < image.nrows() - 1)
	    travel(image, s, interior, color, left, right, p.y() + 1);
	  if (p.y() > 0)
	    travel(image, s, interior, color, left, right, p.y() - 1);
	} else {
	  if (p.y() < image.nrows() - 1)
	    if (image.get(p.y() + 1, left) != color)
	      s.push(Point(left, p.y() + 1));
	  if (p.y() > 1)
	    if (image.get(p.y() - 1, left) != color)
	      s.push(Point(left, p.y() - 1));
	}
      }
    }
  }
};

template<class T>
void flood_fill(T& image, size_t y, size_t x, const typename T::value_type& color) {
  if (y >= image.nrows() || x >= image.ncols())
    throw std::runtime_error("Coordinate out of range.");
  typename T::value_type interior = image.get(y, x);
  if (color == interior)
    return;
  typename FloodFill<T>::Stack s;
  s.push(Point(x, y));
  FloodFill<T>::fill_seeds(image, s, interior, color);
}

template<class T>
void remove_border(T& image) {
  size_t bottom = image.nrows() - 1;
  size_t right = image.ncols() - 1;
  for (size_t x = 0; x < image.ncols(); ++x) {
    if (image.get(0, x) != 0)
      flood_fill(image, 0, x, white(image));
    if (image.get(bottom, x) != 0)
      flood_fill(image, bottom, x, white(image));
  }
  for (size_t y = 0; y < image.nrows(); ++y) {
    if (image.get(y, 0) != 0)
      flood_fill(image, y, 0, white(image));
    if (image.get(y, right) != 0)
      flood_fill(image, y, right, white(image));
  }
}

template<class T, class U>
void highlight(T& a, const U& b, const typename T::value_type& color) {
  size_t ul_y = std::max(a.ul_y(), b.ul_y());
  size_t ul_x = std::max(a.ul_x(), b.ul_x());
  size_t lr_y = std::min(a.lr_y(), b.lr_y());
  size_t lr_x = std::min(a.lr_x(), b.lr_x());
  
  if (ul_y >= lr_y || ul_x >= lr_x)
    return;
  for (size_t y = ul_y, ya = y-a.ul_y(), yb=y-b.ul_y(); 
       y <= lr_y; ++y, ++ya, ++yb)
    for (size_t x = ul_x, xa = x-a.ul_x(), xb=x-b.ul_x(); 
	 x <= lr_x; ++x, ++xa, ++xb) {
      if (is_black(b.get(yb, xb))) 
	a.set(ya, xa, color);
    }
}

#endif
