from distutils.core import setup, Extension

setup(name = "gameracore", version="1.1",
      ext_modules = [Extension("gameracore", ["src/gameramodule.cpp",
                                          "src/sizeobject.cpp",
                                          "src/pointobject.cpp",
                                          "src/dimensionsobject.cpp",
                                          "src/rectobject.cpp",
                                          "src/imagedataobject.cpp"
                                          ], include_dirs=["include"],
                               libraries=["stdc++"])]
      )
