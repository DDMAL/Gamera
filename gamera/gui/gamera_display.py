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

from wxPython.wx import *         # wxPython
from wxPython.grid import *
from wxPython.lib.stattext import wxGenStaticText as wxStaticText
from wxPython.wx import __version__ as __wx_version__
if __wx_version__[-1] == 'u':
   __wx_version__ = __wx_version__[0:-1]
__wx_version__ = int(''.join(__wx_version__.split('.')))
from math import sqrt, ceil, log, floor # Python standard library
from sys import maxint
import sys, string, time, weakref
from gamera.core import *             # Gamera specific
from gamera import paths, util
from gamera.gui import image_menu, var_name, gui_util, toolbar
import gamera.plugins.gui_support  # Gamera plugin


##############################################################################

# we want this done on import
wxInitAllImageHandlers()
config.define_option(
   "display", "scroll_amount", 32,
   help="The size of each scroll step in the image viewer.")

#############################################################################

##############################################################################
# SINGLE IMAGE DISPLAY
##############################################################################
#
# Image display is a scrolled window for displaying a single image

class ImageDisplay(wxScrolledWindow):
   def __init__(self, parent, id = -1, size = wxDefaultSize):
      wxScrolledWindow.__init__(
         self, parent, id, wxPoint(0, 0), size,
         wxCLIP_CHILDREN|wxNO_FULL_REPAINT_ON_RESIZE)
      self.SetBackgroundColour(wxWHITE)

      self.scaling = 1.0
      self.scaling_quality = 0
      self.view_function = None
      self.image = None
      self.original_image = None
      self.highlights = []
      self.boxed_highlights = 0
      self._boxed_highlight = None
      self._boxed_highlight_position = 0
      self._boxed_highlight_timer = wxTimer(self, 100)
      EVT_TIMER(self, 100, self._OnBoxHighlight)
      self.boxes = []
      self.rubber_on = 0
      self.rubber_origin_x = 0
      self.rubber_origin_y = 0
      self.rubber_x2 = 0
      self.rubber_y2 = 0
      self.click_callbacks = []
      self.current_color = 0
      self.dragging = 0
      self.scroll_amount = int(config.options.display.scroll_amount)
      self.block_w = 8
      self.block_h = 8

      EVT_PAINT(self, self._OnPaint)
      EVT_SIZE(self, self._OnResize)
      EVT_LEFT_DOWN(self, self._OnLeftDown)
      EVT_LEFT_UP(self, self._OnLeftUp)
      EVT_MIDDLE_DOWN(self, self._OnMiddleDown)
      EVT_MIDDLE_UP(self, self._OnMiddleUp)
      EVT_RIGHT_DOWN(self, self._OnRightDown)
      EVT_MOTION(self, self._OnMotion)
      EVT_LEAVE_WINDOW(self, self._OnLeave)

   ########################################
   # THE MAIN IMAGE

   # Sets the image being displayed
   # Returns the size of the image
   def set_image(self, image, view_function=None, weak=1):
      if weak:
         self.original_image = weakref.proxy(image)
         self.image = weakref.proxy(image)
      else:
         self.original_image = image
         self.image = image
      self.clear_all_highlights()
      self.view_function = view_function
      self.Raise()
      return self.reload_image()

   # Refreshes the image by recalling the to_string function
   def reload_image(self, *args):
      if self.view_function != None and self.original_image != None:
         self.image = getattr(self.image, self.view_function)()
      if self.image != None:
         self.scale()
         return (self.image.width, self.image.height)

   def add_click_callback(self, cb):
      self.click_callbacks.append(cb)

   def remove_click_callback(self, cb):
      self.click_callbacks.remove(cb)

   ########################################
   # BOXES

   def add_box(self, y, x, h, w):
      self.boxes.append(Rect(y, x, h, w))
      self.RefreshAll()

   def clear_all_boxes(self):
      self.boxes = []
      self.RefreshAll()

   def draw_box(self, box, dc=None):
      if dc == None:
         dc = wxClientDC(self)
      scaling = self.scaling
      origin = [x * self.scroll_amount for x in self.GetViewStart()]

      x, y, x2, y2 = tuple([int(a * scaling) for a in (box.ul_x, box.ul_y,
                                                       box.lr_x + 1, box.lr_y + 1)])
      x -= origin[0]
      y -= origin[1]
      x2 -= origin[0]
      y2 -= origin[1]
      w = x2 - x
      h = y2 - y

      dc.SetLogicalFunction(wxCOPY)
      pen = wxPen(wxColor(0, 0, 255), 2, wxSOLID)
      dc.SetPen(pen)
      dc.SetBrush(wxTRANSPARENT_BRUSH)      
      dc.DrawRectangle(x, y, w, h)

   def draw_boxes(self, dc=None):
      for b in self.boxes:
         self.draw_box(b, dc)


   ########################################
   # HIGHLIGHTED CCs

   def _OnBoxHighlight(self, event):
      if len(self.highlights) == 1 and not self.rubber_on:
         self._boxed_highlight_position &= 0x7  # mod 8
         highlight = self.highlights[0][0]
         i = (16 + abs(4 - self._boxed_highlight_position)) / self.scaling
         self.draw_rubber()
         self.rubber_origin_x = highlight.ul_x - i
         self.rubber_origin_y = highlight.ul_y - i
         self.rubber_x2 = highlight.lr_x + i
         self.rubber_y2 = highlight.lr_y + i
         self._boxed_highlight_position -= 1
         self.draw_rubber()

   def highlight_rectangle(self, y, x, h, w, color, text):
      cc = self.image.subimage(int(y), int(x), int(h), int(w))
      self.add_highlight_cc(cc, color)

   # Highlights only a particular list of ccs in the display
   def highlight_cc(self, ccs, color=None):
      ccs = util.make_sequence(ccs)
      # For efficiency, if we're updating fewer than 10 glyphs, we only
      # update that part of the display, otherwise, we update the entire
      # display
      refresh_at_once = len(ccs) > 10 or len(self.highlights) > 10
      use_color = color
      old_highlights = self.highlights[:]
      self.highlights = []
      for cc in ccs:
         # If the highlight is in the old highlights, use the old
         # color if the color is unspecified.
         # This is really just for "color stability."
         if color == None:
            use_color = None
            for old_cc, old_color in old_highlights:
               if old_cc == cc:
                  use_color = old_color
                  break
         self.highlights.append((cc, gui_util.get_color(use_color)))
         if not refresh_at_once:
            self.PaintAreaRect(cc)
      if not refresh_at_once:
         for cc in old_highlights:
            if cc not in self.highlights:
               self.PaintAreaRect(cc[0])
      else:
         self.RefreshAll()
   highlight_ccs = highlight_cc

   # Adds a list of ccs to be highlighted in the display
   def add_highlight_cc(self, ccs, color=-1):
      ccs = util.make_sequence(ccs)
      # For efficiency, if we're updating fewer than 10 glyphs, we only
      # update that part of the display, otherwise, we update the entire
      # display
      refresh_at_once = len(ccs) > 10
      use_color = color
      for cc in ccs:
         # Only add cc if not already in highlighted list
         add = 1
         for hl, color in self.highlights:
            if cc == hl:
               add = 0
               break
         if add:
            self.highlights.append(
               (cc, gui_util.get_color(use_color)))
            if not refresh_at_once:
               self.PaintAreaRect(cc)
      if refresh_at_once:
         self.RefreshAll()
   add_highlight_ccs = add_highlight_cc

   # Clears a CC in the display.  Multiple CCs
   # may be unhighlighted in turn
   def unhighlight_cc(self, ccs):
      ccs = util.make_sequence(ccs)
      refresh_at_once = len(ccs) > 10
      for cc in ccs:
         for i in range(0, len(self.highlights)):
            if (self.highlights[i][0] == cc):
               del self.highlights[i]
               if not refresh_at_once:
                  self.PaintAreaRect(cc)
               break
      if refresh_at_once:
         self.RefreshAll()
   unhighlight_ccs = unhighlight_cc
      
   # Clears all of the highlighted CCs in the display   
   def clear_all_highlights(self):
      old_highlights = self.highlights[:]
      refresh_at_once = len(old_highlights) > 10
      self.highlights = []
      if not refresh_at_once:
         for highlight in old_highlights:
            self.PaintAreaRect(highlight[0])
      else:
         self.RefreshAll()

   # Adjust the scrollbars so a group of highlighted subimages are visible
   def focus_glyphs(self, glyphs):
      glyphs = util.make_sequence(glyphs)
      if len(glyphs) == 0:
         return
      # Get a combined rectangle of all images in the list
      self.focus_rect(glyphs[0].union_rects(glyphs))

   def focus_rect(self, rect):
      self.focus_points(rect.ul_x, rect.ul_y, rect.lr_x, rect.lr_y)

   def focus_points(self, x1, y1, x2, y2, force=0):
      if self.image is None:
         return
      scroll_amount = self.scroll_amount
      # Adjust for the scaling factor
      scaling = self.scaling
      x1, y1, x2, y2 = tuple([x * scaling for x in (x1, y1, x2, y2)])
      origin_x, origin_y = tuple(
         [x * self.scroll_amount for x in self.GetViewStart()])
      maxwidth = self.image.width * scaling
      maxheight = self.image.height * scaling
      size = self.GetSize()
      if force:
         need_to_scroll = 1
         set_x = max(0, min(maxwidth - size.x / self.scaling, x1 - 20))
         set_y = max(0, min(maxheight - size.y / self.scaling, y1 - 20))
      else:
         need_to_scroll = 0
         if (x1 < origin_x or x2 > origin_x + size.x - 32):
            set_x = max(0, min(maxwidth - size.x / self.scaling, x1 - 20))
            need_to_scroll = 1
         else:
            set_x = origin_x
         if (y1 < origin_y or y2 > origin_y + size.y - 32 or
             need_to_scroll):
            set_y = max(0, min(maxheight - size.y / self.scaling, y1 - 20))
            need_to_scroll = 1
         else:
            set_y = origin_y
      if need_to_scroll:
         self.SetScrollbars(scroll_amount, scroll_amount,
                            int(maxwidth / scroll_amount),
                            int(maxheight / scroll_amount),
                            int(set_x / scroll_amount),
                            int(set_y / scroll_amount))
         self.RefreshAll()

   def _OnBoxHighlightsToggle(self, event):
      self.boxed_highlights = event.GetIsDown()
      if self.boxed_highlights:
         if not self._boxed_highlight_timer.IsRunning():
            self._boxed_highlight_timer.Start(100)
      else:
         if self._boxed_highlight_timer.IsRunning():
            self._boxed_highlight_timer.Stop()
      self.RefreshAll()

   ########################################
   # SCALING
   #
   def scale(self, new_scale=None):
      if self.image is None:
         return
      old_scale = self.scaling
      if new_scale < old_scale:
         self.Clear()
         self.RefreshAll()
      if new_scale == None or new_scale <= 0:
         new_scale = old_scale
         
      # Clamp scaling to a reasonable range
      # Going outside of this range has been known to cause segfaults
      new_scale = min(max(new_scale, 0.1), 16)

      # Quantize scaling to integers, (or 1.0 / integer)
      # This makes scrolling look nice
      if new_scale >= 1.0:
         new_scale = floor(new_scale)
      else:
         new_scale = 1.0 / ceil(1.0 / new_scale)
      scroll_amount = self.scroll_amount
      w = self.image.width * new_scale
      h = self.image.height * new_scale
      origin = [x * self.scroll_amount for x in self.GetViewStart()]
      size = self.GetSize()
      # These weird looking functions ensure that the scrollbars don't
      # jump around too much when zooming in/out
      x = max(min(origin[0] * new_scale / old_scale + 1, w - size.x) - 2, 0)
      y = max(min(origin[1] * new_scale / old_scale + 1, h - size.y) - 2, 0)
      self.scaling = new_scale

      wxBeginBusyCursor()
      try:
         if self.highlights == []:
            self.SetScrollbars(scroll_amount, scroll_amount,
                               int(w / scroll_amount),
                               int(h / scroll_amount),
                               int((x / scroll_amount) + 0.5),
                               int((y / scroll_amount) + 0.5))
         else:
            self.focus_glyphs([x[0] for x in self.highlights])
      finally:
         self.RefreshAll()
         wxEndBusyCursor()
         
   def ZoomOut(self, *args):
      if self.scaling > 0.125:
         self.scale(self.scaling * 0.5)

   def ZoomNorm(self, *args):
      self.scale(1.0)

   def ZoomIn(self, *args):
      if self.scaling < 8:
         self.scale(self.scaling * 2.0)

   def ZoomView(self, *args):
      if self.image is None:
         return
      scroll_amount = self.scroll_amount
      # Zooms in as best it can on the current view
      x = min(self.rubber_origin_x, self.rubber_x2)
      y = min(self.rubber_origin_y, self.rubber_y2)
      x2 = max(self.rubber_origin_x, self.rubber_x2)
      y2 = max(self.rubber_origin_y, self.rubber_y2)
      rubber_w = (x2 - x) + scroll_amount
      rubber_h = (y2 - y) + scroll_amount
      size = self.GetSize()
      if rubber_w == scroll_amount and rubber_h == scroll_amount:
         rubber_w = self.image.ncols
         rubber_h = self.image.nrows
         x = y = x2 = y2 = 0
      scaling = min(float(size.x) / float(rubber_w),
                    float(size.y) / float(rubber_h)) * 0.95
      self.scale(scaling)
      self.focus_points(x, y, x2, y2, force=1)

   def SetZoomType(self, type):
      self.scaling_quality = type
      if self.scaling != 1.0:
         self.RefreshAll()

   ########################################
   # RUBBER BAND
   #
   def draw_rubber(self, dc=None):
      if dc == None:
         dc = wxClientDC(self)
      scaling = self.scaling
      origin = [x * self.scroll_amount for x in self.GetViewStart()]
      x = min(self.rubber_origin_x, self.rubber_x2)
      y = min(self.rubber_origin_y, self.rubber_y2)
      x2 = max(self.rubber_origin_x, self.rubber_x2)
      y2 = max(self.rubber_origin_y, self.rubber_y2)
      x, y, x2, y2 = tuple([int(a * scaling) for a in (x, y, x2, y2)])
      x -= origin[0]
      y -= origin[1]
      x2 -= origin[0]
      y2 -= origin[1]
      w = x2 - x
      h = y2 - y
      dc.SetLogicalFunction(wxXOR)
      pen = wxGREY_PEN
      pen.SetStyle(wxDOT)
      dc.SetPen(pen)
      dc.SetBrush(wxTRANSPARENT_BRUSH)
      dc.DrawRectangle(x, y, w, h)
      dc.SetPen(wxTRANSPARENT_PEN)
      brush = wxBLUE_BRUSH
      brush.SetColour(wxColor(167, 105, 39))
      dc.SetBrush(brush)
      self.block_w = block_w = max(min(w / 2 - 1, 8), 0)
      self.block_h = block_h = max(min(h / 2 - 1, 8), 0)
      dc.DrawRectangle(x + 1, y + 1, block_w, block_h)
      dc.DrawRectangle(x2 - block_w - 1, y + 1, block_w, block_h)
      dc.DrawRectangle(x + 1, y2 - block_h - 1, block_w, block_h)
      dc.DrawRectangle(x2 - block_w - 1, y2 - block_h - 1, block_w, block_h)
      dc.SetLogicalFunction(wxCOPY)

   def _OnRubber(self, shift):
      #deliberately empty -- this is a callback to be overridden by subclasses
      pass

   ########################################
   # UTILITY
   #
   def _OnMakeView(self, *args):
      name = var_name.get("view", image_menu.shell.locals)
      if name:
         if (self.rubber_y2 == self.rubber_origin_y and
             self.rubber_x2 == self.rubber_origin_x):
            subimage = self.original_image.subimage(
               0, 0,
               self.original_image.nrows, self.original_image.ncols)
         else:
            subimage = self.original_image.subimage(
               int(self.rubber_origin_y + self.original_image.ul_y),
               int(self.rubber_origin_x + self.original_image.ul_x),
               int(self.rubber_y2 - self.rubber_origin_y + 1),
               int(self.rubber_x2 - self.rubber_origin_x + 1))
         image_menu.shell.locals[name] = subimage
         image_menu.shell.update()

   def _OnMakeCopy(self, *args):
      name = var_name.get("copy", image_menu.shell.locals)
      if name:
         if (self.rubber_y2 == self.rubber_origin_y and
             self.rubber_x2 == self.rubber_origin_x):
            copy = self.original_image.image_copy()
         else:
            copy = self.original_image.subimage(
               int(self.rubber_origin_y + self.original_image.ul_y),
               int(self.rubber_origin_x + self.original_image.ul_x),
               int(self.rubber_y2 - self.rubber_origin_y + 1),
               int(self.rubber_x2 - self.rubber_origin_x + 1)).image_copy()
         image_menu.shell.locals[name] = copy
         image_menu.shell_frame.icon_display.update_icons()

   ########################################
   # CALLBACKS
   #
   def _OnResize(self, event):
      size = self.GetSize()
      if size.x > 0 and size.y > 0:
         event.Skip()
         self.scale(self.scaling)
      else:
         event.Skip()

   def _OnPaint(self, event):
      if not self.image:
         dc = wxPaintDC(self)
         dc.SetPen(wxTRANSPARENT_PEN)
         dc.SetBrush(wxWHITE_BRUSH)
         size = self.GetSize()
         dc.DrawRectangle(0, 0, size.x, size.y)
         dc.SetBrush(wxBrush(wxBLUE, wxFDIAGONAL_HATCH))
         dc.DrawRectangle(0, 0, size.x, size.y)
         return
      scaling = self.scaling
      origin = [x * self.scroll_amount for x in self.GetViewStart()]
      origin_scaled = [x / scaling for x in origin]
      update_regions = self.GetUpdateRegion()
      rects = wxRegionIterator(update_regions)
      # Paint only where wxWindows tells us to, this is faster
      dc = wxPaintDC(self)
      dc.BeginDrawing()
      while rects.HaveRects():
         ox = rects.GetX() / scaling
         oy = rects.GetY() / scaling
         x = ox + origin_scaled[0]
         y = oy + origin_scaled[1]
         # For some reason, the rectangles wxWindows gives are
         # a bit too small, so we need to fudge their size
         if not (x > self.image.width or y > self.image.height):
            # this used to be int(self.scaling / 2) but this works
            # with the new scaled_to_string. KWM
            fudge = int(self.scaling) * 2
            w = (max(min(int((rects.GetW() / scaling) + fudge),
                         self.image.ncols - ox), 0))
            h = (max(min(int((rects.GetH() / scaling) + fudge),
                         self.image.nrows - oy), 0))
            self.PaintArea(x + self.image.ul_x, y + self.image.ul_y,
                           w, h, check=0, dc=dc)
         rects.Next()
      self.draw_rubber(dc)
      self.draw_boxes(dc)
      dc.EndDrawing()

   def PaintAreaRect(self, rect):
      # When painting a specific area, we have to make it
      # slightly bigger to adjust for scaling
      adjust = self.scaling * 2
      self.PaintArea(rect.ul_x, rect.ul_y,
                     rect.ncols + adjust,
                     rect.nrows + adjust, 1)

   def PaintArea(self, x, y, w, h, check=1, dc=None):
      if not self.image:
         return
      if dc == None:
         dc = wxClientDC(self)
         self.draw_rubber(dc)
         redraw_rubber = 1
      else:
         redraw_rubber = 0
      # This needs to be created every time we call Paint, since its
      # address can (but in practice usually doesn't) change.
      tmpdc = wxMemoryDC()

      scaling = self.scaling
      origin = [a * self.scroll_amount for a in self.GetViewStart()]
      origin_scaled = [a / scaling for a in origin]
      size_scaled = [a / scaling for a in self.GetSizeTuple()]

      # Quantize for scaling
      if scaling > 1.0:
         x = int(int(x / scaling) * scaling)
         y = int(int(y / scaling) * scaling)
         w = int(int(w / scaling) * scaling)
         h = int(int(h / scaling) * scaling)

      if (y < self.image.ul_y):
         y = self.image.ul_y
      if (x < self.image.ul_x):
         x = self.image.ul_x
      if (y + h >= self.image.lr_y):
         h = self.image.lr_y - y + 1
      if (x + w >= self.image.lr_x):
         w = self.image.lr_x - x + 1

      if check:
         if ((x + w < origin_scaled[0]) and
             (y + h < origin_scaled[1]) or
             (x > origin_scaled[0] + size_scaled[0] and
              y > origin_scaled[1] + size_scaled[1])):
            return
         
      # If the update region is smaller than a single pixel, you get
      # "Floating point exceptions", so just bail, since you can't see
      # zero pixels anyway <wink>
      if float(w) * scaling < 1.0 or float(h) * scaling < 1.0:
         if redraw_rubber:
            self.draw_rubber(dc)
         return

      subimage = self.image.subimage(y, x, h, w)
      image = None
      if scaling != 1.0:
         # For the high quality scalings a greyscale (or rgb) is required
         # to really realize the increased quality. There is some smoothing
         # that happens for zooming in, but for zooming out grey pixels are
         # required for the smoothing. This simply does a conversion on the
         # fly, which can be very slow, but limits the total memory usage.
         # The other option is to cache a greyscale copy of the image, but
         # that could use too much memory.
         if self.scaling_quality > 0 and subimage.data.pixel_type == ONEBIT:
            subimage = subimage.to_greyscale()
         scaled_image = subimage.resize(int(ceil(subimage.nrows * scaling)),
                                        int(ceil(subimage.ncols * scaling)),
                                        self.scaling_quality)
      else:
         scaled_image = subimage
      image = wxEmptyImage(scaled_image.ncols, scaled_image.nrows)
      scaled_image.to_buffer(image.GetDataBuffer())
      bmp = wxBitmapFromImage(image)
      x = int((x - self.image.ul_x) * scaling - origin[0])
      y = int((y - self.image.ul_y) * scaling - origin[1])

      tmpdc.SelectObject(bmp)
      dc.Blit(x, y, scaled_image.ncols, scaled_image.nrows,
              tmpdc, 0, 0, wxCOPY, True)

      if len(self.highlights):
         for highlight, color in self.highlights:
            if subimage.intersects(highlight):
               subhighlight = highlight.clip_image(subimage)
               h = subhighlight.nrows
               w = subhighlight.ncols
               if scaling != 1.0:
                  # If zooming has caused the glyph to shrink into
                  # nothingness, don't draw it.
                  if float(h) * scaling <= 1 or float(w) * scaling <= 1:
                     continue
                  scaled_highlight = subhighlight.resize(
                     ceil(subhighlight.nrows * scaling),
                     ceil(subhighlight.ncols * scaling),
                     self.scaling_quality)
               else:
                  # Sometimes we get an image with 0 rows or cols - I'm
                  # not certain the cause, but this will prevent wxPython
                  # errors. KWM
                  if h == 0 or w == 0:
                     continue
                  scaled_highlight = subhighlight
               image = wxEmptyImage(
                  scaled_highlight.ncols, scaled_highlight.nrows)
               scaled_highlight.to_buffer(image.GetDataBuffer())
               bmp = wxBitmapFromImage(image)
               tmpdc = wxMemoryDC()
               tmpdc.SelectObject(bmp)
               x_cc = int(x + (subhighlight.ul_x - subimage.ul_x) * scaling)
               y_cc = int(y + (subhighlight.ul_y - subimage.ul_y) * scaling)
               dc.Blit(x_cc, y_cc,
                       scaled_highlight.ncols, scaled_highlight.nrows,
                       tmpdc, 0, 0, wxAND, True)
               tmpdc.SetBrush(wxBrush(color, wxSOLID))
               tmpdc.SetLogicalFunction(wxAND_REVERSE)
               tmpdc.SetPen(wxTRANSPARENT_PEN)
               tmpdc.DrawRectangle(0, 0, bmp.GetWidth(), bmp.GetHeight())
               dc.Blit(x_cc, y_cc,
                       scaled_highlight.ncols, scaled_highlight.nrows,
                       tmpdc, 0, 0, wxOR, True)
      if redraw_rubber:
         self.draw_rubber(dc)
            
   def RefreshAll(self):
      size = self.GetSize()
      self.Refresh(0, rect=wxRect(0, 0, size.x, size.y))

   def _OnLeftDown(self, event):
      if not self.image:
         return
      self.CaptureMouse()
      self.rubber_on = 1
      self.draw_rubber()
      origin = [x * self.scroll_amount for x in self.GetViewStart()]
      x = min((event.GetX() + origin[0]) / self.scaling,
              self.image.ncols - 1)
      y = min((event.GetY() + origin[1]) / self.scaling,
              self.image.nrows - 1)
      if (x >= self.rubber_origin_x and
          x <= self.rubber_origin_x + self.block_w):
         self.rubber_origin_x, self.rubber_x2 = \
                               self.rubber_x2, self.rubber_origin_x
         if (y >= self.rubber_origin_y and
             y <= self.rubber_origin_y + self.block_h):
            self.rubber_origin_y, self.rubber_y2 = \
                                  self.rubber_y2, self.rubber_origin_y
            self.draw_rubber()
            return
         elif (y <= self.rubber_y2 and
               y >= self.rubber_y2 - self.block_h):
            self.draw_rubber()
            return
      elif (x <= self.rubber_x2 and
            x >= self.rubber_x2 - self.block_w):
         if (y >= self.rubber_origin_y and
             y <= self.rubber_origin_y + self.block_h):
            self.rubber_origin_y, self.rubber_y2 = \
                                  self.rubber_y2, self.rubber_origin_y
            self.draw_rubber()
            return
         elif (y <= self.rubber_y2 and
               y >= self.rubber_y2 - self.block_h):
            self.draw_rubber()
            return
      self.rubber_origin_x = self.rubber_x2 = int(x)
      self.rubber_origin_y = self.rubber_y2 = int(y)

   def _OnLeftUp(self, event):
      if self.rubber_on:
         if self.HasCapture():
            self.ReleaseMouse()
         self.draw_rubber()
         origin = [x * self.scroll_amount for x in self.GetViewStart()]
         self.rubber_x2 = int(max(min((event.GetX() + origin[0]) / self.scaling,
                                      self.image.ncols - 1), 0))
         self.rubber_y2 = int(max(min((event.GetY() + origin[1]) / self.scaling,
                                      self.image.nrows - 1), 0))
         for callback in self.click_callbacks:
            callback(self.rubber_y2, self.rubber_x2)
         self.draw_rubber()
         if self.rubber_origin_x > self.rubber_x2:
            self.rubber_origin_x, self.rubber_x2 = \
                                  self.rubber_x2, self.rubber_origin_x
         if self.rubber_origin_y > self.rubber_y2:
            self.rubber_origin_y, self.rubber_y2 = \
                                  self.rubber_y2, self.rubber_origin_y
         self.rubber_on = 0
         self._OnRubber(event.ShiftDown() or event.ControlDown())

   def _OnMiddleDown(self, event):
      if not self.image:
         return
      wxSetCursor(wxStockCursor(wxCURSOR_BULLSEYE))
      self.dragging = 1
      self.dragging_x = event.GetX()
      self.dragging_y = event.GetY()
      self.dragging_origin_x, self.dragging_origin_y = \
         [x * self.scroll_amount for x in self.GetViewStart()]

   def _OnMiddleUp(self, event):
      wxSetCursor(wxSTANDARD_CURSOR)
      self.dragging = 0

   def _OnRightDown(self, event):
      if not self.image:
         return
      from gamera.gui import image_menu
      menu = image_menu.ImageMenu(
         self, event.GetX(), event.GetY(),
         self.image)
      if menu.did_something:
         self.reload_image()

   def _OnMotion(self, event):
      if self.rubber_on:
         self.draw_rubber()
         origin = [x * self.scroll_amount for x in self.GetViewStart()]
         self.rubber_x2 = int(
            max(min((event.GetX() + origin[0]) / self.scaling,
                    self.image.ncols - 1), 0))
         self.rubber_y2 = int(
            max(min((event.GetY() + origin[1]) / self.scaling,
                    self.image.nrows - 1), 0))
         self.draw_rubber()
      if self.dragging:
         self.Scroll(
            (self.dragging_origin_x - (event.GetX() - self.dragging_x)) /
            self.scroll_amount,
            (self.dragging_origin_y - (event.GetY() - self.dragging_y)) /
            self.scroll_amount)

   def _OnLeave(self, event):
      if not self.HasCapture() and self.rubber_on:
         if self.rubber_origin_x > self.rubber_x2:
            self.rubber_origin_x, self.rubber_x2 = \
                                  self.rubber_x2, self.rubber_origin_x
         if self.rubber_origin_y > self.rubber_y2:
            self.rubber_origin_y, self.rubber_y2 = \
                                  self.rubber_y2, self.rubber_origin_y
         self.rubber_on = 0
         self._OnRubber(event.ShiftDown() or event.ControlDown())
      if self.dragging:
         self.dragging = 0
         
