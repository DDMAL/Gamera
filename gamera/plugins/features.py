#
# Copyright (C) 2002 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

import array
from gamera.plugin import *
import gamera.util
import _features

class black_area(PluginFunction):
    """The simplest of all feature-generating functions, ``black_area`` simply
returns the number of black pixels.

.. warning: This feature is not scale invariant."""
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=1)
    feature_function = True
    doc_examples = [(ONEBIT,)]

class moments(PluginFunction):
    """Returns *moments* of the image.

The elements of the returned ``FloatVector`` are:

0. center of gravity on *x* axis
1. center of gravity on *y* axis
2. second order moment on *x* axis
3. second order moment on *y* axis
4. first order moment on both axes

The rest of these I'm not so sure about anymore.

These features are scale invariant.
"""
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=9)
    feature_function = True
    doc_examples = [(ONEBIT,)]

class nholes(PluginFunction):
    """Returns the average number of transitions from white to black in each
row or column.

The elements of the returned ``FloatVector`` are:

0. vertical
1. horizontal

These features are scale invariant.
"""
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=2)
    feature_function = True
    doc_examples = [(ONEBIT,)]

class nholes_extended(PluginFunction):
    """Divides the image into four quadrants and then does a
nholes_ analysis on each of those quadrants.

The elements of the returned ``FloatVector`` are:

0 - 3
  vertical ``nholes`` for each of the quadrants in order NW, NE, SW, SE.
4 - 7
  horizonal ``nholes`` for each of the quadrants in order NW, NE, SW, SE.

These features are scale invariant.
"""
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=8)
    feature_function = True
    doc_examples = [(ONEBIT,)]

class volume(PluginFunction):
    """The percentage of black pixels within the rectangular bounding
box of the image.  Result in range (0, 1].

This feature is scale and rotation invariant.
"""
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=1)
    feature_function = True
    doc_examples = [(ONEBIT,)]

class area(PluginFunction):
    """The area of the bounding box (i.e. *nrows* * *ncols*).

.. warning: this feature is not scale invariant.
    """
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=1)
    feature_function = True
    doc_examples = [(ONEBIT,)]

class aspect_ratio(PluginFunction):
    """The aspect ratio of the bounding box (i.e. *ncols* / *nrows*).

This feature is scale invariant.
    """
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=1)
    feature_function = True
    doc_examples = [(ONEBIT,)]

class compactness(PluginFunction):
    """Loosely speaking, compactness is a volume to surface area ratio
of the connected components.  Highly ornate connected components have
a low compactness, whereas a perfect circle has very high compactness.

The result is derived by calling outline_ on the image and then
comparing the number of pixels in the outline to the original.

.. _outline: filter.html#outline

Since this function requires allocation and deallocation of memory, it
is relatively slow.  However, it has proven to be a very useful feature
in many cases.

This feature is relatively scale and rotation invariant, though as the
image scales, the pixel size relative to the image size diminishes.  This
is currently not corrected for.
"""
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=1)
    feature_function = True
    doc_examples = [(ONEBIT,)]
    
class volume16regions(PluginFunction):
    """Divides the image into a 4 x 4 grid of 16 regions and calculates
the volume within each."""
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=16)
    feature_function = True
    doc_examples = [(ONEBIT,)]

class volume64regions(PluginFunction):
    """Divides the image into a 8 x 8 grid of 64 regions and calculates
the volume within each."""
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=64)
    feature_function = True
    doc_examples = [(ONEBIT,)]

class zernike_moments(PluginFunction):
    """I can't say I understand much about Zernike moments, except that
they are well known for all kinds of invariance, and are often detailed
enough to reconstruct many shapes in a reasonable way.

A. Khotanzad and Y. Hong. Invariant image recognition by Zernike moments.
*IEEE Transactions on Pattern Analysis and Machine Intelligence*, 12(5), 1990.
"""
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=26)
    feature_function = True
    doc_examples = [(ONEBIT,)]

class skeleton_features(PluginFunction):
    """Generates a number of features based on the skeleton of an image.
First, the image in skeletonized using the Lee and Chen algorithm, which
guarantees that the pixels of the resulting skeleton are never more than
4-connected.  Then, this skeleton is analysed for a number of properties:

0. Number of X joints (4-connected pixels)
1. Number of T joints (3-connected pixels)
2. Average bumber of bend points (pixels which do not form a horizontal or
   vertical line with its neighbors)
3. Number of end points (1-connected pixels)
4. Number of *x*-axis crossings
5. Number of *y*-axis crossings
"""
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=6)
    feature_function = True

class top_bottom(PluginFunction):
    """Features useful only for segmentation-free analysis.  Currently, the
first feature is the first row containing a black pixel, and the second feature
is the last row containing a black pixel."""
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=2)
    feature_function = True
    doc_examples = [(ONEBIT,)]

class generate_features(PluginFunction):
    """Generates features for the image by calling a number of feature functions
and storing the results in the image's ``features`` member variable (a Python ``array``).

*features*
  Optional.  A list of feature function names.  If not given, the previously
  set feature functions will be used.  If none were previously given,
  all available feature functions will be used.  Using all feature functions
  can also be forced by passing ``'all'``.

.. warning: For efficiency, if the given feature functions
  match those that have been already generated for the image, the
  features are *not* recalculated.  If you want to force recalculation,
  pass the optional argument ``force=True``."""
    category = "Utility"
    pure_python = True
    self_type = ImageType([ONEBIT])
    args = Args([Class('features', list), Check('force')])
    return_type = None
    cache = {}
    def __call__(self, features=None, force=False):
      if features is None:
         features = self.get_feature_functions()
      if self.feature_functions == features and not force:
         return
      self.feature_functions = features
      features, num_features = features
      if len(self.features) != num_features:
          if not generate_features.cache.has_key(num_features):
              generate_features.cache[num_features] = [0] * num_features
          self.features = array.array('d', generate_features.cache[num_features])
      offset = 0
      for name, function in features:
          function.__call__(self, offset)
          offset += function.return_type.length
    __call__ = staticmethod(__call__)

class FeaturesModule(PluginModule):
    category = "Features"
    cpp_headers=["features.hpp"]
    cpp_namespaces = ["Gamera"]
    functions = [black_area, moments, nholes,
                 nholes_extended, volume, area,
                 aspect_ratio, compactness,
                 volume16regions, volume64regions,
                 generate_features, zernike_moments,
                 skeleton_features, top_bottom]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"
module = FeaturesModule()

def get_features_length(features):
    """Given a list of feature functions return the number
of features that will be generated. This function is
necessary because each features 'function' can return
multiple individual float values."""
    from gamera import core
    ff = core.ImageBase.get_feature_functions(features)
    return ff[1]

def generate_features_list(list, features='all'):
   """Generate features on a list of images.

*features*
   Follows the same rules as for generate_features_.
   """
   from gamera import core, util
   ff = core.Image.get_feature_functions(features)
   progress = util.ProgressFactory("Generating features...", len(list) / 10)
   try:
      for i, glyph in enumerate(list):
         glyph.generate_features(ff)
         if i % 10 == 0:
             progress.step()
   finally:
       progress.kill()
