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

from threading import *
import sys, os
from gamera import core, util, config
from gamera.plugins import features
import gamera.knncore, gamera.gamera_xml
import array

from gamera.knncore import CITY_BLOCK
from gamera.knncore import EUCLIDEAN
from gamera.knncore import FAST_EUCLIDEAN

KNN_XML_FORMAT_VERSION = 1.0

config.define_option(
   "knn", "default_settings", "",
   "File name for the default kNN settings")

_distance_type_to_name = {
    CITY_BLOCK: "CITY-BLOCK",
    EUCLIDEAN: "EUCLIDEAN",
    FAST_EUCLIDEAN: "FAST-EUCLIDEAN" }

_distance_type_to_number = {
    "CITY-BLOCK": CITY_BLOCK,
    "EUCLIDEAN": EUCLIDEAN,
    "FAST-EUCLIDEAN": FAST_EUCLIDEAN }

class GaWorker(Thread):
   def __init__(self, knn):
      Thread.__init__(self)
      self.knn = knn

   def run(self):
      self.ga_initial = self.knn._ga_create()
      self.ga_best = self.ga_initial
      while(1):
         if self.knn.ga_worker_stop:
            return
         self.knn.ga_best = self.knn._ga_step()
         self.knn.ga_generation += 1
         for x in self.knn.ga_callbacks:
            x(self.knn)

# The kNN classifier stores it settings in a simple xml file -
# this class uses the gamera_xml.LoadXML class to load that
# file. After the file is loaded, kNN.load_settings extracts
# the data from the class to set up kNN.
class _KnnLoadXML(gamera.gamera_xml.LoadXML):
   def __init__(self):
      gamera.gamera_xml.LoadXML.__init__(self)

   def _setup_handlers(self):
      self.feature_functions = []
      self.weights = { }
      self.num_k = None
      self.distance_type = None
      self.ga_mutation = None
      self.ga_crossover = None
      self.ga_population = None
      self.add_start_element_handler('gamera-knn-settings', self._tag_start_knn_settings)
      self.add_start_element_handler('ga', self._tag_start_ga)
      self.add_start_element_handler('weights', self._tag_start_weights)
      self.add_end_element_handler('weights', self._tag_end_weights)

   def _remove_handlers(self):
      self.remove_start_element_handler('gamera-knn-settings')
      self.remove_start_element_handler('ga')
      self.remove_start_element_handler('weights')

   def _tag_start_knn_settings(self, a):
      version = self.try_type_convert(a, 'version', float, 'gamera-knn-settings')
      if version < KNN_XML_FORMAT_VERSION:
         raise gamera_xml.XMLError(
            "knn-settings XML file is an older version that can not be read by this version of Gamera.")
      self.num_k = self.try_type_convert(a, 'num-k', int, 'gamera-knn-settings')
      self.distance_type = \
        _distance_type_to_number[self.try_type_convert(a, 'distance-type',
                                                       str, 'gamera-knn-settings')]

   def _tag_start_ga(self, a):
      self.ga_mutation = self.try_type_convert(a, 'mutation', float, 'ga')
      self.ga_crossover = self.try_type_convert(a, 'crossover', float, 'ga')
      self.ga_population = self.try_type_convert(a, 'population', int, 'ga')

   def _tag_start_weights(self, a):
      self.add_start_element_handler('weight', self._tag_start_weight)
      self.add_end_element_handler('weight', self._tag_end_weight)

   def _tag_end_weights(self):
      self.remove_start_element_handler('weight')
      self.remove_end_element_handler('weight')

   def _tag_start_weight(self, a):
      self._data = u''
      self._weight_name = str(a["name"])
      self._parser.CharacterDataHandler = self._add_weights

   def _tag_end_weight(self):
      self._parser.CharacterDataHandler = None
      self.weights[self._weight_name] = array.array('d')
      nums = str(self._data).split()
      tmp = array.array('d', [float(x) for x in nums])
      self.weights[self._weight_name] = tmp

   def _add_weights(self, data):
      self._data += data

