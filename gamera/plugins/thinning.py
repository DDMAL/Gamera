# -*- coding: utf-8 -*-
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

class thin_zs(PluginFunction):
    """Thins (skeletonizes) a ONEBIT image using the Zhang and Suen algorithm

T. Y. Zhang and C. Y. Suen. 1984.
A Fast Parallel Algorithm for Thinning Digital Patterns.,
*Communications of ACM*, 2(3).
  
R. C. Gonzalez and P. Wintz. 1987
*Digital Image Processing.*,
2. edition. 398-402. 
"""
    self_type = ImageType([ONEBIT])
    return_type = ImageType([ONEBIT])
    doc_examples = [(ONEBIT,)]

class thin_hs(PluginFunction):
    """Thins (skeletonizes) a ONEBIT image using the Haralick and Shapiro algorithm

This algorithm has the interesting property that all pixels are never more than
4-connected.

R. M. Haralick and L. G. Shapiro. 1992.
*Computer and Robot Vision*,
Vol. 1, Chapter 5 (especially 5.10.1).
Reading, MA: Addison-Wesley.
"""
    self_type = ImageType([ONEBIT])
    return_type = ImageType([ONEBIT])
    doc_examples = [(ONEBIT,)]

class thin_lc(PluginFunction):
    """Thins (skeletonizes) a ONEBIT image using the Lee and Chen algorithm.

H.-J. Lee and B. Chen. 1992.
Recognition of handwritten chinese characters via short
line segments. *Pattern Recognition*. 25(5) 543-552.
"""
    self_type = ImageType([ONEBIT])
    return_type = ImageType([ONEBIT])
    doc_examples = [(ONEBIT,)]

class ThinningModule(PluginModule):
    category = "Filter/Thinning"
    functions = [thin_zs, thin_hs, thin_lc]
    cpp_headers = ["thinning.hpp"]
    cpp_namespaces = ["Gamera"]
    author = "Michael Droettboom and Karl MacMillan (based on code by Øivind Due Trier and Qian Huang)"
    url = "http://gamera.dkc.jhu.edu/"
module = ThinningModule()
