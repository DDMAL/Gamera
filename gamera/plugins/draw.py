#
#
# Copyright (C) 2001-2004 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#

from gamera.plugin import *
import _draw
try:
  from gamera.core import RGBPixel
except:
  def RGBPixel(*args):
    pass

class draw_line(PluginFunction):
  """Draws a straight line between two points.

The coordinates can be specified either by four integers or two Points:

  *y1*:
    Starting *y* coordinate.
  *x1*:
    Starting *x* coordinate.
  *y2*:
    Ending *y* coordinate.
  *x2*:
    Ending *x* coordinate.

**or**

  *a*:
    The start ``Point``.
  *b*:
    The end ``Point``.

*value*:
  The pixel value to set for the line.

Based on Po-Han Lin's "Extremely Fast Line Algorithm", which is based
on the classical Breshenham's algorithm.

Freely useable in non-commercial applications as long as credits to
Po-Han Lin and link to http://www.edepot.com is provided in source
code and can been seen in compiled executable.
"""
  self_type = ImageType(ALL)
  args = Args([Float("y1"), Float("x1"), Float("y2"), Float("x2"), Pixel("value")])
  authors = "Michael Droettboom based on Po-Han Lin's Extremely Fast Line Algorithm"

  def __call__(self, *args):
    if len(args) == 5:
      return _draw.draw_line(self, *args)
    elif len(args) == 3:
      try:
        a = args[0]
        b = args[1]
        value = args[2]
        return _draw.draw_line(self, a.y, a.x, b.y, b.x, value)
      except KeyError, AttributeError:
        pass
    raise ValueError("Arguments are incorrect.")
  __call__ = staticmethod(__call__)

  def __doc_example1__():
    from random import randint
    from gamera.core import Image
    image = Image(0, 0, 100, 100, RGB, DENSE)
    for i in range(10):
      image.draw_line(randint(0, 100), randint(0, 100),
                      randint(0, 100), randint(0, 100),
                      RGBPixel(randint(0, 255), randint(0,255), randint(0, 255)))
    return image
  doc_examples = [(__doc_example1__,)]

class draw_hollow_rect(PluginFunction):
  """Draws a hollow rectangle.

The coordinates can be specified either by four integers or two Points:

  *y1*:
    Starting *y* coordinate.
  *x1*:
    Starting *x* coordinate.
  *y2*:
    Ending *y* coordinate.
  *x2*:
    Ending *x* coordinate.

**or**

  *a*:
    The start ``Point``.
  *b*:
    The end ``Point``.

*value*:
  The pixel value to set for the lines.
"""
  self_type = ImageType(ALL)
  args = Args([Int("y1"), Int("x1"), Int("y2"), Int("x2"), Pixel("value")])
  doc_examples = [(ONEBIT, 5, 5, 20, 25, 1), (RGB, 5, 5, 20, 25, RGBPixel(255, 0, 0))]

  def __call__(self, *args):
    if len(args) == 5:
      return _draw.draw_hollow_rect(self, *args)
    elif len(args) == 3:
      try:
        a = args[0]
        b = args[1]
        value = args[2]
        return _draw.draw_hollow_rect(self, a.y, a.x, b.y, b.x, value)
      except KeyError, AttributeError:
        pass
    raise ValueError("Arguments are incorrect.")
  __call__ = staticmethod(__call__)

  def __doc_example1__():
    from random import randint
    from gamera.core import Image
    image = Image(0, 0, 100, 100, RGB, DENSE)
    for i in range(10):
      image.draw_hollow_rect(randint(0, 100), randint(0, 100),
                             randint(0, 100), randint(0, 100),
                             RGBPixel(randint(0, 255), randint(0,255), randint(0, 255)))
    return image
  doc_examples = [(__doc_example1__,)]

class draw_filled_rect(PluginFunction):
  """Draws a filled rectangle.

The coordinates can be specified either by four integers or two Points:

  *y1*:
    Starting *y* coordinate.
  *x1*:
    Starting *x* coordinate.
  *y2*:
    Ending *y* coordinate.
  *x2*:
    Ending *x* coordinate.

**or**

  *a*:
    The start ``Point``.
  *b*:
    The end ``Point``.

*value*:
  The pixel value to set for the rectangle.
"""
  self_type = ImageType(ALL)
  args = Args([Int("y1"), Int("x1"), Int("y2"), Int("x2"), Pixel("value")])
  doc_examples = [(ONEBIT, 5, 5, 20, 25, 1), (RGB, 5, 5, 20, 25, RGBPixel(255, 0, 0))]

  def __call__(self, *args):
    if len(args) == 5:
      return _draw.draw_filled_rect(self, *args)
    elif len(args) == 3:
      try:
        a = args[0]
        b = args[1]
        value = args[2]
        return _draw.draw_filled_rect(self, a.y, a.x, b.y, b.x, value)
      except KeyError, AttributeError:
        pass
    raise ValueError("Arguments are incorrect.")
  __call__ = staticmethod(__call__)

  def __doc_example1__():
    from random import randint
    from gamera.core import Image
    image = Image(0, 0, 100, 100, RGB, DENSE)
    for i in range(10):
      image.draw_filled_rect(randint(0, 100), randint(0, 100),
                             randint(0, 100), randint(0, 100),
                             RGBPixel(randint(0, 255), randint(0,255), randint(0, 255)))
    return image
  doc_examples = [(__doc_example1__,)]

