from distutils.core import setup, Extension
import sys, os, time, locale
import glob
from gamera import generate

##########################################
# generate the plugins
plugins = glob.glob("gamera/plugins/*.py")
plugins.remove("gamera/plugins/__init__.py")

plugins = ["gamera/plugins/threshold.py"]

plugin_extensions = []
for x in plugins:
    plugin_extensions.append(generate.generate_plugin(x))

########################################
# Check that this is at least Python 2.2
if float(''.join([str(x) for x in sys.version_info[0:3]])) < 220:
    print "Gamera requires Python version 2.2 or greater."
    print "You are running the following Python version:"
    print sys.version
    sys.exit(1)

########################################
# Generate the command line scripts based on the running version of Python
command_line_utils = {
    'gamera_gui':
    """#!%(executable)s\n"""
    """%(header)s\n"""
    """print "Loading GAMERA..."\n"""
    """from gamera.gui import gui\n"""
    """gui.run()""",

    'gamera_cl':
    """#!/usr/bin/env sh\n"""
    """%(header)s\n"""
    """%(executable)s -i -c "from gamera.gamera import *; init_gamera()"\n"""
    }

if os.name == 'posix':
    info = {'executable': sys.executable,
            'header'    :
            """# This file was automatically generated by the\n"""
            """# Gamera setup script on %s.\n""" %
            time.strftime(locale.nl_langinfo(locale.D_FMT))}
    for file, content in command_line_utils.items():
        fd = open(file, 'w')
        fd.write(content % info)
        fd.close()
        os.chmod(file, 0700)

########################################
# Distutils setup

extensions = [Extension("gamera.gameracore",
                        ["src/gameramodule.cpp",
                         "src/sizeobject.cpp",
                         "src/pointobject.cpp",
                         "src/dimensionsobject.cpp",
                         "src/rectobject.cpp",
                         "src/regionobject.cpp",
                         "src/regionmapobject.cpp",
                         "src/rgbpixelobject.cpp",
                         "src/imagedataobject.cpp",
                         "src/imageobject.cpp",
                         "src/imageinfoobject.cpp"
                         ],
                        include_dirs=["include"],
                        # FIXME
                        libraries=["stdc++"]),
              Extension("gamera.knn", ["src/knnmodule.cpp"],
                        include_dirs=["include"], libraries=["stdc++"])]
extensions.extend(plugin_extensions)

setup(name = "gameracore", version="1.1",
      ext_modules = extensions,
      packages = ['gamera', 'gamera.gui', 'gamera.plugins', 'gamera.toolkits',
                  'gamera.toolkits.omr']
      )