class kNN(gamera.knncore.kNN):
   """k-NN classifier that supports optimization using
   a Genetic Algorithm. This classifier supports all of
   the Gamera interactive/non-interactive classifier interface."""

   def __init__(self, features=None):
      """Constructor for knn object. Features is a list
      of feature names to use for classification. If features
      is none then the default settings will be loaded from a
      file specified in the user config file. If there is no
      settings file specified then all of the features will be
      used."""
      gamera.knncore.kNN.__init__(self)
      self.ga_initial = 0.0
      self.ga_best = 0.0
      self.ga_worker_thread = None
      self.ga_worker_stop = 0
      self.ga_generation = 0
      self.ga_callbacks = []
      self.features = features
      if self.features is None:
         settings = config.options.knn.default_settings
         try:
            print settings
            self.load_settings(settings)
         except Exception, e:
            self.features = 'all'
            self.change_feature_set(self.features)

   def change_feature_set(self, f):
      """Change the set of features used in the classifier.  features is a list of
      strings, naming the feature functions to be used."""
      self.features = f
      self.feature_functions = core.ImageBase.get_feature_functions(self.features)
      self.num_features = features.get_features_length(self.features)

   def distance_from_images(self, images, glyph, max):
      """Compute a list of distances between a list of images
      and a singe images. Distances greater than the max distance
      are not included in the output."""
      glyph.generate_features(self.feature_functions)
      progress = None
      for x in images:
         if progress == None and \
            x.feature_functions != self.feature_functions:
            progress = util.ProgressFactory("Generating Features . . .", len(images))
         x.generate_features(self.feature_functions)
         if progress:
            progress.step()
      if progress:
         progress.kill()
      return self._distance_from_images(iter(images), glyph, max)

   def distance_between_images(self, imagea, imageb):
      """Compute the distance between two images using the settings
      for the kNN object (distance_type, features, weights, etc). This
      can be used when more control over the distance computations are
      needed than with any of the other methods that work on multiple
      images at once."""
      imagea.generate_features(self.feature_functions)
      imageb.generate_features(self.feature_functions)
      return self._distance_between_images(imagea, imageb)

   def evaluate(self):
      """Evaluate the performance of the kNN classifier using
      leave-one-out cross-validation. The return value is a
      floating-point number between 0.0 and 1.0"""
      return self.leave_one_out()

   def supports_interactive(self):
      """Flag indicating that this classifier supports interactive
      classification."""
      return 1

   def supports_optimization(self):
      """Flag indicating that this classifier supports optimization."""
      return 1

   def start_optimizing(self):
      """Start optizing the classifier using a Genetic Algorithm. This
      function will not block. Instead it starts a background (daemon) thread that
      will perform the optimization in the background. While the classifier is
      performing classification no other methods should be called until stop_optimizing
      has been called."""
      self.ga_worker_stop = 0
      self.ga_worker_thread = GaWorker(self)
      self.ga_worker_thread.setDaemon(1)
      self.ga_worker_thread.start()
      return

   def stop_optimizing(self):
      """Stop optimization with the Genetic Algorithm. WARNING: this function
      has to wait for the current GA generation to finish before returning, which
      could take several secongs."""
      if not self.ga_worker_thread:
         return
      self.ga_worker_stop = 1
      self.ga_worker_thread.join()
      self.ga_worker_thread = None
      self._ga_destroy()

   def add_optimization_callback(self, func):
      """Add a function to be called everytime time the optimization updates the
      classifier."""
      self.ga_callbacks.append(func)

   def remove_optimization_callback(self, func):
      """Remove a function that is called everytime time the optimization updates the
      classifier."""
      try:
         self.ga_callbacks.remove(func)
      except:
         pass

   def supports_settings_dialog(self):
      """Flag to indicate that this classifier has a settings dialog"""
      return 1

   def settings_dialog(self, parent):
      """Display a settings dialog for k-NN settings"""
      from gamera import args
      dlg = args.Args([args.Int('k', range=(0, 100), default=self.num_k),
                       args.Choice('Distance Function',
                                   ['City block', 'Euclidean', 'Fast Euclidean'],
                                   default = self.distance_type)
                       ], name="kNN settings")
      results = dlg.show(parent)
      if results is None:
         return
      self.num_k, self.distance_type = results

   def save_settings(self, filename):
      """Save the k-NN settings to filename. This settings file (which is xml)
      includes k, distance type, GA mutation rate, GA crossover rate, GA population size,
      and the current floating point weights. This file is different from the one produced
      by serialize in that it contains only the settings and no data."""
      from util import word_wrap
      file = open(filename, "w")
      indent = 0
      word_wrap(file, '<?xml version="1.0" encoding="utf-8"?>', indent)
      word_wrap(file,
                '<gamera-knn-settings version="%s" num-k="%s" distance-type="%s">'
                % (KNN_XML_FORMAT_VERSION,
                   self.num_k,
                   _distance_type_to_name[self.distance_type]), indent)
      indent += 1
      word_wrap(file, '<ga mutation="%s" crossover="%s" population="%s"/>' %
                (self.ga_mutation, self.ga_crossover, self.ga_population), indent)
      if self.feature_functions != None:
         word_wrap(file, '<weights>', indent)
         indent += 1
         feature_no = 0
         weights = self.get_weights()
         for name, function in self.feature_functions:
            word_wrap(file, '<weight name="%s">' % name, indent)
            length = function.return_type.length
            word_wrap(file,
                      [x for x in
                       weights[feature_no:feature_no+length]],
                      indent + 1)
            word_wrap(file, '</weight>', indent)
            feature_no += length
         indent -= 1
         word_wrap(file, '</weights>', indent)
      indent -= 1
      word_wrap(file, '</gamera-knn-settings>', indent)
      file.close()

   def load_settings(self, filename):
      """Load the k-NN settings from an xml file. If settings file contains
      weights they are loading. Loading the weights can potentially change the
      feature functions of which the classifier is aware."""
      from gamera import core

      loader = _KnnLoadXML()
      loader.parse_filename(filename)
      self.num_k = loader.num_k
      self.distance_type = loader.distance_type
      self.ga_mutation = loader.ga_mutation
      self.ga_crossover = loader.ga_crossover
      self.ga_population = loader.ga_population
      # In order to correctly set the weights we need to look
      # up all of the feature functions for the individual weights
      # so that we can set self.feature_functions. This allows the
      # classification methods to know whether the weights should
      # be used or not (according to whether the features on the
      # image passed in match the features we currently know about).
      functions = []
      for key in loader.weights:
         try:
            func = core.ImageBase.get_feature_functions(key)
         except Exception, e:
            raise gamera.gamera_xml.XMLError("While loading the weights " +
            "an unknown feature function '%s' was found." % key)
         functions.append(func[0])
      functions.sort()
      self.change_feature_set(functions)
      # Create the weights array with the weights in the correct order
      weights = array.array('d')
      for x in self.feature_functions:
         weights.extend(loader.weights[x[0]])
      self.set_weights(weights)

   def serialize(self, filename):
      """Save the k-NN settings and data into an optimized, k-NN specific
      file format."""
      if self.features == 'all':
         gamera.knncore.kNN.serialize(self, filename,['all'])
      else:
         gamera.knncore.kNN.serialize(self, filename,self.features)


   def unserialize(self, filename):
      """Load the k-NN settings and data from an optimized, k-NN specific
      file format"""
      features = gamera.knncore.kNN.unserialize(self, filename)
      if len(features) == 1 and features[0] == 'all':
         self.change_feature_set('all')
      else:
         self.change_feature_set(features)


