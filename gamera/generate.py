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

from pyplate import *
from os import path
import os
import sys
from distutils.core import Extension
from gamera import paths

global std_import
global plugins_to_ignore

# magic_import and magic_import_setup
#
# This allows us to ignore a list of modules passed into
# magic_import_setup. generate_plugin uses this to prevent
# the loading of C++ modules that may not exist yet during
# the build process.
def magic_import(name, globals_={}, locals_={}, fromlist=[]):
  if fromlist != None and "core" in fromlist:
    fromlist = list(fromlist)
    fromlist.remove("core")

  for x in plugins_to_ignore:
    if name == x:
      return None

  return std_import(name, globals_, locals_, fromlist)
  
def magic_import_setup(ignore):
  global plugins_to_ignore
  global std_import
  plugins_to_ignore = ignore
  # Save the standard __import__ function so we can chain to it
  std_import = __builtins__['__import__']
  # Override the __import__ function with our new one
  __builtins__['__import__'] = magic_import

def restore_import():
  global std_import
  __builtins__['__import__'] = std_import


def generate_plugin(plugin_filename):
  template = Template("""
  [[exec import string]]
  [[exec from os import path]]
  [[exec from enums import *]]
  [[exec from plugin import *]]
  
  [[# Standard headers used in the plugins #]]
  #include <string>
  #include <stdexcept>
  #include \"gameramodule.hpp\"
  #include \"Python.h\"

  [[# include the headers that the module needs #]]
  [[for header in module.cpp_headers]]
    #include \"[[header]]\"
  [[end]]
  
  using namespace Gamera;
  using namespace Gamera::Python;
  
  [[# Generate the plugin path and module name from the filename. #]]
  [[# The module name for our purposes will be prefixed with an underscore #]]
  [[exec plug_path, filename = path.split(__file__)]]
  [[exec module_name = '_' + filename.split('.')[0] ]]

  [[# Declare all of the functions - because this is a C++ file we have to #]]
  [[# declare the functions as C functions so that Python can access them #]]
  extern \"C\" {
    void init[[module_name]](void);
    [[for function in module.functions]]
      static PyObject* call_[[function.__class__.__name__]](PyObject* self, PyObject* args);
    [[end]]
  }

  [[# Create the list of methods for the module - the name of the function #]]
  [[# is derived from the name of the class implementing the function - #]]
  [[# also, the function name is prepended with call_ so that there are no clashes #]]
  [[# with the real plugin functions #]]
  static PyMethodDef [[module_name]]_methods[] = {
    [[for function in module.functions]]
      { \"[[function.__class__.__name__]]\",
        call_[[function.__class__.__name__]], METH_VARARGS },
    [[end]]
    { NULL }
  };

  [[# These hold the types for the various image types defined in gameracore - this #]]
  [[# is for type checking the arguments and creating the return types. See the init #]]
  [[# function for more details. #]]
  static PyTypeObject* image_type;
  static PyTypeObject* subimage_type;
  static PyTypeObject* cc_type;
  static PyTypeObject* data_type;

  [[# Each module can declare several functions so we loop through and generate wrapping #]]
  [[# code for each function #]]
  [[for function in module.functions]]
    [[if not function.self_type is None]]
      [[exec pyarg_format = 'O']]
    [[else]]
      [[exec pyarg_format = '']]
    [[end]]
    static PyObject* call_[[function.__class__.__name__]](PyObject* self, PyObject* args) {
    [[# this holds the self argument - note that the self passed into the function will #]]
    [[# be Null because this functions is not actually bound to an object #]]
    [[if not function.self_type is None]]
      PyObject* real_self;
    [[end]]

    [[# for each argument insert the appropriate conversion code into the string that will #]]
    [[# be passed to PyArg_ParseTuple and create a variable to hold the result. #]]
    [[for x in function.args.list]]
      hello
      [[if isinstance(x, Int) or isinstance(x, Choice)]]
        int [[x.name + '_arg']];
        [[exec pyarg_format = pyarg_format + 'i']]
      [[elif isinstance(x, Float)]]
        double [[x.name + '_arg']];
        [[exec pyarg_format = pyarg_format + 'd']]
      [[elif isinstance(x, String)]]
        char* [[x.name + '_arg']];
        [[exec pyarg_format = pyarg_format + 's']]
      [[elif isinstance(x, ImageType) or isinstance(x, Class)]]
        PyObject* [[x.name + '_arg']];
        [[exec pyarg_format = pyarg_format + 'O']]
      [[else]]
        Something funny happened - [[x.__class__.__name__]]
      [[end]]
    [[end]]

    [[# Create a variable to hold the return value of the plugin function - see #]]
    [[# below for how the Image* is converted to a PyImageObject. #]]
    [[if isinstance(function.return_type, Int)]]
      int return_value = 0;
    [[elif isinstance(function.return_type, Float)]]
      double return_value = 0.0;
    [[elif isinstance(function.return_type, String)]]
      char* return_value = 0;
    [[elif isinstance(function.return_type, ImageType)]]
      Image* return_value = 0;
    [[elif isinstance(function.return_type, Class)]]
      PyObject* return_value = 0;
    [[elif isinstance(function.return_type, ImageInfo)]]
      ImageInfo* return_value = 0;
    [[end]]

    [[# Now that we have all of the arguments and variables for them we can parse #]]
    [[# the argument tuple. Again, there is an assumption that there is at least one #]]
    [[# argument #]]
    [[if pyarg_format != '']]
      if (PyArg_ParseTuple(args, \"[[pyarg_format]]\"
        [[if not function.self_type is None]]
          ,&real_self
        [[end]]
      [[for i in range(len(function.args.list))]]
        ,
        &[[function.args.list[i].name + '_arg']]
      [[end]]
      ) <= 0)
        return 0;\
    [[end]]

    [[# Type check the self argument #]]
    [[if not function.self_type is None]]
      if (!PyObject_TypeCheck(real_self, image_type)) {
        PyErr_SetString(PyExc_TypeError, \"Object is not an image as expected!\");
        return 0;
      }
    [[end]]
    [[# This code goes through each of the image arguments and builds up a list of #]]
    [[# possible image type names. What is passed in is an abstract notion of #]]
    [[# pixel type (which is saved and restored at the end of this process). That #]]
    [[# is converted into strings used for the the enums (in the switch statemnt) #]]
    [[# and for the casting of the pointers to the C++ objects held in the PyObjects. #]]
    [[# Finally, type-checking code is inserted as well #]]      
    [[if not function.self_type is None]]
      [[exec tmp = [] ]]
      [[exec orig_self_types = function.self_type.pixel_types[:] ]]
      [[for type in function.self_type.pixel_types]]
        [[if type == ONEBIT]]
          [[exec tmp.append('OneBitRleImageView')]]
          [[exec tmp.append('RleCc')]]
          [[exec tmp.append('Cc')]]
        [[end]]
        [[exec tmp.append(get_pixel_type_name(type) + 'ImageView')]]
      [[end]]
      [[exec function.self_type.pixel_types = tmp]]
      [[exec function.self_type.name = 'real_self']]
      [[exec images = [function.self_type] ]]
    [[else]]
      [[exec images = [] ]]
    [[end]]      
    [[exec orig_image_types = [] ]]
    [[for x in function.args.list]]
      [[if isinstance(x, ImageType)]]
        [[exec tmp = [] ]]
        [[for type in x.pixel_types]]
          [[if type == ONEBIT]]
            [[exec tmp.append('OneBitRleImageView')]]
            [[exec tmp.append('RleCc')]]
            [[exec tmp.append('Cc')]]
          [[end]]
          [[exec tmp.append(get_pixel_type_name(type) + 'ImageView')]]
        [[end]]
        [[exec orig_image_types.append(x.pixel_types[:])]]
        [[exec x.name = x.name + '_arg']]
        [[exec x.pixel_types = tmp]]
        [[exec images.append(x)]]
        if (!PyObject_TypeCheck([[x.name]], image_type)) {
          PyErr_SetString(PyExc_TypeError, \"Object is not an image as expected!\");
          return 0;
        }
      [[end]]
    [[end]]

    [[def switch(layer, args)]]
      switch(get_image_combination([[images[layer].name]], cc_type)) {
        [[for type in images[layer].pixel_types]]
          [[exec current = '*((' + type + '*)((RectObject*)' + images[layer].name + ')->m_x)']]
          case [[type.upper()]]:
            [[if layer == len(images) - 1]]
              [[if not function.return_type is None]]
                return_value =
              [[end]]
              [[function.__class__.__name__]]
              (
              [[exec tmp_args = args + [current] ]]
              [[exec arg_string = tmp_args[0] ]]
              [[exec if len(function.args.list) > 0: arg_string += ', ']]
              [[exec current_image = 1]]
              [[for i in range(len(function.args.list))]]
                [[if isinstance(function.args.list[i], ImageType)]]
                  [[exec arg_string += tmp_args[current_image] ]]
                  [[exec current_image += 1]]
                [[else]]
                  [[exec arg_string += function.args.list[i].name + '_arg']]
                [[end]]
                [[if i < len(function.args.list) - 1]]
                  [[exec arg_string += ', ']]
                [[end]]
              [[end]]
              [[arg_string]]

              );
            [[else]]
              [[call switch(layer + 1, args + [current])]]
            [[end]]
          break;
        [[end]]
      }
    [[end]]
    try {
    [[if images != [] ]]
      [[call switch(0, [])]]
    [[else]]
      [[if function.return_type != None]]
        return_value =
      [[end]]
      [[function.__class__.__name__]]
      (
      [[exec arg_string = '']]
      [[for i in range(len(function.args.list))]]
        [[exec arg_string += function.args.list[i].name + '_arg']]
        [[if i < len(function.args.list) - 1]]
          [[exec arg_string += ', ']]
        [[end]]
      [[end]]
      [[arg_string]]
      );
    [[end]]
    } catch (std::exception& e) {
      PyErr_SetString(PyExc_RuntimeError, e.what());
      return 0;
    }
    [[if function.return_type == None]]
      Py_INCREF(Py_None);
      return Py_None;
    [[elif isinstance(function.return_type, ImageType)]]
      return create_ImageObject(return_value, image_type, subimage_type, cc_type, data_type);
    [[elif isinstance(function.return_type, String)]]
      return PyString_FromStringAndSize(return_value.c_str(), return_value.size() + 1);
    [[elif isinstance(function.return_type, ImageInfo)]]
      return create_ImageInfoObject(return_value);
    [[else]]
      return return_value;
    [[end]]

    [[if not function.self_type is None]]
      [[exec function.self_type.pixel_types = orig_self_types]]
    [[end]]
    [[for i in range(len(function.args.list))]]
      [[if isinstance(function.args.list[i], ImageType)]]
        [[exec function.args.list[i].pixel_types = orig_image_types[i] ]]
      [[end]]
    [[end]]
    }
  [[end]]


  DL_EXPORT(void) init[[module_name]](void) {
    Py_InitModule(\"[[module_name]]\", [[module_name]]_methods);
    PyObject* mod = PyImport_ImportModule(\"gamera.core\");
    if (mod == 0) {
      printf(\"Could not load gamera.py - falling back to gameracore\\n\");
      mod = PyImport_ImportModule(\"gamera.gameracore\");
      if (mod == 0) {
        PyErr_SetString(PyExc_RuntimeError, \"Unable to load gameracore.\\n\");
        return;
      }
    }
    PyObject* dict = PyModule_GetDict(mod);
    if (dict == 0) {
      PyErr_SetString(PyExc_RuntimeError, \"Unable to get module dictionary\\n\");
      return;
    }
    image_type = (PyTypeObject*)PyDict_GetItemString(dict, \"Image\");
    subimage_type = (PyTypeObject*)PyDict_GetItemString(dict, \"SubImage\");
    cc_type = (PyTypeObject*)PyDict_GetItemString(dict, \"CC\");
    data_type = (PyTypeObject*)PyDict_GetItemString(dict, \"ImageData\");

  }

  """)
  

  plug_path, filename = path.split(plugin_filename)
  module_name = filename.split('.')[0]
  sys.path.append(plug_path)
  ignore = ["_" + module_name] + ["core", "gamera.core", "gameracore"]
  magic_import_setup(ignore)

  import plugin
  plugin_module = __import__(module_name)
  module_name = "_" + module_name
  cpp_filename = path.join(plug_path, module_name + ".cpp")
  output_file = open(cpp_filename, "w")
  output_file = open(cpp_filename, "aw")

  template.execute(output_file, plugin_module.__dict__)
  # add newline to make gcc shut-up about no newline at end of file!
  output_file.write('\n')
  output_file.close()
  restore_import()

  # make the a distutils extension class for this plugin
  cpp_files = [cpp_filename]
  for file in plugin_module.module.cpp_sources:
    cpp_files.append(plug_path + file)
  extra_libraries = ["stdc++"] + plugin_module.module.extra_libraries
  return Extension("gamera.plugins." + module_name, cpp_files,
                   include_dirs=["include", plug_path, "include/plugins"],
                   libraries=extra_libraries)
