#
#
# Copyright (C) 2001 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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
  
class _LogicalBase(PluginFunction):
  self_type = ImageType([ONEBIT])
  args = Args([ImageType([ONEBIT], "mask")])

class and_image(_LogicalBase):
  """Perform the AND operation on two images.

  +---+---+---+
  | \ | 0 | 1 |
  +---+---+---+
  | 0 | 0 | 0 |
  +---+---+---+
  | 1 | 0 | 1 |
  +---+---+---+
  """
  testable = 1
and_image = and_image()

class or_image(_LogicalBase):
  """Perform the OR operation on two images.

  +---+---+---+
  | \ | 0 | 1 |
  +---+---+---+
  | 0 | 0 | 1 |
  +---+---+---+
  | 1 | 1 | 1 |
  +---+---+---+
  """
  testable = 1
or_image = or_image()

class xor_image(_LogicalBase):
  """Perform the XOR operation on two images.

  +---+---+---+
  | \ | 0 | 1 |
  +---+---+---+
  | 0 | 0 | 1 |
  +---+---+---+
  | 1 | 1 | 0 |
  +---+---+---+
  """
  testable = 1
xor_image = xor_image()

class LogicalModule(PluginModule):
  """This module provides methods to perform basic logical (bitwise) operations on images."""
  category = "Logical"
  cpp_headers = ["logical.hpp"]
  cpp_namespaces=["Gamera"]
  functions = [and_image, or_image, xor_image]
  author = "Michael Droettboom and Karl MacMillan"
  url = "http://gamera.dkc.jhu.edu/"

module = LogicalModule()
