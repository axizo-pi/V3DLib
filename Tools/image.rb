#!/usr/bin/env ruby
# Install rmagick for ruby
#
#   sudo apt-get install libmagick++-dev libmagickcore-dev libmagickwand-dev
#   sudo gem install rmagick
#
# Man: https://rmagick.github.io/usage.html
###################################################
require 'rmagick'

list = Magick::ImageList.new #("*.pgm")

index = 0
filename = "#{index}_heatmap.pgm"

while File.exist?(filename) && (index < 200)
	#puts "#{filename} exists"
	list.read filename

	index += 1
	filename = "#{index}_heatmap.pgm"
end

list.display
puts "Displaying"
#list.animate

puts "Done"