class draw_bezier(PluginFunction):
  """Draws a cubic bezier curve

The coordinates can be specified either by six integers or three Points:

  *start_y*:
    Starting *y* coordinate.
  *start_x*:
    Starting *x* coordinate.
  *c1_y*:
    Control point 1 *y* coordinate.
  *c1_x*:
    Control point 1 *x* coordinate.
  *c2_y*
    Control point 2 *y* coordinate.
  *c2_x*
    Control point 2 *x* coordinate.
  *end_y*
    Ending *y* coordinate.
  *end_x*
    Ending *x* coordinate.

**or**

  *start*:
    The start ``Point``.
  *c1*:
    The control point associated with the start point.
  *c2*
    The control point associated with the end point.
  *end*
    The end ``Point``.

*value*:
  The pixel value to set for the curve.
"""
  self_type = ImageType(ALL)
  args = Args([Float("start_y"), Float("start_x"), Float("c1_y"), Float("c1_x"),
               Float("c2_y"), Float("c2_x"), Float("end_y"), Float("end_x"),
               Pixel("value")])

  def __call__(self, *args):
    if len(args) == 9:
      return _draw.draw_bezier(self, *args)
    elif len(args) == 5:
      try:
        a = args[0]
        b = args[1]
        c = args[2]
        d = args[3]
        value = args[4]
        return _draw.draw_bezier(self, a.y, a.x, b.y, b.x,
                                 c.y, c.x, d.y, d.x, value)
      except KeyError, AttributeError:
        pass
      raise ValueError("Arguments are incorrect.")
  __call__ = staticmethod(__call__)

  def __doc_example1__():
    from random import randint
    from gamera.core import Image
    image = Image(0, 0, 100, 100, RGB, DENSE)
    for i in range(10):
      image.draw_bezier(randint(0, 100), randint(0, 100),
                        randint(0, 100), randint(0, 100),
                        randint(0, 100), randint(0, 100),
                        randint(0, 100), randint(0, 100),
                        RGBPixel(randint(0, 255), randint(0,255), randint(0, 255)))
    return image
  doc_examples = [(__doc_example1__,)]

class flood_fill(PluginFunction):
  """Flood fills from the given point using the given color.  This is similar
to the "bucket" tool found in many paint programs.

The coordinates can be specified either by two integers or one Point:

  *y*:
    Starting *y* coordinate.
  *x*:
    Starting *x* coordinate.

**or**

  *a*:
    The start ``Point``.
  *b*:
    The end ``Point``.

*color*:
  The pixel value to set for the rectangle.
"""
  self_type = ImageType([GREYSCALE, FLOAT, ONEBIT, RGB])
  args = Args([Int("y"), Int("x"), Pixel("color")])
  doc_examples = [(ONEBIT, 58, 10, 0)]

  def __call__(self, *args):
    if len(args) == 3:
      return _draw.flood_fill(self, *args)
    elif len(args) == 2:
      try:
        a = args[0]
        value = args[1]
        return _draw.flood_fill(self, a.y, a.x, value)
      except KeyError, AttributeError:
        pass
    raise ValueError("Arguments are incorrect.")
  __call__ = staticmethod(__call__)

class remove_border(PluginFunction):
  """This is a special case of the flood_fill algorithm that is designed to
remove dark borders produced by photocopiers or flatbed scanners around the
border of the image."""
  self_type = ImageType([ONEBIT])

class highlight(PluginFunction):
  """Highlights a connected component on a given image using the given color.
Self must be an RGB image (usually the original image.)

*cc*
   A one-bit connected component from the image

*color*
   An RGBPixel color value used to color the *cc*."""
  self_type = ImageType([RGB])
  args = Args([ImageType([ONEBIT], "cc"), Pixel("color")])

class DrawModule(PluginModule):
  cpp_headers = ["draw.hpp"]
  cpp_namespaces = ["Gamera"]
  category = "Draw"
  functions = [draw_line, draw_bezier,
               draw_hollow_rect, draw_filled_rect, flood_fill,
               remove_border, highlight]
  author = "Michael Droettboom"
  url = "http://gamera.dkc.jhu.edu/"

module = DrawModule()
