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

import sys, os, types, os.path, inspect     # Python standard library

# import the classification states
from gameracore import UNCLASSIFIED, AUTOMATIC, HEURISTIC, MANUAL
# import the pixel types
from gameracore import ONEBIT, GREYSCALE, GREY16, RGB, FLOAT
# import the storage types
from gameracore import DENSE, RLE
# import some of the basic types
from gameracore import ImageData, Size, Dimensions, Point, \
     Rect, Region, RegionMap
# import gamera.gameracore for subclassing
import gameracore

#import classify
import paths, util, config  # Gamera-specific

######################################################################

# Reference to the currently active gui
config.add_option_default("__gui", None)

class ImageBase:
   # Stores the categorized set of methods.  Bears no relationship
   # to __methods__
   _methods = {}
   
   def __init__(self):
      self.name = "Untitled"
      self._display = None
      self.last_display = None

   def __del__(self):
      if self._display:
         self._display.close()

   def add_plugin_method(cls, plug, func, category):
      methods = cls._methods
      if func != None:
         setattr(cls, plug.__name__, func)
      if category != None:
         for type in plug.self_type.pixel_types:
            if not methods.has_key(type):
               methods[type] = {}
            start = methods[type]
            for subcategory in category.split('/'):
               if not start.has_key(subcategory):
                  start[subcategory] = {}
               start = start[subcategory]
            start[plug.__name__] = plug

   add_plugin_method = classmethod(add_plugin_method)

   _pixel_type_names = {ONEBIT:     "OneBit",
                        GREYSCALE:  "GreyScale",
                        GREY16:     "Grey16",
                        RGB:        "RGB",
                        FLOAT:      "Float"}
   def _get_pixel_type_name(self):
      return self._pixel_type_names[self.data.pixel_type]
   pixel_type_name = property(_get_pixel_type_name)

   _storage_format_names = {DENSE:  "Dense",
                            RLE:    "RLE"}
   def _get_storage_format_name(self):
      return self._storage_format_names[self.data.storage_format]
   storage_format_name = property(_get_storage_format_name)
   
   _members_for_menu = ('pixel_type_name',
                        'storage_format_name',
                        'ul_x', 'ul_y', 'nrows', 'ncols')
   def members(self):
      return ["%s: %s" % (x, getattr(self, x)) for x in self._members_for_menu]

   # Find all the methods of the instance
   # This is not done by examining the instance, but instead by
   # examining its methods members.  This is so things can be sorted by
   # categories, and have gui argument resolution etc.
   def methods(self):
      methods = {}
      for type in ("All", self.pixel_type_name):
         if self._methods.has_key(type):
            self._methods_sub(methods, self._methods[type])
      return methods

   def _methods_sub(self, dest, source):
     for key, val in source.items():
       if type(val) == type({}):
         if not dest.has_key(key):
           dest[key] = {}
         self._methods_sub(dest[key], val)
       else:
         dest[key] = val

   def methods_flat_category(self, category):
     methods = self.methods()
     start = methods
     for subcategory in category.split('/'):
       if start.has_key(subcategory):
         start = start[subcategory]
       else:
         raise KeyError
     return self._methods_flatten(start)

   def _methods_flatten(self, mat):
     list = []
     for val in mat.values():
       if type(val) == type({}):
         list.extend(self._methods_flatten(val))
       else:
         list.append(val)
     return list

   def load_image(self, filename=None):
      self = load_image(filename)

   def set_display(self, _display):
      self._display = _display

   def set_resolution(self, v):
      self.m.resolution(v)

   def parent(self):
      pass

   def image(self):
      pass

   def size_mbytes(self):
      return self.m_data.mbytes()

   # Displays the image in its own window
   def display(self):
      gui = config.get_option("__gui")
      if gui:
         if self._display:
            self._display.set_image(self, Image.to_string)
         else:
            self.set_display(
               gui.ShowImage(self, self.name,
                             Image.to_string, owner=self))
      self.last_display = "normal"
      return self._display

   # Displays the image in its own window, coloring the connected-
   # component labels
   def display_ccs(self):
      gui = config.get_option("__gui")
      if gui:
         if self._display:
            self._display.set_image(self, Image._cc_mat_to_string)
         else:
            self.set_display(
               gui.ShowImage(self, self.name,
                             Image._cc_mat_to_string, owner=self))
      self.last_display = "ccs"

   # Displays the image in its own window, highlighting the given
   # subimage (or subimages)
   def display_cc(self, cc):
      gui = config.get_option("__gui")
      # If the last thing displayed was something other than a cc
      # do a normal display to clear and refresh the window
      if self.last_display != "cc" or not gui or not self._display:
         self.display()
      # If the last thing displayed was a cc, clear only those ccs
      # This is much faster than clearing the whole image
      if self.last_display == "cc":
         for c in self.cc:
            self._display.clear_highlight_cc(c)
      if not util.is_sequence(cc):
         cc = [cc]
      self.cc = []
      for c in cc:
         if isinstance(c, Cc) or isinstance(c, SubImage):
            self._display.highlight_cc(c, Image.to_string)
            self.cc.append(c)
      # This will adjust the scroll bars so the cc will be visible
      self._display.focus(self.cc)
      self.last_display = "cc"

   def display_children(self):
      if hasattr(self, 'children_images') and self.children_images != []:
         display_multi(self.children_images)

   def color(self):
      if self.classification_state == CLASS_UNCLASSIFIED:
         return None
      elif self.classification_state == CLASS_AUTOMATIC:
         return (198,145,145)
      elif self.classification_state == CLASS_HEURISTIC:
         return (240,230,140)
      elif self.classification_state == CLASS_MANUAL:
         return (180,238,180)

   def unclassified(self):
      return self.classification_state == CLASS_UNCLASSIFIED

   def automatically_classified(self):
      return self.classification_state == CLASS_AUTOMATIC

   def manually_classified(self):
      return self.classification_state == CLASS_MANUAL

   def heuristically_classified(self):
      return self.classification_state == CLASS_HEURISTIC

   def set_id_name_manual(self, id_name):
      if id_name[-1] == ".":
         id_name = id_name[:-1]
      if not util.is_sequence(id_name):
         self.id_name = [id_name]
      else:
         self.id_name = list(id_name)
      self.classification_state = CLASS_MANUAL

   def set_id_name_automatic(self, id_name):
      if id_name[-1] == ".":
         id_name = id_name[:-1]
      if not util.is_sequence(id_name):
         self.id_name = [id_name]
      else:
         self.id_name = list(id_name)
      self.classification_state = CLASS_AUTOMATIC

   def set_id_name_heuristic(self, id_name):
      if id_name[-1] == ".":
         id_name = id_name[:-1]
      if not util.is_sequence(id_name):
         self.id_name = [id_name]
      else:
         self.id_name = list(id_name)
      self.classification_state = CLASS_HEURISTIC

      
