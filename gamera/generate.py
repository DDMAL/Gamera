# vi:set tabsize=3:
#
# Copyright (C) 2001, 2002 Ichiro Fujinaga, Michael Droettboom,
#                          and Karl MacMillan
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
import re
from distutils.core import Extension
from distutils.dep_util import newer
from distutils import sysconfig
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

template = Template("""
  [[exec import string]]
  [[exec from os import path]]
  [[exec from enums import *]]
  [[exec from plugin import *]]
  [[exec from util import get_pixel_type_name]]

  [[# Standard headers used in the plugins #]]
  #include <string>
  #include <stdexcept>
  #include \"Python.h\"
  #include <list>
  #include \"gameramodule.hpp\"
  #include \"knnmodule.hpp\"

  [[# include the headers that the module needs #]]
  [[for header in module.cpp_headers]]
    #include \"[[header]]\"
  [[end]]

  using namespace Gamera;
  [[for x in module.cpp_namespaces]]
    using namespace [[x]];
  [[end]]

  [[# Generate the plugin path and module name from the filename. #]]
  [[# The module name for our purposes will be prefixed with an underscore #]]
  [[exec plug_path, filename = path.split(__file__)]]
  [[exec module_name = '_' + filename.split('.')[0] ]]

  [[# Declare all of the functions - because this is a C++ file we have to #]]
  [[# declare the functions as C functions so that Python can access them #]]
  extern \"C\" {
    void init[[module_name]](void);
    [[for function in module.functions]]
      [[if not function.pure_python]]
        static PyObject* call_[[function.__class__.__name__]](PyObject* self, PyObject* args);
      [[end]]
    [[end]]
  }

  [[# Create the list of methods for the module - the name of the function #]]
  [[# is derived from the name of the class implementing the function - #]]
  [[# also, the function name is prepended with call_ so that there are no clashes #]]
  [[# with the real plugin functions #]]
  static PyMethodDef [[module_name]]_methods[] = {
    [[for function in module.functions]]
      [[if not function.pure_python]]
        { \"[[function.__class__.__name__]]\",
          call_[[function.__class__.__name__]], METH_VARARGS },
      [[end]]
    [[end]]
    { NULL }
  };

  [[# Each module can declare several functions so we loop through and generate wrapping #]]
  [[# code for each function #]]
  [[for function in module.functions]]
    [[if not function.pure_python]]
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
        Image* real_self_image;
      [[end]]

      [[# for each argument insert the appropriate conversion code into the string that will #]]
      [[# be passed to PyArg_ParseTuple and create a variable to hold the result. #]]
      [[for x in function.args.list]]
        [[exec x.name = re.sub(\"\s\", \"_\", x.name)]]
        [[if isinstance(x, Int) or isinstance(x, Choice) or isinstance(x, Check)]]
          int [[x.name + '_arg']];
          [[exec pyarg_format = pyarg_format + 'i']]
        [[elif isinstance(x, Float)]]
          double [[x.name + '_arg']];
          [[exec pyarg_format = pyarg_format + 'd']]
        [[elif isinstance(x, String) or isinstance(x, FileSave) or isinstance(x, FileOpen)]]
          char* [[x.name + '_arg']];
          [[exec pyarg_format = pyarg_format + 's']]
        [[elif isinstance(x, Class)]]
          PyObject* [[x.name + '_arg']];
          [[exec pyarg_format = pyarg_format + 'O']]
        [[elif isinstance(x, Region)]]
          PyObject* [[x.name + '_arg']];
          Region* [[x.name + '_regionarg']];
          [[exec pyarg_format = pyarg_format + 'O']]
        [[elif isinstance(x, RegionMap)]]
          PyObject* [[x.name + '_arg']];
          RegionMap* [[x.name + '_regionmaparg']];
          [[exec pyarg_format = pyarg_format + 'O']]
        [[elif isinstance(x, ImageType)]]
          PyObject* [[x.name + '_arg']];
          Image* [[x.name + '_imagearg']];
          [[exec pyarg_format = pyarg_format + 'O']]
        [[elif isinstance(x, ImageList)]]
          PyObject* [[x.name + '_arg']];
          [[exec pyarg_format = pyarg_format + 'O']]
          std::vector<Image*> [[x.name + '_list_arg']];
        [[else]]
          Something funny happened - [[x.__class__.__name__]]
          [[isinstance(x, ImageType)]]
        [[end]]
      [[end]]

      [[# Create a variable to hold the return value of the plugin function - see #]]
      [[# below for how the Image* is converted to a PyImageObject. #]]
      [[if isinstance(function.return_type, Int)]]
        int return_value = 0;
      [[elif isinstance(function.return_type, Float)]]
        double return_value = 0.0;
      [[elif isinstance(function.return_type, String)]]
        [[# changed from char * to std::string  MGD #]]
        std::string return_value; // Let C++ implicitly initialize this.
      [[elif isinstance(function.return_type, ImageType)]]
        Image* return_value = 0;
      [[elif isinstance(function.return_type, Class)]]
        PyObject* return_value = 0;
      [[elif isinstance(function.return_type, Region)]]
        Region* return_value = 0;
      [[elif isinstance(function.return_type, RegionMap)]]
        RegionMap* return_value = 0;
      [[elif isinstance(function.return_type, ImageInfo)]]
        ImageInfo* return_value = 0;
      [[elif isinstance(function.return_type, FloatVector)]]
        FloatVector* return_value = 0;
      [[elif isinstance(function.return_type, IntVector)]]
        IntVector* return_value = 0;
      [[elif isinstance(function.return_type, ImageList)]]
        std::list<Image*>* return_value = 0;
      [[end]]

      [[# Now that we have all of the arguments and variables for them we can parse #]]
      [[# the argument tuple. #]]
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
        if (!is_ImageObject(real_self)) {
          PyErr_SetString(PyExc_TypeError, \"Object is not an image as expected!\");
          return 0;
        }
        real_self_image = ((Image*)((RectObject*)real_self)->m_x);
        image_get_fv(real_self, &real_self_image->features, &real_self_image->features_len);
      [[end]]

      [[for arg in function.args.list]]
        [[if isinstance(arg, ImageList)]]
          if (!PyList_Check([[arg.name + '_arg']])) {
            PyErr_SetString(PyExc_TypeError, \"Expected a list of images.\");
            return 0;
          }
          [[arg.name + '_list_arg']].resize(PyList_GET_SIZE([[arg.name + '_arg']]));
          for (int i=0; i < PyList_GET_SIZE([[arg.name + '_arg']]); ++i) {
            PyObject *element = PyList_GET_ITEM([[arg.name + '_arg']], i);
            if (!is_ImageObject(element)) {
              PyErr_SetString(PyExc_TypeError, \"Expected a list of images.\");
              return 0;
            }
            [[arg.name + '_list_arg']][i] = ((Image*)((RectObject*)element)->m_x);
            image_get_fv(element, &[[arg.name + '_list_arg']][i]->features,
                         &[[arg.name + '_list_arg']][i]->features_len);
          }
          [[exec arg.name += '_list']]  
        [[end]]
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
        [[exec function.self_type.name = 'real_self_image']]
        [[exec function.self_type.pyname = 'real_self']]
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
          [[exec x.pyname = x.name + '_arg']]
          [[exec x.name = x.name + '_imagearg']]
          [[exec x.pixel_types = tmp]]
          [[exec images.append(x)]]
          if (!is_ImageObject([[x.pyname]])) {
            PyErr_SetString(PyExc_TypeError, \"Object is not an image as expected!\");
            return 0;
          }
          [[x.name]] = ((Image*)((RectObject*)[[x.pyname]])->m_x);
          image_get_fv([[x.pyname]], &[[x.name]]->features,
                       &[[x.name]]->features_len);
        [[end]]
      [[end]]

      [[# Extract the other arguments #]]
      [[for x in function.args.list]]
        [[if isinstance(x, RegionMap)]]
          [[x.pyname = x.name + '_arg']]
          [[x.name = x.name + '_regionmaparg']]
          if (!is_RegionMapObject([[x.pyname]])) {
            PyErr_SetString(PyExc_TypeError, \"Object is not a RegionMap.\");
            return 0;
          }
          [[x.name]] = (RegionMap*)((RegionMapObect*)[[x.pyname]])->m_x;
        [[elif isinstance(x, Region)]]
          [[x.pyname = x.name + '_arg']]
          [[x.name = x.name + '_regionarg']]
          if (!is_RegionObject([[x.pyname]])) {
            PyErr_SetString(PyExc_TypeError, \"Object is not a Region.\");
            return 0;
          }
          [[x.name]] = (Region*)((RectObect*)[[x.pyname]])->m_x;
        [[end]]
      [[end]]

      [[def switch(layer, args)]]
        switch(get_image_combination([[images[layer].pyname]])) {
          [[for type in images[layer].pixel_types]]
            [[exec current = '*((' + type + '*)' + images[layer].name + ')']]
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
          default:
            PyErr_SetString(PyExc_TypeError, \"Image types do not match function signature.\");
            return 0;
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
        return create_ImageObject(return_value);
      [[elif isinstance(function.return_type, String)]]
        return PyString_FromStringAndSize(return_value.data(), return_value.size() [[# + 1 doesn't seem to be needed MGD #]]);
      [[elif isinstance(function.return_type, ImageInfo)]]
        return create_ImageInfoObject(return_value);
      [[elif isinstance(function.return_type, Int)]]
        return Py_BuildValue(\"i\", return_value);
      [[elif isinstance(function.return_type, Float)]]
        return Py_BuildValue(\"f\", return_value);
      [[elif isinstance(function.return_type, Region)]]
        PyObject* result = create_RegionObject(*return_value);
        delete return_value;
        return result;
      [[elif isinstance(function.return_type, RegionMap)]]
        PyObject* result = create_RegionMapObject(*return_value);
        delete return_value;
        return result;
      [[elif isinstance(function.return_type, FloatVector) or isinstance(function.return_type, IntVector)]]
         [[# This is pretty expensive, but simple#]]
         PyObject* array_func;
         PyObject* array_module = PyImport_ImportModule(\"array\");
         if (array_module == 0)
           return 0;
         PyObject* array_dict = PyModule_GetDict(array_module);
         if (array_dict == 0)
           return 0;
         array_func = PyDict_GetItemString(array_dict, \"array\");
         if (array_func == 0)
           return 0;
         [[if isinstance(function.return_type, FloatVector)]]
           PyObject* arglist = Py_BuildValue(\"(s)\", \"d\");
         [[else]]
           PyObject* arglist = Py_BuildValue(\"(s)\", \"i\");
         [[end]]
         PyObject* array = PyEval_CallObject(array_func, arglist);
         Py_DECREF(arglist);
         [[# There isn't a way to create an array of a set size!?!?!#]]
         [[# So we are going to append single items until it is the right #]]
         [[# size - the alternative is to build an initilization list, but #]]
         [[# this is probably more efficient. #]]
         array_func = PyObject_GetAttrString(array, \"append\");
         if (array_func == 0)
           return 0;         
         [[if isinstance(function.return_type, FloatVector)]]
           arglist = Py_BuildValue(\"(f)\", 0.0);
         [[else]]
           arglist = Py_BuildValue(\"(i)\", 0);
         [[end]]
         PyObject* result;
         for (size_t i = 0; i < return_value->size(); ++i) {
           result = PyEval_CallObject(array_func, arglist);
           if (result == 0)
             return 0;
           Py_DECREF(result);
         }
         Py_DECREF(array_func);
         Py_DECREF(arglist);
         [[if isinstance(function.return_type, FloatVector)]]
           double* buf;
         [[else]]
           int* buf;
         [[end]]
         int len;
         if (PyObject_AsWriteBuffer(array, (void**)&buf, &len) < 0)
           return 0;
         [[if isinstance(function.return_type, FloatVector)]]
           if (len / sizeof(double) != return_value->size()) {
         [[else]]
           if (len / sizeof(int) != return_value->size()) {
         [[end]]
           Py_DECREF(array);
           PyErr_SetString(PyExc_RuntimeError, \"There was a problem creating the array\");
           return 0;
         }
         for (size_t i = 0; i < return_value->size(); ++i) {
           buf[i] = (*return_value)[i];
         }
         delete return_value;
         return array;
      [[elif isinstance(function.return_type, ImageList)]]
        PyObject* list = PyList_New(return_value->size());
        std::list<Image*>::iterator it = return_value->begin();
        for (size_t i = 0; i < return_value->size(); ++i, ++it) {
          PyList_SET_ITEM(list, i,
            create_ImageObject(*it));
        }
        delete return_value;
        return list;
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
  [[end]]


  DL_EXPORT(void) init[[module_name]](void) {
    Py_InitModule(\"[[module_name]]\", [[module_name]]_methods);
  }

  """)
  
