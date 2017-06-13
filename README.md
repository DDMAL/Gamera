Gamera
======

Unofficial Gamera Mirror

How to Install wxPython
 
Download:
https://sourceforge.net/projects/wxpython/files/wxPython/2.8.12.1/wxPython2.8-osx-unicode-2.8.12.1-universal-py2.7.dmg/download
 
Mount the dmg, inside there is a .pkg file. (This is an old osx packaging format) 
You can inspect its contents by right clicking it and choosing ‘Show Package Contents’
 
In package, go to Contents/Resources
Run the following in terminal:
```` 
sudo ./preflight
 
sudo tar -xvf wxPython2.8-osx-unicode-universal-py2.7.pax.gz -C /
 
sudo ./postflight 
```` 
This does everything the package would have done, including moving files into /usr and /lib.
 
It’s possible this will only be the 32 bit version. You can set python to default to running with 32 bits by running the following line (on mac)
 
defaults write com.apple.versioner.python Prefer-32-Bit -bool yes
 
NOW you can test by opening up a python terminal and 

```` 
import wx
wx.__version__
````

If it is 2.8.12.1 you are golden ponyboy.
 
 
## Install gamera:
http://gamera.informatik.hsnr.de/download/index.html
 
python setup.py build && sudo python setup.py install
 
 
*** If you get an error about not being able to find a 32-bit version of python while installing gamera, even after doing the defaults write command correctly.*** 
 
In terminal, go to the folder where you extracted gamera 
Create a file called python_32 with NO extentions (you can use vim or a text editor)
 
Write this in the file (use the method you prefer, I suggest copy & paste)
```` 
#! /bin/bash
export VERSIONER_PYTHON_PREFER_32_BIT=yes
/usr/bin/python "$@"
````
Save the file, and execute this command on the file you’ve just created. 
 
chmod a+x ./python_32
 
And install gamera with the terminal command:
 
sudo ./python_32 setup.py install_scripts -d /usr/local/bin
 
Or:
sudo pip install -e /!!location-of the gamera FOLDER!! 
 
 
You can test the installation again by going into terminal, running python and import wx Bob’s your uncle. 