class ImageWindow(wxPanel):
   def __init__(self, parent = None, id = -1):
      wxPanel.__init__(self, parent, id, style=
                       wxNO_FULL_REPAINT_ON_RESIZE|wxCLIP_CHILDREN)
      self.id = self.get_display()
      self.SetAutoLayout(True)
      self.toolbar = toolbar.ToolBar(self, -1)
      from gamera.gui import gamera_icons
      self.toolbar.AddSimpleTool(10, gamera_icons.getIconRefreshBitmap(),
                                 "Refresh", self.id.reload_image)
      self.toolbar.AddSeparator()
      self.toolbar.AddSimpleTool(
         20, gamera_icons.getIconZoomInBitmap(),
         "Zoom in", self.id.ZoomIn)
      self.toolbar.AddSimpleTool(
         21, gamera_icons.getIconZoomNormBitmap(),
         "Zoom 100%", self.id.ZoomNorm)
      self.toolbar.AddSimpleTool(
         22, gamera_icons.getIconZoomOutBitmap(),
         "Zoom out", self.id.ZoomOut)
      self.toolbar.AddSimpleTool(
         23, gamera_icons.getIconZoomViewBitmap(),
         "Zoom in on selected region", self.id.ZoomView)
      # self.toolbar.AddControl(wxStaticText(self.toolbar, -1, "Quality: "))
      self.zoom_slider = wxComboBox(
         self.toolbar, 24,
         choices=['low quality','medium quality','high quality'],
         style=wxCB_READONLY)
      EVT_COMBOBOX(self, 24, self._OnZoomTypeChange)
      self.toolbar.AddControl(self.zoom_slider)

      self.toolbar.AddSeparator()
      self.toolbar.AddSimpleTool(
         31, gamera_icons.getIconMakeViewBitmap(),
         "Make new view", self.id._OnMakeView)
      self.toolbar.AddSimpleTool(
         32, gamera_icons.getIconImageCopyBitmap(),
         "Make new copy", self.id._OnMakeCopy)

      lc = wxLayoutConstraints()
      lc.top.SameAs(self, wxTop, 0)
      lc.left.SameAs(self, wxLeft, 0)
      lc.right.SameAs(self, wxRight, 0)
      lc.height.AsIs()
      self.toolbar.SetAutoLayout(True)
      self.toolbar.SetConstraints(lc)
      lc = wxLayoutConstraints()
      lc.top.Below(self.toolbar, 0)
      lc.left.SameAs(self, wxLeft, 0)
      lc.right.SameAs(self, wxRight, 0)
      lc.bottom.SameAs(self, wxBottom, 0)
      self.id.SetAutoLayout(True)
      self.id.SetConstraints(lc)
      self.Layout()

   def get_display(self):
      return ImageDisplay(self)

   ########################################
   # CALLBACKS
   def _OnZoomTypeChange(self, event):
      self.id.SetZoomType(
         self.zoom_slider.GetSelection())


