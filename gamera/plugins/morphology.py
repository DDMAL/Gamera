#
#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

from gamera.plugin import *
import _morphology

#TODO: Change these to out-of-place

class erode(PluginFunction):
  """
  Morpholgically erodes the image with a 3x3 square structuring element.
  """
  self_type = ImageType([ONEBIT, GREYSCALE, FLOAT])
  doc_examples = [(GREYSCALE,), (ONEBIT,)]
  return_type = ImageType([ONEBIT, GREYSCALE, FLOAT])
  pure_python = True
  def __call__(image):
    return _morphology.erode_dilate(image, 1, 1, 0)
  __call__ = staticmethod(__call__)

class dilate(PluginFunction):
  """
  Morpholgically dilates the image with a 3x3 square structuring element.

  The returned image is of the same size as the input image, which means
  that border pixels are not dilated beyond the image dimensions. If you
  also want the border pixels to be dilated, apply pad_image_ to the input
  image beforehand.

.. _pad_image: utility.html#pad-image
  """
  self_type = ImageType([ONEBIT, GREYSCALE, FLOAT])
  doc_examples = [(GREYSCALE,), (ONEBIT,)]
  return_type = ImageType([ONEBIT, GREYSCALE, FLOAT])
  pure_python = True
  def __call__(image):
    return _morphology.erode_dilate(image, 1, 0, 0)
  __call__ = staticmethod(__call__)

class erode_dilate(PluginFunction):
  """
  Morphologically erodes or dilates the image with a rectangular or
  ocagonal structuring element. For onebit images, this is simply a
  wrapper for erode_with_structure_ or dilate_with_structure_ with
  special cases for the structuring element.

  The returned image is of the same size as the input image, which means
  that border pixels are not dilated beyond the image dimensions. If you
  also want the border pixels to be dilated, apply pad_image_ to the input
  image beforehand.

.. _pad_image: utility.html#pad-image
.. _erode_with_structure: #erode-with-structure
.. _dilate_with_structure: #dilate-with-structure

  *ntimes*
    The number of times to perform the operation.
  *direction*
    dilate (0)
      increase the presence of black
    erode (1)
      decrease the presence of black
  *shape*
    rectangular (0)
      use a 3x3 rectangular morphology operator
    octagonal (1)
      use octagonal morphology operator by alternately using
      a 3x3 cross and a 3x3 square structuring element
  """
  self_type = ImageType([ONEBIT, GREYSCALE, FLOAT])
  args = Args([Int('ntimes', range=(0, 10), default=1), \
               Choice('direction', ['dilate', 'erode']), \
               Choice('shape', ['rectangular', 'octagonal'])])
  return_type = ImageType([ONEBIT, GREYSCALE, FLOAT])
  doc_examples = [(GREYSCALE, 10, 0, 1)]

class despeckle(PluginFunction):
  """
  Removes connected components that are smaller than the given size.

  *size*
    The maximum number of pixels in each connected component that
    will be removed.

  This approach to finding connected components uses a pseudo-recursive
  descent, which gets around the hard limit of ~64k connected components
  per page in ``cc_analysis``.  Unfortunately, this approach is much
  slower as the connected components get large, so *size* should be
  kept relatively small.

  *size* == 1 is a special case and runs much faster, since it does not
  require recursion.
  """
  self_type = ImageType([ONEBIT])
  args = Args([Int('cc_size', range=(1, 100))])
  doc_examples = [(ONEBIT,15)]

class distance_transform(PluginFunction):
  """
  For all black pixels, the distance to the nearest white pixel is calculated.
  In the destination image, all pixels corresponding to black pixels in the
  input image will be assigned the their distance value, all pixels
  corresponding to white pixels will be assigned 0.  The result is returned
  as a Float image.

  *norm*:

    0: use chessboard distance (L-infinity norm)

    1: use Manhattan distance (L1 norm)

    2: use Euclidean distance (L2 norm)
  """
  self_type = ImageType([ONEBIT])
  args = Args([Choice("norm", ['chessboard', 'manhattan', 'euclidean'])])
  return_type = ImageType([FLOAT])
  doc_examples = [(ONEBIT,5),]
  author = u"Ullrich K\u00f6the (wrapped from VIGRA by Michael Droettboom)"


class dilate_with_structure(PluginFunction):
    """
    Performs a binary morphological dilation with the given structuring
    element.

    Note that it is necessary to specify which point in the
    structuring element shall be treated as origin. This allows for
    arbitrary structuring elements. Examples:

    .. code:: Python

      # same as image.dilate()
      structure = Image(Point(0,0), Point(2,2), ONEBIT)
      structure.fill(1)
      image = image.dilate_with_structure(structure, Point(1,1))

      # same as image.erode_dilate(3,0,0)
      structure = Image(Point(0,0), Point(6,6), ONEBIT)
      structure.fill(1)
      image = image.dilate_with_structure(structure, Point(3,3))

    The implementation is straightforward and can be slow for large
    structuring elements. If you know that your structuring element is
    connected and its origin is black, you can set *only_border* to
    ``True``, because in this case only the border pixels in the image
    need to be considered which can speed up the dilation for some
    images (though not for all).

    The returned image is of the same size as the input image, which means
    that border pixels are not dilated beyond the image dimensions. If you
    also want the border pixels to be dilated, apply pad_image_ to the input
    image beforehand.

.. _pad_image: utility.html#pad-image

    References:

      A proof that only the contour pixels need to be dilated for
      connected structuring elements containing their origin is given
      by Luc Vincent in *Morphological Transformations of Binary
      Images with Arbitrary Structuring Elements*, Signal Processing,
      Vol. 22, No. 1, pp. 3-23, January 1991 (see theorem 2.13)
    """
    self_type = ImageType([ONEBIT])
    args = Args([ImageType([ONEBIT],'structuring_element'),
                 Point('origin'),
                 Check('only_border', default=False)])
    return_type = ImageType([ONEBIT])
    author = "Christoph Dalitz"

    def __call__(self, structuring_element, origin, only_border=False):
        return _morphology.dilate_with_structure(self, structuring_element, origin, only_border)

    __call__ = staticmethod(__call__)

class erode_with_structure(PluginFunction):
    """
    Performs a binary morphological erosion with the given structuring
    element.

    Note that it is necessary to specify which point in the
    structuring element shall be treated as origin. This allows for
    arbitrary structuring elements.
    
    Border pixels at which the structuring element extends beyond the
    image dimensions are whitened. In other words the image is padded
    with white pixels before erosion.

    Example:

    .. code:: Python

      # same as image.erode()
      structure = Image(Point(0,0), Point(2,2), ONEBIT)
      structure.fill(1)
      image = image.erode_with_structure(structure, Point(1,1))
    """
    self_type = ImageType([ONEBIT])
    args = Args([ImageType([ONEBIT],'structuring_element'),
                 Point('origin')])
    return_type = ImageType([ONEBIT])
    author = "Christoph Dalitz"

class MorphologyModule(PluginModule):
  cpp_headers = ["morphology.hpp"]
  category = "Morphology"
  functions = [erode_dilate, erode, dilate, despeckle,
               distance_transform, dilate_with_structure, erode_with_structure]
  author = "Michael Droettboom and Karl MacMillan"
  url = "http://gamera.sourceforge.net/"

module = MorphologyModule()