def generate_plugin(plugin_filename, location, compiling_gamera):
  plug_path, filename = path.split(plugin_filename)
  module_name = filename.split('.')[0]
  cpp_filename = path.join(plug_path, "_" + module_name + ".cpp")

  regenerate = 0
  if newer(plugin_filename, cpp_filename) or '-f' in sys.argv:
    regenerate = 1

  sys.path.append(plug_path)

  #import plugin
  plugin_module = __import__(module_name)
  if plugin_module.module.pure_python:
    return None

  # see if any of the header files have changed since last time
  # we compiled
  include_dirs = (["include", plug_path, "include/plugins"] +
                  plugin_module.module.cpp_include_dirs)
  if not compiling_gamera:
     include_dirs.append(sysconfig.get_python_inc() + "\gamera")
  if not regenerate:
    for header in plugin_module.module.cpp_headers:
      found_header = 0
      for include_dir in include_dirs:
        header_filename = path.join(include_dir, header)
        if path.exists(header_filename):
          found_header = 1
          if newer(header_filename, cpp_filename):
            regenerate = 1
            break
          break
      if regenerate:
        break

  if regenerate:
    print "generating wrappers for", module_name, "plugin"
    template.execute_file(cpp_filename, plugin_module.__dict__)
  else:
    print "skipping wrapper generation for", module_name, "plugin (output up-to-date)"

  # make the a distutils extension class for this plugin
  cpp_files = [cpp_filename]
  for file in plugin_module.module.cpp_sources:
    cpp_files.append(file)
  
  extra_libraries = plugin_module.module.extra_libraries
  if '--compiler=mingw32' in sys.argv or not sys.platform == 'win32':
     if "stdc++" not in extra_libraries:
        extra_libraries.append("stdc++")
  compile_args = []
  if sys.platform == 'win32' and not '--compiler=mingw32' in sys.argv:
     compile_args = ["/GR", "/Zi", "/Yd"]
     #compile_args = ["/GR"]
  return Extension(location + "._" + module_name, cpp_files,
                   include_dirs=include_dirs,
                   library_dirs=plugin_module.module.library_dirs,
                   libraries=extra_libraries,
                   extra_compile_args=plugin_module.module.extra_compile_args + compile_args,
                   extra_link_args=plugin_module.module.extra_link_args,
                   define_macros=plugin_module.module.define_macros,
                   extra_objects=plugin_module.module.extra_objects)