##############################################################################
# MULTIPLE IMAGE DISPLAY IN A GRID
##############################################################################

class MultiImageGridRenderer(wxPyGridCellRenderer):
   def __init__(self, parent):
      wxPyGridCellRenderer.__init__(self)
      self.parent = parent

   _colors = {UNCLASSIFIED: wxColor(255,255,255),
              AUTOMATIC:    wxColor(198,145,145),
              HEURISTIC:    wxColor(240,230,140),
              MANUAL:       wxColor(180,238,180)}
   def get_color(self, image):
      return self._colors.get(
         image.classification_state, wxColor(200,200,200))

   # Draws one cell of the grid
   def Draw(self, grid, attr, dc, rect, row, col, isSelected):
      scaling = self.parent.scaling
      tmp_dc = wxMemoryDC()
      
      bitmap_no = row * grid.cols + col
      if bitmap_no < len(self.parent.list):
         image = self.parent.list[bitmap_no]
      else:
         image = None
      if image != None:
         # Fill the background
         color = self.get_color(image)
         dc.SetPen(wxTRANSPARENT_PEN)
         if isSelected:
            dc.SetBrush(wxBrush(wxBLACK, wxSOLID))
         else:
            dc.SetBrush(wxBrush(color, wxSOLID))
         dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height)
         # If the image is bigger than the cell, crop it. This used to be done
         # in wxPython, but it is more efficient to do it with Gamera SubImages
         scaled_image = None
         if scaling != 1.0:
            # Things are complicated by the ability to provide a global scaling
            # to all of the images in the grid. Here we handle the scaling and,
            # if necessary, we also do the cropping.
            height = ceil(image.nrows * scaling)
            width = ceil(image.ncols * scaling)
            if (height >= rect.GetHeight() or
                width >= rect.GetWidth()):
               # If the scaled version is going to still be too large to fit in
               # the grid cell, we crop it first and then scale it. We could
               # just scale the whole image and then crop that to the
               # appropriate size, but that could be very expensive. Instead we
               # figure out how big of a cropped image to create so that after
               # scaling it is the appropriate size.
               sub_height = min(int((rect.GetHeight() + 1) / scaling), image.nrows)
               sub_width = min(int((rect.GetWidth() + 1) / scaling), image.ncols)
               sub_image = image.subimage(
                  image.offset_y, image.offset_x, sub_height, sub_width)
               scaled_image = sub_image.resize(
                  int(ceil(sub_image.nrows * scaling)),
                  int(ceil(sub_image.ncols * scaling)), 0)
            else:
               # This is the easy case - just scale the image.
               scaled_image = image.resize(height, width, 0)
         else:
            # If we don't scale the image we can simply crop if the image is too
            # big to fit into the grid cell or otherwise do nothing.
            if (image.nrows >= rect.GetHeight() or
                image.ncols >= rect.GetWidth()):
               height = min(image.nrows, rect.GetHeight() + 1)
               width = min(image.ncols, rect.GetWidth() + 1)
               scaled_image = image.subimage(
                  image.offset_y, image.offset_x, height, width)
            else:
               scaled_image = image

         wx_image = wxEmptyImage(scaled_image.ncols, scaled_image.nrows)
         scaled_image.to_buffer(wx_image.GetDataBuffer())
         bmp = wx_image.ConvertToBitmap()
         # Display centered within the cell
         x = int(rect.x + (rect.width / 2) - (bmp.GetWidth() / 2))
         y = int(rect.y + (rect.height / 2) - (bmp.GetHeight() / 2))

         if isSelected:
            # This used to use dc.DrawBitmap, but the correct logical function
            # (wxSRC_INVERT) didn't seem to get used under Windows.
            tmp_dc.SelectObject(bmp)
            dc.Blit(x, y, bmp.GetWidth(), bmp.GetHeight(), tmp_dc, 0, 0,
                    wxSRC_INVERT)
         else:
            tmp_dc = wxMemoryDC()
            tmp_dc.SelectObject(bmp)
            dc.Blit(x, y, bmp.GetWidth(), bmp.GetHeight(), tmp_dc, 0, 0, wxAND)
         if self.parent.display_names:
            label = self.parent.get_label(image)
            if label != '':
               dc.SetBackgroundMode(wxTRANSPARENT)
               # The default font is too big under windows - this should
               # probably be a configurable option . . .
               dc.SetFont(wxFont(6, wxSWISS, wxNORMAL, wxNORMAL))
               # the the logical function is ignored for Windows, so we have
               # to set the Foreground and Background colors manually
               if isSelected:
                  dc.SetTextForeground(wxWHITE)
               else:
                  dc.SetTextForeground(wxBLACK)
               label = self.parent.reduce_label_length(dc, rect.width, label)
               dc.DrawText(label, rect.x, rect.y)
         if isSelected:
            dc.SetLogicalFunction(wxAND)
            dc.SetBrush(wxBrush(color, wxSOLID))
            dc.SetPen(wxTRANSPARENT_PEN)
            dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height)
      if image is None or hasattr(image, 'dead'):
         # If there's no image in this cell, draw a hatch pattern
         dc.SetLogicalFunction(wxCOPY)
         dc.SetBackgroundMode(wxSOLID)
         dc.SetPen(wxTRANSPARENT_PEN)
         if image is None:
            dc.SetBrush(wxWHITE_BRUSH)
            dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height)
            dc.SetBrush(wxBrush(wxBLUE, wxFDIAGONAL_HATCH))
         else:
            dc.SetBrush(wxBrush(wxRED, wxFDIAGONAL_HATCH))
         dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height)
      dc.SetLogicalFunction(wxCOPY)

   # The images should be a little padded within the cells
   # Also, there is a max size for every cell
   def GetBestSize(self, grid, attr, dc, row, col):
      bitmap_no = row * grid.cols + col
      if bitmap_no < len(self.parent.list):
         image = self.parent.list[bitmap_no]
      else:
         image = None
      if image != None:
         return wxSize(
            min(grid.max_cell_width,
                int(image.ncols * grid.scaling + grid.cell_padding)),
            min(grid.max_cell_height,
                int(image.nrows * grid.scaling + grid.cell_padding)))
      return wxSize(25, 25)

   def Clone(self):
      return MultiImageGridRenderer(self.parent)

