from pyplate import *
import plugin
import threshold

if __name__ == '__main__':
  combinations = (('OneBit', 'Float', 'GreyScale'),
                  ('GreyScale', 'RGB'))
  
  template = Template("""
  [[exec import string]]
  #include \"gameramodule.hpp\"
  [[for header in module.cpp_headers]]
    #include \"[[header]]\"
  [[end]]
  using namespace Gamera;
  using namespace Gamera::Python;
  
  [[# The module name is prepended with an underscore #]]
  [[exec module_name = "_" + __file__.split(".")[0]]]
  extern \"C\" {
    void init[[module_name]](void);
    [[for function in module.functions]]
      static PyObject* [[function.__class__.__name__]](PyObject* self, PyObject* args);
    [[end]]
  }

  static PyMethodDef [[module_name]]_methods[] = {
    [[for function in module.functions]]
      { \"[[function.__class__.__name__]]\",
        [[function.__class__.__name__]], METH_VARARGS },
    [[end]]
    { NULL }
  };

  static PyTypeObject* image_type;
  static PyTypeObject* subimage_type;
  static PyTypeObject* cc_type;
  static PyTypeObject* data_type;

  [[for function in module.functions]]
    [[exec pyarg_format = 'O']]
    static PyObject* [[function.__class__.__name__]](PyObject* self, PyObject* args) {
      PyObject* real_self;
      [[for x in function.args.list]]
        [[if isinstance(x, Int)]]
          int [[x.name]];
          [[exec pyarg_format = pyarg_format + 'i']]
        [[elif isinstance(x, Float)]]
          double [[x.name]];
          [[exec pyarg_format = pyarg_format + 'd']]
        [[elif isinstance(x, String)]]
          char* [[x.name]];
          [[exec pyarg_format = pyarg_format + 'i']]
        [[elif isinstance(x, ImageType)]]
          PyObject* [[x.name]];
          [[exec pyarg_format = pyarg_format + 'O']]
        [[end]]
      [[end]]

      [[if isinstance(function.return_type, Int)]]
        int return_value;
      [[elif isinstance(function.return_type, Float)]]
        double return_value;
      [[elif isinstance(function.return_type, String)]]
        char* return_value;
      [[elif isinstance(function.return_type, ImageType)]]
        ImageBase* return_value;
      [[end]]

      if (PyArg_ParseTuple(args, [[pyarg_format]],
      [[for i in range(len(function.args.list))]]
        [[if i != 0]]
          ,
        [[end]]
        &[[function.args.list[i].name]]
      [[end]]
      ) < == 0)
        return 0;

      if (!PyObject_TypeCheck(real_self, image_type)) {
        PyErr_SetString(PyExc_TypeError, \"Object is not an image as expected!\");
        return 0;
      }
      [[exec tmp = [] ]]
      [[for type in function.self_type.pixel_types]]
        [[if type == 'OneBit']]
          [[exec tmp.append('OneBitRleImageView')]]
          [[exec tmp.append('RleCC')]]
          [[exec tmp.append('CC')]]
        [[end]]
        [[exec tmp.append(type + 'ImageView')]]
      [[end]]
      [[exec function.self_type.pixel_types = tmp]]
      [[exec images = [function.self_type] ]]
      [[for x in function.args.list]]
        [[if isinstance(x, ImageType)]]
          [[exec tmp = [] ]]
          [[for type in x.pixel_types]]
            [[if type == 'OneBit']]
              [[exec tmp.append('OneBitRleImageView')]]
              [[exec tmp.append('RleCC')]]
              [[exec tmp.append('CC')]]
            [[end]]
            [[exec tmp.append(type + 'ImageView')]]
          [[end]]
          [[exec x.pixel_types = tmp]]
          [[exec images.append(x)]]
          if (!PyObject_TypeCheck([[x.name]], image_type)) {
            PyErr_SetString(PyExc_TypeError, \"Object is not an image as expected!\");
            return 0;
          }
        [[end]]
      [[end]]

      [[def switch(layer, args)]]
        switch(get_image_combination([[images[layer].name]])) {
          [[for type in images[layer].pixel_types]]
            [[exec current = '*((' + type + '*)((RectObject*)' + images[layer].name + ')->m_x)']]
            case [[type.upper()]]:
              [[if layer == len(images) - 1]]
                [[if function.return_type != None]]
                  return_value =
                [[end]]
                [[function.__class__.__name__]]
                (
                [[exec tmp_args = args + [current]]]
                [[exec arg_string = tmp_args[0] + ', ']]
                [[exec current_image = 1]]
                [[for i in range(len(function.args.list))]]
                  [[if isinstance(function.args.list[i], ImageType)]]
                    [[exec arg_string += tmp_args[current_image]]]
                    [[exec current_image += 1]]
                  [[else]]
                    [[exec arg_string += function.args.list[i].name]]
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
    [[call switch(0, [])]]

    [[if function.return_type == None]]
      Py_INCREF(Py_None);
      return Py_None;
    [[elif isinstance(function.return_type, ImageType)]]
      return create_ImageObject(return_value, image_type, subimage_type, cc_type, data_type);
    [[else]]
      return return_value;
    [[end]]
    }
  [[end]]


  DL_EXPORT(void) init[[module_name]](void) {
    Py_InitModule(\"[[module_name]]\",
      [[module_name]]_methods);
    PyObject* mod = PyImportModule(\"gamera\");
    if (mod == 0) {
      printf(\"Could not load gamera.py - falling back to gameracore\n\");
      mod = PyImportModule(\"gameracore\");
      if (mod == 0) {
        PyErr_SetString(PyExc_RuntimeError, \"Unable to load gameracore.\");
        return;
      }
    }
    PyObject* dict = PyModule_GetDict(mod);
    if (dict == 0) {
      PyErr_SetString(PyExc_RuntimeError, \"Unable to get module dictionary\");
      return;
    }
    image_type = (PyTypeObject*)PyDict_GetItemString(dict, \"Image\");
    subimage_type = (PyTypeObject*)PyDict_GetItemString(dict, \"SubImage\");
    cc_type = (PyTypeObject*)PyDict_GetItemString(dict, \"CC\");
    data_type = (PyTypeObject*)PyDict_GetItemString(dict, \"ImageData\");

  }
  
  """)

  template.execute(sys.stdout, threshold.__dict__)

##  [[# This is a comment #]]
##  [[# This example does recursive function calls need to generate feature combinations #]]
##  [[def switch(layer, args)]]
##     switch(m[[layer]].id) {
##     [[for option in combinations[layer]]]
##     [[exec current = option + '(m' + str(layer) + ')']]
##     case [[option]]:
##       [[if layer == layers - 1]]
##         function_call([[string.join(args + [current], ',')]]);
##       [[else]]
##         [[call switch(layer + 1, args + [current])]]
##       [[end]]
##     break;
##     [[end]]
##     }
##  [[end]]
  
##  PyObject *call_[[function_name]](PyObject* self, PyObject *args) {
##  [[call switch(0, [])]]
##  }