def simple_feature_selector(glyphs):
   """simple_feature_selector does a brute-force search through all
   possible combinations of features and returns a sorted list of
   tuples (accuracy, features). WARNING: this function should take
   a long time to complete."""

   import classify
   if len(glyphs <= 1):
      raise RuntimeError("Lenght of list must be greater than 1")
   
   c = classify.NonInteractiveClassifier()

   all_features = []
   feature_indexes = {}
   offset = 0
   for x in glyphs[0].get_feature_functions():
      all_features.append(x[0])
      feature_indexes[x[0]] = range(offset, offset + x[1].return_type.length)
      offset += x[1].return_type.length
   # First do the easy ones = single features and all features
   for x in feature_indexes.values():
      print x
   answers = []

   c.change_feature_set(all_features)
   c.set_glyphs(glyphs)
   answers.append((c.evaluate(), all_features))
   for x in all_features:
      print x
      answers.append((c.classifier.leave_one_out(feature_indexes[x]), x))
   for i in range(2, len(all_features) - 1):
      for x in CombGen(all_features, i):
         print x
         indexes = []
         for y in x:
            indexes.extend(feature_indexes[y])
         answers.append((c.classifier.leave_one_out(indexes), x))
   answers.sort().reverse()
   return answers
   

class CombGen:
   """Generate the k-combinations of a sequence. This is a iterator
   that generates the combinations on the fly. This code was adapted
   from a posting by Tim Peters"""
   def __init__(self, seq, k):
      n = self.n = len(seq)
      if not 1 <= k <= n:
         raise ValueError("k must be in 1.." + `n` + ": " + `k`)
      self.k = k
      self.seq = seq
      self.indices = range(k)
      # Trickery to make the first .next() call work.
      self.indices[-1] = self.indices[-1] - 1

   def __iter__(self):
      return self

   def next(self):
      n, k, indices = self.n, self.k, self.indices
      lasti, limit = k-1, n-1
      while lasti >= 0 and indices[lasti] == limit:
         lasti = lasti - 1
         limit = limit - 1
      if lasti < 0:
         raise StopIteration
      newroot = indices[lasti] + 1
      indices[lasti:] = range(newroot, newroot + k - lasti)
      # Build the result.
      result = []
      seq = self.seq
      for i in indices:
         result.append(seq[i])
      return result