# Grid constants
config.define_option(
   'grid', 'max_cell_width', 200,
   'Maximum width of a grid cell')
config.define_option(
   'grid', 'max_cell_height', 200,
   'Maximum height of a grid cell')
config.define_option(
   'grid', 'max_label_length', 200,
   'Maximum length (in pixels) of the row labels in the grid')
config.define_option(
   'grid', 'cell_padding', 20,
   'Amount of padding around the glyphs in the grid')
config.define_option(
   'grid', 'ncols', 8,
   'Number of columns in the grid')
class MultiImageDisplay(wxGrid):
   def __init__(self, parent = None, id = -1, size = wxDefaultSize):
      wxGrid.__init__(self, parent, id,
                      style=wxNO_FULL_REPAINT_ON_RESIZE|wxCLIP_CHILDREN)
      self.GetGridWindow().SetWindowStyle(
         wxNO_FULL_REPAINT_ON_RESIZE|wxCLIP_CHILDREN)
      self.list = []
      self.rows = 1
      self.cols = config.options.grid.ncols
      self.max_cell_width = config.options.grid.max_cell_width
      self.max_cell_height = config.options.grid.max_cell_height
      self.max_label_length = config.options.grid.max_label_length
      self.cell_padding = config.options.grid.cell_padding
      self.frame = parent
      self.updating = 0
      self.sort_function = ""
      self.display_attribute = ""
      self.display_names = 0
      self.created = 0
      self.do_updates = 0
      self.last_image_no = None
      self.scaling = 1.0
      if wxPlatform == '__WXMAC__':
        size = wxSize(300, 24)
      else:
        size = wxSize(175, 24)
      self.tooltip = wxButton(self.GetGridWindow(), -1, "",
                              wxPoint(0, 0), size)
      self.tooltip.Show(False)
      EVT_GRID_CELL_LEFT_DCLICK(self, self._OnLeftDoubleClick)
      EVT_GRID_CELL_RIGHT_CLICK(self, self._OnRightClick)
      EVT_GRID_SELECT_CELL(self, self._OnSelect)
      EVT_GRID_CELL_CHANGE(self, self._OnSelect)
      EVT_MOTION(self.GetGridWindow(), self._OnMotion)
      EVT_LEAVE_WINDOW(self.GetGridWindow(), self._OnLeave)
      self.renderer = MultiImageGridRenderer(self)
      self.SetDefaultRenderer(self.renderer)

   ########################################
   # BASIC UTILITY
   
   def set_image(self, list, view_function=None):
      wxBeginBusyCursor()
      self.BeginBatch()
      try:
         self.list = list
         self.do_updates = 0
         self.sort_images()
         self.frame.set_choices()
         if not self.created:
            self.rows = 1
            self.CreateGrid(1, self.cols)
            self.created = 1
         self.EnableEditing(0)
         self.resize_grid()
         self.ClearSelection()
         x = self.GetSize()
         self.do_updates = 1
      finally:
         self.EndBatch()
         wxEndBusyCursor()
      return (x.x, 600)

   if float(__wx_version__) < 2342:
      def AutoSize(self):
         # This should work correctly in wxPython itself, but it doesn't
         # (It causes random seqfaulting when it writes over the cache)
         # This function should works for versions where this is broken.
         wxBeginBusyCursor()
         self.BeginBatch()
         try:
            col_max = [25] * self.cols
            height = 0
            image_no = self.rows * self.cols - 1
            for row in xrange(self.rows - 1, -1, -1):
               row_max = 25
               for col in xrange(self.cols - 1, -1, -1):
                  if image_no < len(self.list):
                     image = self.list[image_no]
                     if not image is None:
                        row_max = max(
                           row_max,
                           min(self.max_cell_width,
                               image.nrows * self.scaling +
                               self.cell_padding))
                        col_max[col] = max(
                           col_max[col],
                           min(self.max_cell_height,
                               image.ncols * self.scaling +
                               self.cell_padding))
                  image_no -= 1
               self.SetRowSize(row, row_max)
               height += row_max
            width = 0
            for col in range(self.cols - 1, -1, -1):
               self.SetColSize(col, col_max[col])
               width += col_max[col]
         finally:
            self.EndBatch()
            self.GetGridWindow().SetVirtualSize(self.GetSize())
            self.GetGridWindow().Layout()
            self.GetGridWindow().GetParent().Layout()
            wxEndBusyCursor()
   else:
      def AutoSize(self):
         wxGrid.AutoSize(self)
         self.GetGridWindow().SetVirtualSize(self.GetSize())
         self.GetGridWindow().Layout()
         self.GetGridWindow().GetParent().Layout()

   def resize_grid(self, do_auto_size=1):
      if not self.created:
         return
      wxBeginBusyCursor()
      self.BeginBatch()
      try:
         start_row = self.GetGridCursorRow()
         start_col = self.GetGridCursorCol()
         orig_rows = self.rows
         rows = int(max(ceil(float(len(self.list)) / float(self.cols)), 1))
         cols = self.cols
         if rows < orig_rows:
            self.DeleteRows(0, orig_rows - rows)
         elif rows > orig_rows:
            self.AppendRows(rows - orig_rows)
         self.rows = rows
         self.cols = cols
         row_size = 1
         for row in range(rows - 1, -1, -1):
            for col in range(cols):
               self.SetCellRenderer(row, col, self.GetDefaultRenderer())
               self.SetReadOnly(row, col, True)
         width = self.set_labels()
         self.SetRowLabelSize(width + 20)
         self.SetColLabelSize(20)
         self.AutoSize()
         if start_row < self.GetNumberRows() and start_col < self.GetNumberCols():
            self.SetGridCursor(start_row, start_col)
            self.MakeCellVisible(start_row, start_col)
      finally:
         self.EndBatch()
         wxEndBusyCursor()

   def append_glyphs(self, list):
      wxBeginBusyCursor()
      self.BeginBatch()
      try:
         # Remove trailing 'None's
         for i in range(len(self.list) - 1, -1, -1):
            if not self.list[i] is None:
               break
         self.list = self.list[:i+1]
         self.list.extend(list)
         self.resize_grid(do_auto_size=1)
      finally:
         self.EndBatch()
         wxEndBusyCursor()

   def remove_glyphs(self, list):
      for glyph in list:
         glyph.dead = 1

   def append_and_remove_glyphs(self, add, remove):
      if len(add):
         self.append_glyphs(add)
      if len(remove):
         self.remove_glyphs(remove)
      #self.ForceRefresh()
      
   def scale(self, scaling):
      if self.scaling != scaling:
         self.scaling = scaling
         self.AutoSize()
         self.MakeCellVisible(
            self.GetGridCursorRow(), self.GetGridCursorCol())

   ########################################
   # SORTING

   def _sort_by_name_func(self, a, b):
      if a.id_name == [] and b.id_name == []:
         r = 0
      elif a.id_name == []:
         r = 1
      elif b.id_name == []:
         r = -1
      else:
         r = cmp(a.get_main_id(), b.get_main_id())
      if r == 0:
         r = cmp(b.classification_state, a.classification_state)
         if r == 0 and a.classification_state != UNCLASSIFIED:
            r = cmp(b.id_name[0][0], a.id_name[0][0])
      return r

   def _split_classified_from_unclassified(self, list):
      # Find split between classified and unclassified
      if not len(list):
         return [], []
      for i in range(len(list)):
         if list[i].classification_state == UNCLASSIFIED:
            break
      return list[:i], list[i:]

   def _insert_for_line_breaks(self, list):
      # Make sure each new label begins in a new row
      column = 0
      prev_id = -1
      new_list = []
      for image in list:
         main_id = image.get_main_id()
         if main_id != prev_id and column != 0:
            new_list.extend([None] * (self.cols - column))
            column = 0
         new_list.append(image)
         column += 1
         column %= self.cols
         prev_id = main_id
      return new_list

   # To minimize the size of the grid, we sort the images
   # first by height, and then within each row by width
   def _sort_by_size(self, list):
      list.sort(lambda x, y: cmp(x.nrows, y.nrows))
      outlist = []
      while len(list):
         if len(list) < self.cols:
            sublist = list; list = []
         else:
            sublist = list[:self.cols]; list = list[self.cols:]
         sublist.sort(lambda x, y: cmp(x.ncols, y.ncols))
         outlist.extend(sublist)
      return outlist

   def default_sort(self, list):
      # If we've done no classification, use the default sort from
      # MultiImageDisplay
      self.last_sort = "default"
      # mark that we want to display row labels
      self.display_row_labels = 1
      # Sort by label
      list.sort(self._sort_by_name_func)
      # Find split between classified and unclassified
      classified, unclassified = self._split_classified_from_unclassified(list)
      # Sort the unclassified by size
      unclassified = self._sort_by_size(unclassified)
      # Merge back together
      list = classified + unclassified
      # Make sure each new label begins in a new row
      new_list = self._insert_for_line_breaks(list)
      return new_list

   # Sorts the list of images by a given function, or the
   # default function if None is given
   def sort_images(self, function=None, order=0):
      wxBeginBusyCursor()
      self.BeginBatch()
      self.last_sort = None
      self.display_row_labels = not function
      orig_len = len(self.list)
      new_list = self.GetAllItems()
      if len(new_list) != len(self.list):
         self.list = new_list
      try:
         orig_len = len(self.list)
         if function != None:
            self.sort_function = function
         if self.sort_function == "":
            self.list = self.default_sort(self.list)
         else:
            self.list = self.GetAllItems()
            self.list.sort(self.sort_function)
            for item in self.list:
               del item.sort_cache
         if order:
            self.list.reverse()
         self.resize_grid()
         self.ClearSelection()
         self.MakeCellVisible(0, 0)
      finally:
         self.EndBatch()
         wxEndBusyCursor()

   def set_labels(self):
      self.BeginBatch()
      max_label = 1
      for i in range(self.cols):
         self.SetColLabelValue(i, "")
      dc = wxClientDC(self)
      for i in range(self.rows):
         try:
            image = self.list[i * self.cols]
         except IndexError:
            self.SetRowLabelValue(i, "")
         else:
            if image == None or image.classification_state == UNCLASSIFIED:
               self.SetRowLabelValue(i, "")
            elif self.display_row_labels:
               label = self.get_label(image)
               label = self.reduce_label_length(
                  dc, self.max_label_length * 0.6, label)
               max_label = max(dc.GetTextExtent(label)[0], max_label)
               self.SetRowLabelValue(i, label)
            else:
               max_label = max(dc.GetTextExtent("")[0], max_label)
               self.SetRowLabelValue(i, "")
      self.EndBatch()
      return min(max_label, self.max_label_length)

   ########################################
   # SELECTING

   def select_images(self, function):
      wxBeginBusyCursor()
      self.BeginBatch()
      try:
         self.updating = 1
         self.ClearSelection()
         for i in range(len(self.list)):
            x = self.list[i]
            if x != None:
             try:
                result = function(x)
             except Exception, err:
                gui_util.message(str(err))
                return
             if result:
                row = i / self.cols
                col = i % self.cols
                self.SelectBlock(
                   row, col, row, col, True)
         self.updating = 0
         # self.OnSelectImpl()
      finally:
         self.EndBatch()
         wxEndBusyCursor()

   ########################################
   # UTILITY FUNCTIONS

   def get_image_no(self, row, col):
      no =  row * self.cols + col
      if no >= len(self.list):
         return None
      else:
         return no

   def GetAllItems(self):
      return [x for x in self.list if x != None and not hasattr(x, 'dead')]

   def GetSelectedItems(self, row = None, col = None):
      if row != None:
         bitmap_no = self.get_image_no(row, col)
         # if bitmap_no != None:
         #   self.SetGridCursor(row, col)
      image = []
      if self.IsSelection():
         for row in range(self.rows):
            for col in range(self.cols):
               if self.IsInSelection(row, col):
                  index = self.get_image_no(row, col)
                  if index != None:
                     item = self.list[index]
                     if item != None:
                        image.append(item)
      elif row != None and bitmap_no != None:
         image = [self.list[bitmap_no]]
      return image

   def RefreshSelected(self, row = None, col = None):
      if row != None:
         bitmap_no = self.get_image_no(row, col)
         if bitmap_no != None:
            self.SetGridCursor(row, col)
      if self.IsSelection():
         for row in range(self.rows):
            for col in range(self.cols):
               if self.IsInSelection(row, col):
                  self.SetCellValue(row, col, "")
      elif row != None:
         self.SetCellValue(row, col, "")

   def SelectGlyphs(self, glyphs):
      matches = []
      for g in glyphs:
         for i, g2 in enumerate(self.list):
            if g is g2:
               matches.append(i)
      if len(matches):
         self.BeginBatch()
         first = 0
         self.updating = 1
         last_index = matches[-1]
         for index in matches:
            row = index / self.cols
            col = index % self.cols
            if index is last_index:
               self.updating = 0
            self.SelectBlock(row, col, row, col, first)
            if first == 0:
               self.MakeCellVisible(row, col)
               first = 1
         self.EndBatch()


   ########################################
   # CALLBACKS

   def ZoomOut(self):
      if self.scaling > pow(2, -4):
         self.scale(self.scaling * 0.5)

   def ZoomNorm(self):
      self.scale(1.0)

   def ZoomIn(self):
      if self.scaling < pow(2, 4):
         self.scale(self.scaling * 2.0)
   
   def _OnSelectImpl(self):
      pass

   def _OnSelect(self, event):
      image_no = self.get_image_no(event.GetRow(), event.GetCol())
      if image_no != None:
         event.Skip()
         self._OnSelectImpl()

   def _OnRightClick(self, event):
      row = event.GetRow()
      col = event.GetCol()
      images = self.GetSelectedItems(row, col)
      if images != None:
         position = event.GetPosition()
         image_menu.ImageMenu(self, position.x, position.y,
                              images, mode=0)
         self.ForceRefresh()

   def _OnLeftDoubleClick(self, event):
      bitmap_no = self.get_image_no(event.GetRow(), event.GetCol())
      if bitmap_no != None and self.list[bitmap_no] != None:
         self.list[bitmap_no].display()

   def get_label(self, glyph):
      if self.display_attribute == '':
         return glyph.get_main_id()
      else:
         try:
            label = str(
               eval(self.display_attribute,
                    {'x': glyph}))
         except:
            label = '<ERROR!>'
         return label

   def reduce_label_length(self, dc, width, label):
      extent = dc.GetTextExtent(label)
      while extent[0] > width:
         if len(label) < 5:
            label = '...'
            break
         label = "..." + label[5:]
         extent = dc.GetTextExtent(label)
      return label

   if wxPlatform == '__WXMAC__':
      _tooltip_extra = 32
   else:
      _tooltip_extra = 6
   def set_tooltip(self, label):
      #self.tooltip.SetLabel(label.decode('utf8'))
      self.tooltip.SetLabel(label)
      dc = wxClientDC(self.tooltip)
      extent = dc.GetTextExtent(label)
	
      self.tooltip.SetDimensions(
         -1,-1,extent[0]+self._tooltip_extra,extent[1]+6,wxSIZE_AUTO)

   def _OnMotion(self, event):
      origin = self.GetViewStart()
      units = self.GetScrollPixelsPerUnit()
      row = self.YToRow(event.GetY() + origin[1] * units[1])
      col = self.XToCol(event.GetX() + origin[0] * units[0])
      image_no = self.get_image_no(row, col)
      if image_no == None or image_no >= len(self.list) or image_no < 0:
         image = None
      else:
         image = self.list[image_no]

      if image == None:
         self.tooltip.Show(False)
         last_image_no = None
      else:
         self.tooltip.Show(1)
         if self.last_image_no != image_no:
            self.set_tooltip(self.get_label(image))
            self.last_image_no = image_no
         self.tooltip.Move(wxPoint(event.GetX() + 16, event.GetY() + 16))
      event.Skip()

   def display_label_at_cell(self, row, col, label):
      rect = self.CellToRect(row, col)
      origin = self.GetViewStart()
      units = self.GetScrollPixelsPerUnit()
      self.tooltip.Show()
      self.set_tooltip(label)
      self.tooltip.Move(wxPoint(rect.GetLeft() - origin[0] * units[0],
                                rect.GetBottom() - origin[1] * units[1] + 1))
 
   def _OnLeave(self, event):
      x, y = event.GetX(), event.GetY()
      w, h = self.GetSizeTuple()
      if x < 0 or x > w or y < 0 or y > h:
         self.tooltip.Hide()
      event.Skip()