class Image(ImageBase, gameracore.Image):

   def __init__(self, page_offset_y, page_offset_x, nrows, ncols,
                pixel_format, storage_type):
      ImageBase.__init__(self)
      gameracore.Image.__init__(self, page_offset_y, page_offset_x,
                                nrows, ncols, pixel_format, storage_type)

######################################################################

class SubImage(ImageBase, gameracore.SubImage):
   def __init__(self, image, offset_y, offset_x, nrows, ncols):
      ImageBase.__init__(self)
      gameracore.SubImage.__init__(self, image, offset_y, offset_x,
                                   nrows, ncols)

######################################################################

class Cc(ImageBase, gameracore.Cc):
   def __init__(self, image, label, offset_y, offset_x, nrows, ncols):
      ImageBase.__init__(self)
      gameracore.Cc.__init__(self, image, label, offset_y, offset_x,
                             nrows, ncols)
   
   # Displays this cc in context
   def display_context(self):
      gui = config.get_option("__gui")
      if not gui:
         return
      image = self.parent()
      if self._display:

         self._display.set_image(image,
                                  Image.to_string)
      else:
         self.set_display(
            gui.ShowImage(image,
                          "Connected Component",
                          Image.to_string,
                          owner=self))
      self._display.highlight_cc(self,
                                 Image.to_string)
      self._display.focus(self)
      self.last_display = "context"


#####################################################################
#
# Copy Utilities
#
# The Python builtin copy utilities break when trying to
# copy Gamera images. These utility functions allow you
# to easily copy the portions of a Gamera image that you
# want with the correct semantics (i.e. things like._display
# are never copied, but they need to be properly initialized.
#
######################################################################

def copy_image_classification(source, dest):
   assert isinstance(source, Image)
   assert isinstance(dest, Image)
   from copy import copy
   if source.features:
      dest.features = type(source.features)()
      for i in source.features:
         dest.features.append(i)
      if hasattr(source.features, 'id'):
         dest.features.id(source.features.id())
   if source.id_name:
      dest.id_name = copy(source.id_name)
   dest.unique_id = source.unique_id
   dest.children_images = []
   dest.action_depth = 0

def copy_image_regions(source, dest):
   assert isinstance(source, Image)
   assert isinstance(dest, Image)
   dest.region_maps = source.region_maps
   dest.region_map = source.region_map
   dest.scaling = source.scaling

def copy_image_misc(source, dest):
   assert isinstance(source, Image)
   assert isinstance(dest, Image)
   dest.name = source.name

def copy_image_display(source, dest):
   assert isinstance(source, Image)
   assert isinstance(dest, Image)
   dest._display = None
   dest.last_display = None

def copy_image_all(source, dest):
   copy_image_classification(source, dest)
   copy_image_regions(source, dest)
   copy_image_misc(source, dest)
   copy_image_display(source, dest)

# this is a convenience function for using in a console
_gamera_initialised = 0
def init_gamera():
   global _gamera_initialised
   if not _gamera_initialised:
      import magic_import
      import plugin
      # Create the default functions for the menu
      for method in [
         plugin.PluginFactory("load_image", None, "File", plugin.ImageType([], "image"), plugin.ImageType(["All"]), (plugin.FileOpen("filename"))),
         plugin.PluginFactory("display", None, "Display", None, plugin.ImageType(["All"]), None),
         plugin.PluginFactory("display_children", None, "Display", None, plugin.ImageType(["All"]), None),
         plugin.PluginFactory("display_ccs", None, "Display", None, plugin.ImageType(["OneBit"]), None),
         plugin.PluginFactory("display_cc", None, "Display", None, plugin.ImageType(["OneBit"]), plugin.ImageType(["OneBit"], "cc"))
         ]:
         method.register()
      magic_import.import_directory(paths.plugins, globals(), locals(), 1)
      gamera_initialised = 1
