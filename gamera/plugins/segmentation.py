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

from gamera.plugin import *
import _segmentation

class cc_analysis(PluginFunction):
    """Performs connected component analysis on the image.

This algorithm assumes 8-connected components, meaning any two pixels are
considered "connected" if they are adjacent in any direction, including
diagonally.

The original image will have all of it's pixels "labeled" with a number
representing each connected component.  This is so the connected components
can share data with their source image and makes things much more efficient.

Returns a list of ccs found in the image.  Since all the CC's share the same
data with the original image, changing the CC's will affect the original.  If
you do not want this behavior, use the image_copy_ function on each of the CCs::

   ccs = [x.image_copy() for x in ccs]

"""
    self_type = ImageType([ONEBIT])
    return_type = ImageList("ccs")
    doc_examples = [(ONEBIT,)]

class cc_and_cluster(PluginFunction):
    """Performs connected component analysis using cc_analysis_ and then
clusters the CC's according to their similarity.

TODO: We need some more detailed documentation here."""
    pure_python = True
    self_type = ImageType([ONEBIT])
    args = Args([Float('ratio', default = 1.0), Int('distance', default=2)])
    return_type = ImageList("ccs")
    def __call__(image, ratio = 1.0, distance = 2):
        from gamera import cluster
        cc = image.cc_analysis()
        return cluster.cluster(cc, ratio, distance)
    __call__ = staticmethod(__call__)

class splitx(PluginFunction):
    """Splits an image vertically.

The split point is determined automatically
by finding a valley in the projections near the center of the image."""
    self_type = ImageType([ONEBIT])
    args = Args([Float("center")])
    return_type = ImageList("splits")
    doc_examples = [(ONEBIT,)]
    def __call__(self, center=0.5):
        return _segmentation.splitx(self, center)
    __call__ = staticmethod(__call__)

class splitx_max(PluginFunction):
    """Splits an image vertically.

The split point is determined automatically
by finding a peak in the projections near the center of the image."""
    self_type = ImageType([ONEBIT])
    args = Args([Float("center")])
    return_type = ImageList("splits")
    doc_examples = [(ONEBIT,)]
    def __call__(self, center=0.5):
        return _segmentation.splitx_max(self, center)
    __call__ = staticmethod(__call__)

class splity(PluginFunction):
    """Splits an image horizontally.

The split point is determined automatically
by finding a valley in the projections near the center of the image."""
    self_type = ImageType([ONEBIT])
    args = Args([Float("center")])
    return_type = ImageList("splits")
    doc_examples = [(ONEBIT,)]
    def __call__(self, center=0.5):
        return _segmentation.splity(self, center)
    __call__ = staticmethod(__call__)

class splitx_base(PluginFunction):
    pure_python = 1
    self_type = ImageType([ONEBIT])
    return_type = ImageList("splits")
    doc_examples = [(ONEBIT,)]
    
class splitx_left(splitx_base):
    """Splits an image vertically.

The split point is determined automatically
by finding a valley in the projections near the left of the image."""
    _center = 0.25
    def __call__(self):
        return self.splitx(0.25)
    __call__ = staticmethod(__call__)

class splitx_right(splitx_base):
    """Splits an image vertically.

The split point is determined automatically
by finding a valley in the projections near the right of the image."""
    _center = 0.75
    def __call__(self):
        return self.splitx(0.75)
    __call__ = staticmethod(__call__)

class splity_base(PluginFunction):
    pure_python = 1
    self_type = ImageType([ONEBIT])
    return_type = ImageList("splits")
    doc_examples = [(ONEBIT,)]
    
class splity_top(splity_base):
    """Splits an image horizontally.

The split point is determined automatically
by finding a valley in the projections near the top of the image."""
    _center = 0.25
    def __call__(self):
        return self.splity(0.25)
    __call__ = staticmethod(__call__)

class splity_bottom(splity_base):
    """Splits an image horizontally.

The split point is determined automatically
by finding a valley in the projections near the bottom of the image."""
    _center = 0.75
    def __call__(self):
        return self.splity(0.75)
    __call__ = staticmethod(__call__)

# connected-component filters

def filter_wide(ccs, max_width):
    tmp = []
    for x in ccs:
        if x.ncols > max_width:
            x.fill_white()
        else:
            tmp.append(x)
    return tmp

def filter_narrow(ccs, min_width):
    tmp = []
    for x in ccs:
        if x.ncols < min_width:
            x.fill_white()
        else:
            tmp.append(x)
    return tmp

def filter_tall(ccs, max_height):
    tmp = []
    for x in ccs:
        if x.nrows > max_height:
            x.fill_white()
        else:
            tmp.append(x)
    return tmp

def filter_short(ccs, min_height):
    tmp = []
    for x in ccs:
        if x.nrows < min_height:
            x.fill_white()
        else:
            tmp.append(x)
    return tmp

def filter_small(ccs, min_size):
    tmp = []
    for x in ccs:
        if x.nrows < min_size or x.ncols < min_size:
            x.fill_white()
        else:
            tmp.append(x)
    return tmp

def filter_large(ccs, max_size):
    tmp = []
    for x in ccs:
        if x.nrows > max_size or x.ncols > max_size:
            x.fill_white()
        else:
            tmp.append(x)
    return tmp

def filter_black_area_small(ccs, min_size):
    tmp = []
    for x in ccs:
        if x.black_area()[0] < min_size:
            x.fill_white()
        else:
            tmp.append(x)
    return tmp

def filter_black_area_large(ccs, max_size):
    tmp = []
    for x in ccs:
        if x.black_area()[0] > max_size:
            x.fill_white()
        else:
            tmp.append(x)
    return tmp

class SegmentationModule(PluginModule):
    category = "Segmentation"
    cpp_headers=["segmentation.hpp"]
    cpp_namespaces = ["Gamera"]
    functions = [cc_analysis, cc_and_cluster, splitx, splity,
                 splitx_left, splitx_right, splity_top, splity_bottom,
                 splitx_max]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"

module = SegmentationModule()