class MultiImageWindow(wxPanel):
   def __init__(self, parent = None, id = -1, title = "Gamera", owner=None):
      from gamera.gui import gamera_icons
      wxPanel.__init__(self, parent, id,
                       style=wxNO_FULL_REPAINT_ON_RESIZE|wxCLIP_CHILDREN)
      self.SetAutoLayout(True)
      self.toolbar = toolbar.ToolBar(self, -1)

      self.toolbar.AddSimpleTool(
         10, gamera_icons.getIconRefreshBitmap(),
         "Refresh", self._OnRefreshClick)
      self.toolbar.AddSeparator()
      self.toolbar.AddSimpleTool(
         20, gamera_icons.getIconZoomInBitmap(),
         "Zoom in", self._OnZoomInClick)
      self.toolbar.AddSimpleTool(
         21, gamera_icons.getIconZoomNormBitmap(),
         "Zoom 100%", self._OnZoomNormClick)
      self.toolbar.AddSimpleTool(
         22, gamera_icons.getIconZoomOutBitmap(),
         "Zoom out", self._OnZoomOutClick)
      self.toolbar.AddSeparator()
      self.display_text_combo = wxComboBox(self.toolbar, 50, choices=[],
                                     size = wxSize(200, 20))
      EVT_COMBOBOX(self.display_text_combo, 50, self._OnChangeDisplayText)
      self.toolbar.AddControl(self.display_text_combo)
      self.toolbar.AddSimpleTool(
         24, gamera_icons.getIconShowNameBitmap(),
         "Display classes on grid", self._OnDisplayClasses, 1)
      self.toolbar2 = toolbar.ToolBar(self, -1)
      self.sort_combo = wxComboBox(self.toolbar2, 100, choices=[],
                                   size=wxSize(200, 20))
      self.toolbar2.AddControl(self.sort_combo)
      self.toolbar2.AddSimpleTool(
         101, gamera_icons.getIconSortAscBitmap(),
         "Sort Ascending", self._OnSortAscending)
      self.toolbar2.AddSimpleTool(
         102, gamera_icons.getIconSortDecBitmap(),
         "Sort Descending", self._OnSortDescending)
      self.toolbar2.AddSeparator()
      self.select_combo = wxComboBox(self.toolbar2, 103, choices=[],
                                     size=wxSize(200, 20))
      self.toolbar2.AddControl(self.select_combo)
      self.toolbar2.AddSimpleTool(
         104, gamera_icons.getIconSelectBitmap(),
         "Select by given expression", self._OnSelect)
      self.toolbar2.AddSimpleTool(
         105, gamera_icons.getIconSelectAllBitmap(),
         "Select All", self._OnSelectAll)
      self.select_choices = []
      lc = wxLayoutConstraints()
      lc.top.SameAs(self, wxTop, 0)
      lc.left.SameAs(self, wxLeft, 0)
      lc.right.SameAs(self, wxRight, 0)
      lc.height.AsIs()
      self.toolbar.SetAutoLayout(True)
      self.toolbar.SetConstraints(lc)
      lc = wxLayoutConstraints()
      lc.top.Below(self.toolbar, 0)
      lc.left.SameAs(self, wxLeft, 0)
      lc.right.SameAs(self, wxRight, 0)
      lc.height.AsIs()
      self.toolbar2.SetAutoLayout(True)
      self.toolbar2.SetConstraints(lc)
      self.id = self.get_display()
      lc = wxLayoutConstraints()
      lc.top.Below(self.toolbar2, 0)
      lc.left.SameAs(self, wxLeft, 0)
      lc.right.SameAs(self, wxRight, 0)
      lc.bottom.SameAs(self, wxBottom, 0)
      self.id.SetAutoLayout(True)
      self.id.SetConstraints(lc)
      self.Layout()
      self.sort_choices = []

   # This can be overridden to change the internal display class
   def get_display(self):
      return MultiImageDisplay(self)

   def set_choices(self):
      methods = [x[0] + "()" for x in
                 ImageBase.methods_flat_category("Features", ONEBIT)]
      methods.sort()

      self.display_choices = [
         "x.get_main_id()",
         "(x.offset_y, x.offset_x, x.nrows, x.ncols)",
         "x.label", "x.properties"]
      self.display_text_combo.Clear()
      for choice in self.display_choices:
         self.display_text_combo.Append(choice)

      self.sort_choices = [
         "", "offset_x", "offset_y",
         "ncols", "nrows",
         "label", "get_main_id()",
         "classification_state"]
      self.sort_combo.Clear()
      for choice in self.sort_choices:
         self.sort_combo.Append(choice)
      for method in methods:
         self.sort_combo.Append(method)

      self.select_choices = [
         "", "x.classification_state == 0",
         "x.classification_state == 1",
         "x.classification_state == 2",
         "x.classification_state == 3",
         "x.nrows < 2 or x.ncols < 2"]
      self.select_combo.Clear()
      for choice in self.select_choices:
         self.select_combo.Append(choice)

   def close(self):
      self.Destroy()

   ########################################
   # CALLBACKS

   def _OnRefreshClick(self, event):
      self.id.ForceRefresh()

   def _OnSortAscending(self, event, order=0):
      sort_string = string.strip(self.sort_combo.GetValue())
      if sort_string == '':
         self.id.sort_images('', order)
         return
      if sort_string.startswith('func'):
         split = string.split(sort_string)
         if len(split) >= 1:
            final = string.join(split[1:])
         else:
            gui_util.message(
               "The given sorting expressing contains a syntax error.")
            return
         try:
            sort_func = eval(final, globals(), image_menu.shell.locals)
         except Exception, e:
            gui_util.message(str(e))
            return
      else:
         error_messages = {}
         try:
            for image in self.id.GetAllItems():
               image.sort_cache = eval("x." + sort_string, {'x': image})
         except Exception, e:
            error_messages[str(e)] = None
            image.sort_cache = None
         sort_func = util.fast_cmp
         if len(error_messages):
            message = '\n\n'.join(error_messages.keys())
            gui_util.message(message)
      if sort_string not in self.sort_choices:
         self.sort_choices.append(sort_string)
         self.sort_combo.Append(sort_string)
      self.id.sort_images(sort_func, order)

   def _OnSortDescending(self, event):
      self._OnSortAscending(event, 1)

   def _OnSelect(self, event):
      select_string = string.strip(self.select_combo.GetValue())
      if select_string == "":
         return
      try:
         select_func = eval("lambda x: (" + select_string + ")", globals(),
                            image_menu.shell.locals)
      except Exception, e:
         gui_util.message(str(e))
         return
      if select_string not in self.select_choices:
         self.select_choices.append(select_string)
         self.select_combo.Append(select_string)
      self.id.select_images(select_func)

   def _OnSelectAll(self, event):
      wxBeginBusyCursor()
      try:
         self.id.SelectAll()
      finally:
         wxEndBusyCursor()

   def _OnSelectInvert(self, event):
      self.id.SelectInvert()

   def _OnZoomInClick(self, event):
      self.id.ZoomIn()

   def _OnZoomNormClick(self, event):
      self.id.ZoomNorm()

   def _OnZoomOutClick(self, event):
      self.id.ZoomOut()

   def _OnChangeDisplayText(self, event):
      value = self.display_text_combo.GetValue()
      self.id.display_attribute = value
      if value not in self.display_choices:
         self.display_choices.append(value)
         self.display_text_combo.Append(value)
      if self.id.display_names:
         self.id.ForceRefresh()

   def _OnDisplayDetails(self, event):
      self.id.display_details = event.GetIsDown()
      if self.id.display_names:
         self.id.Refresh()

   def _OnDisplayClasses(self, event):
      self.id.display_names = event.GetIsDown()
      self.id.Refresh()

