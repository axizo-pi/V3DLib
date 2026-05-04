#!/usr/bin/env ruby
#
# This is a reference app for RNN's.
#
# This program demonstrably works, so the C++ is adjusted to the working of this program.
#
# Adapted from: https://www.geeksforgeeks.org/numpy/implementation-of-neural-network-from-scratch-using-numpy/
#
##################################################################
require 'matrix'
require_relative 'nn/test'
require_relative 'nn/data'
require_relative 'nn/model'


####################
# Main
####################

# To compare pseudo-random values with C++
if false; disp_random; return; end

nn = NeuralNetwork.new

if false; test_back_prop; return; end

acc, losss = train($inputs, $desireds, nn, NumEpochs)

# Example: Predicting for letter 'B'
predict($inputs[0], nn)
predict($inputs[1], nn)
predict($inputs[2], nn)
