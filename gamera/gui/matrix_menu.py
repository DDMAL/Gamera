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

from wxPython.wx import *   # wxPython
import keyword              # Python standard library
from gamera.gamera import *        # Gamera-specific
from gamera import util
from gamera.gui import var_name
from gamera.args import *

EXECUTE_MODE = 0
HELP_MODE = 1

# These variables need to be globals and not member variables to avoid
# stickiness problem (they, for some reason, don't get changed when a
# new menu is created)
functions = []
shell = None
shell_frame = None
matrices = None
matrices_name = None


######################################################################
# These are redefined here from gamera_shell, since importing
# gamera_shell here causes cyclic references
def verify_variable_name(name):
  if keyword.iskeyword(name):
    return 0
  return 1

def set_shell(sh):
  global shell
  shell = sh

def set_shell_frame(sf):
  global shell_frame
  shell_frame = sf



######################################################################

class MatrixMenu:
  def __init__(self, parent, x, y, matrices_, name_, shell_=None,
               mode=EXECUTE_MODE):
    global matrices, matrices_name
    self.shell = shell_
    self.x = x
    self.y = y
    self.locals = locals
    self.mode = mode
    self.parent = parent
    if not util.is_sequence(matrices_):
      matrices = [matrices_]
      matrices_name = [name_]
    else:
      matrices = matrices_
      matrices_name = name_
    self.variables = matrices[0].members()
    self.methods = matrices[0].methods()

  # Given a list of variables and methods, put it all together
  def create_menu(self, variables, methods, type):
    global functions
    menu = wxMenu()
    # Top line
    if self.mode == HELP_MODE:
      menu.Append(0, "Help")
    menu.Append(0, type + " Matrix")
    menu.AppendSeparator()

    menu.Append(10000, "new reference")
    EVT_MENU(self.parent, 10000, self.OnCreateReference)
    menu.Append(10001, "new copy")
    EVT_MENU(self.parent, 10001, self.OnCreateCopy)
    menu.AppendSeparator()

    # Variables
    for i in range(len(variables)):
      menu.Append(0, variables[i])
    EVT_MENU(self.parent, 0, self.OnPopupVariable)
    menu.AppendSeparator()
    # Methods
    functions = [None]
    menu = self.create_methods(methods, menu)
    return menu

  def create_methods(self, methods, menu):
    global functions
    items = methods.items()
    items.sort()
    for key, val in items:
      if type(val) == type({}):
        item = self.create_methods(val, wxMenu())
        menu.AppendMenu(0, key, item)
      else:
        menu.Append(len(functions), key)
        EVT_MENU(self.parent, len(functions), self.OnPopupFunction)
        functions.append(val)
    return menu

  def PopupMenu(self):
    menu = self.create_menu(self.variables,
                            self.methods,
                            matrices[0].get_type())
    self.parent.PopupMenu(menu, wxPoint(self.x, self.y))
    menu.Destroy()

  def GetMenu(self):
    return self.create_main_menu(self.variables,
                                 self.methods,
                                 matrices[0].get_type())

  def get_shell(self):
    if self.shell:
      return self.shell
    return shell

  # Creates a new top-level reference to the matrix
  def OnCreateReference(self, event):
    sh = self.get_shell()
    name = var_name.get("ref", sh.locals)
    if name:
      if len(matrices) == 1:
        sh.locals[name] = matrices[0]
      else:
        sh.locals[name] = []
        for i in range(len(matrices)):
          sh.locals[name].append(matrices[i])
      sh.Update()    

  def OnCreateCopy(self, event):
    sh = self.get_shell()
    name = var_name.get("copy", sh.locals)
    if name:
      if len(matrices) == 1:
        sh.locals[name] = matrices[0].matrix_copy()
      else:
        sh.locals[name] = []
        for i in range(len(matrices)):
          sh.locals[name].append(matrices[i].matrix_copy())
      sh.Update()

  # TODO: not implemented
  def OnPopupVariable(self, event):
    print ("Here we will set the value of ",
           self.variables[event.GetId() - 1])

  def get_function_call(self, sh, function):
    # determine if the function requires an argument gui
    if function.args in ('', None, (), []):
      # if not, we can just use empty parentheses
      return function.__name__ + "()"
    # else, display the argument gui and use what is returns
    return function.args.show(self.parent,
                              sh.GetLocals(),
                              function.__name__)

  def get_result_name(self, function, dict):
    if function.return_type not in ('', None):
      return var_name.get(function.return_type.name, dict)
    return ''

  def get_matrix_name(self, matrices_name, i):
    # If the matrix exists at the top-level in the shell's
    # namespace, we can use that to refer to it
    if util.is_sequence(matrices_name):
      # If it is a single matrix
      return matrices_name[i]
    elif type(matrices_name) == type('') and matrices_name != '':
      # If is is a list of matrices
      return matrices_name + "[" + str(i) + "]"
    # The matrix does not exist at the top-level of the shell's
    # namespace
    return matrices_name

  def OnPopupFunction(self, event):
    sh = self.get_shell()
    
    function = functions[event.GetId()]
    if matrices:
      if self.mode == HELP_MODE:
        sh.run("help('" + function.__name__ + "')")
      elif self.mode == EXECUTE_MODE:
        func_call = self.get_function_call(sh, function)
        if func_call == None:  # User pressed cancel, so bail
          return
        result_name = self.get_result_name(function, sh.locals)
        # if we're going to return multiple results, prepare the
        # variable as a list
        if len(matrices) > 1 and result_name != '':
          sh.locals[result_name] = []
        # Now let's run some code and get results
        for i in range(len(matrices)):
          matrix = matrices[i]
          matrix_name = self.get_matrix_name(matrices_name, i)
          # If the matrix name is a string, we can call the function
          # directly in the shell
          if type(matrix_name) == type(''):
            source = matrix_name + "." + func_call
            if result_name != '':
              if len(matrices) > 1:
                source = result_name + ".append(" + source + ")"
              else:
                source = result_name + " = " + source
            sh.run(source)
          # If the matrix name is not a string, we have to call the
          # function here
          else:
            source = "matrices_name[" + str(i) + "]." + func_call
            if result_name != '':
              if len(matrices) > 1:
                sh.locals[result_name].append(eval(source))
              else:
                sh.locals[result_name] = eval(source)
            else:
              eval(source, globals(), sh.locals)