##############################################################################
# TOP-LEVEL FRAMES
##############################################################################

class ImageFrameBase:
   def __init__(self, parent = None, id = -1, title = "Gamera", owner=None):
      self._frame = wxFrame(config.options.__.gui.TopLevel(), id, title,
                            wxDefaultPosition, (600, 400),
                            style=wxDEFAULT_FRAME_STYLE|wxCLIP_CHILDREN|
                            wxNO_FULL_REPAINT_ON_RESIZE)
      if (owner != None):
         self.owner = weakref.proxy(owner)
      else:
         self.owner = None
      EVT_CLOSE(self._frame, self._OnCloseWindow)

   def set_image(self, image, view_function=None, weak=1):
      size = self._iw.id.set_image(image, view_function, weak)
      self._frame.SetSize((max(200, min(600, size[0] + 30)),
                           max(200, min(400, size[1] + 60))))

   def close(self):
      self._iw.Destroy()
      self._frame.Destroy()

   def refresh(self):
      self._iw.id.refresh(0)

   def add_click_callback(self, cb):
      self._iw.id.add_click_callback(cb)

   def remove_click_callback(self, cb):
      self._iw.id.remove_click_callback(cb)

   def _OnCloseWindow(self, event):
      del self._iw
      if self.owner:
         self.owner.set_display(None)
      self._frame.Destroy()
      del self._frame

   def Show(self, flag=1):
      self._frame.Show(flag)
   show = Show

