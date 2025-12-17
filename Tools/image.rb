#!/usr/bin/env ruby
# Install rmagick for ruby
#
#   sudo apt-get install libmagick++-dev libmagickcore-dev libmagickwand-dev
#   sudo gem install rmagick
#
# Man: https://rmagick.github.io/usage.html
###################################################
require 'rmagick'

cat = Magick::ImageList.new("mandelbrot.pgm")
cat.display

puts "Done"