class ImageFrame(ImageFrameBase):
   def __init__(self, parent = None, id = -1, title = "Gamera", owner=None):
      ImageFrameBase.__init__(self, parent, id, title, owner)
      self._iw = ImageWindow(self._frame)
      from gamera.gui import gamera_icons
      icon = wxIconFromBitmap(gamera_icons.getIconImageBitmap())
      self._frame.SetIcon(icon)

   def __repr__(self):
      return "<ImageFrame Window>"

   def add_highlight_cc(self, cc, color):
      self._iw.id.add_highlight_cc(cc, color)
   add_highlight_ccs = add_highlight_cc

   def highlight_rectangle(self, y, x, h, w, c, t):
      self._iw.id.highlight_rectangle(y, x, h, w, c, t)

   def add_box(self, y, x, h, w):
      self._iw.id.add_box(y, x, h, w)

   def clear_all_boxes(self):
      self._iw.id.clear_all_boxes()

   def highlight_cc(self, cc):
      self._iw.id.highlight_cc(cc)
   highlight_ccs = highlight_cc

   def unhighlight_cc(self, cc):
      self._iw.id.unhighlight_cc(cc)
   unhighlight_ccs = unhighlight_cc

   def clear_all_highlights(self):
      self._iw.id.clear_all_highlights()

   def focus_glyphs(self, glyphs):
      self._iw.id.focus_glyphs(glyphs)


class MultiImageFrame(ImageFrameBase):
   def __init__(self, parent = None, id = -1, title = "Gamera", owner=None):
      ImageFrameBase.__init__(self, parent, id, title, owner)
      self._iw = MultiImageWindow(self._frame)
      from gamera.gui import gamera_icons
      icon = wxIconFromBitmap(gamera_icons.getIconImageListBitmap())
      self._frame.SetIcon(icon)

   def __repr__(self):
      return "<MultiImageFrame Window>"

   def set_image(self, image, view_function=None):
      size = self._iw.id.set_image(image, view_function)


##############################################################################
# GRAPHING
##############################################################################

# Draws a horizontal bar graph
def graph_horiz(data, dc, x1, y1, x2, y2, mark=None, border=1):
   scale_x = float(x2 - x1) / float(len(data))
   scale_y = (y2 - y1) / max(data)
   m = log(max(data))
   dc.SetPen(wxTRANSPARENT_PEN)
   light_blue = wxColor(128, 128, 255)
   for i in range(len(data)):
      datum = data[i] * scale_y
      dc.SetBrush(wxWHITE_BRUSH)
      dc.DrawRectangle(x1 + i * scale_x, y1,
                       scale_x + 1, y2 - y1)
      dc.SetBrush(wxBrush(light_blue, wxSOLID))
      dc.DrawRectangle(x1 + i * scale_x, y2 - datum,
                       scale_x + 1, datum)
   if mark:
      dc.SetBrush(wxCYAN_BRUSH)
      dc.SetLogicalFunction(wxXOR)
      dc.DrawRectangle(x1 + mark * scale_x, y1,
                       scale_x + 1, y2 - y1)
      dc.SetLogicalFunction(wxCOPY)
   if border:
      dc.SetPen(wxBLACK_PEN)
      dc.SetBrush(wxTRANSPARENT_BRUSH)
      dc.DrawRectangle(x1 - 1, y1 - 1, x2 - x1 + 2, y2 - y1 + 1)

# Draws a vertical bar graph
def graph_vert(data, dc, x1, y1, x2, y2, mark=None, border=1):
   scale_y = float(y2 - y1) / float(len(data))
   scale_x = (x2 - x1) / max(data)
   light_blue = wxColor(128, 128, 255)
   dc.SetPen(wxTRANSPARENT_PEN)
   for i in range(len(data)):
      datum = data[i] * scale_x
      dc.SetBrush(wxWHITE_BRUSH)
      dc.DrawRectangle(x1, y1 + i * scale_y, 
                       x2 - x1, scale_y + 1)
      dc.SetBrush(wxBrush(light_blue, wxSOLID))
      dc.DrawRectangle(x1, y1 + i * scale_y,
                       datum, scale_y + 1)
   if mark:
      dc.SetBrush(wxCYAN_BRUSH)
      dc.SetLogicalFunction(wxXOR)
      dc.DrawRectangle(x1, y1 + mark * scale_y,
                       x2 - x1, scale_y + 1)
      dc.SetLogicalFunction(wxCOPY)
   if border:
      dc.SetPen(wxBLACK_PEN)
      dc.SetBrush(wxTRANSPARENT_BRUSH)
      dc.DrawRectangle(x1 - 1, y1 - 1, x2 + 1 - x1, y2 - y1)

# Draws a grey-scale scale
def graph_scale(dc, x1, y1, x2, y2):
   scale_x = float(x2 - x1) / float(255)
   dc.SetPen(wxTRANSPARENT_PEN)
   for i in range(255):
      dc.SetBrush(wxBrush(wxColor(i, i, i), wxSOLID))
      dc.DrawRectangle(x1 + i * scale_x, y1,
                       scale_x + 1, y2 - y1)
   dc.SetPen(wxBLACK_PEN)
   dc.SetBrush(wxTRANSPARENT_BRUSH)
   dc.DrawRectangle(x1 - 1, y1 - 1, x2 - x1 + 2, y2 - y1 + 1)

# Clears an area of a wxDC
def clear_dc(dc):
   width = dc.GetSize().x
   height = dc.GetSize().y
   dc.SetBackgroundMode(wxSOLID)
   dc.SetBrush(wxBrush(wxWHITE, wxSOLID))
   dc.SetPen(wxTRANSPARENT_PEN)
   dc.DrawRectangle(0, 0, width, height)

HISTOGRAM_PAD = 10
class HistogramDisplay(wxFrame):
   def __init__(self, data=None, mark=None, parent=None,
                title="Histogram"):
      wxFrame.__init__(self, parent, -1, title,
                       style=wxRESIZE_BORDER|wxCAPTION)
      m = max(data)
      new_data = []
      for datum in data:
         if datum != 0:
            new_data.append(sqrt(datum))
      self.data = new_data
      self.mark = mark
      EVT_PAINT(self, self._OnPaint)

   def _OnPaint(self, event):
      dc = wxPaintDC(self)
      width = dc.GetSize().x
      height = dc.GetSize().y
      clear_dc(dc)
      graph_horiz(self.data, dc, HISTOGRAM_PAD, HISTOGRAM_PAD,
                  width - HISTOGRAM_PAD, height - HISTOGRAM_PAD * 1.5,
                  mark = self.mark)
      graph_scale(dc, HISTOGRAM_PAD, height - HISTOGRAM_PAD * 1.5,
                  width - HISTOGRAM_PAD, height - HISTOGRAM_PAD)


class ProjectionsDisplay(wxFrame):
   def __init__(self, x_data=None, y_data=None, image=None, parent=None, title="Projections"):
      size = (image.ncols + max(x_data) + (HISTOGRAM_PAD * 3),
              image.nrows + max(y_data) + (HISTOGRAM_PAD * 3))
      wxFrame.__init__(self, parent, -1, title,
                       style=wxCAPTION,
                       size=size)
      self.x_data = x_data
      self.y_data = y_data
      self.image = image
      EVT_PAINT(self, self._OnPaint)

   def _OnPaint(self, event):
      dc = wxPaintDC(self)
      clear_dc(dc)
      dc_width = dc.GetSize().x
      dc_height = dc.GetSize().y
      mat_width = self.image.ncols
      mat_height = self.image.nrows
      image = wxEmptyImage(self.image.ncols, self.image.nrows)
      self.image.to_buffer(image.GetDataBuffer())
      bmp = image.ConvertToBitmap()
      # Display centered within the cell
      x = HISTOGRAM_PAD * 2 + max(self.x_data)
      y = HISTOGRAM_PAD
      dc.DrawBitmap(bmp, x, y, 0)
      graph_vert(self.x_data, dc, HISTOGRAM_PAD, y,
                 HISTOGRAM_PAD + max(self.x_data), y + mat_height, border=0)
      graph_horiz(self.y_data, dc, x, y + mat_height + HISTOGRAM_PAD,
                  x + mat_width,
                  y + (mat_height + max(self.y_data)) + HISTOGRAM_PAD, border=0)

class ProjectionDisplay(wxFrame):
   def __init__(self, data, title="Projection"):
      wxFrame.__init__(self, -1, -1, title,
                       style=wxCAPTION,
                       size=((len(data) * 2) + (HISTOGRAM_PAD * 3),
                             max(data) + (HISTOGRAM_PAD * 3)))
      self.data = data
      EVT_PAINT(self, self._OnPaint)

   def _OnPaint(self, event):
      dc = wxPaintDC(self)
      clear_dc(dc)
      dc_width = dc.GetSize().x
      dc_height = dc.GetSize().y
      # Display centered within the cell
      x = (dc_width / 2) + (HISTOGRAM_PAD / 2)
      y = HISTOGRAM_PAD
      dc.DrawBitmap(bmp, x, y, 0)
      graph_horiz(self.data, dc, x, y + HISTOGRAM_PAD,
                  x + mat_width, y + HISTOGRAM_PAD, border=0)
