#!/usr/bin/env ruby
# coding: utf-8
#
# Generate the pages for the web server.
#
#########################################################
WebGen = '../../WebGen'
require_relative "#{WebGen}/WebController"

GenLog.log_level 'info'


TARGET_DIR   = '../generated/website'
TEMPLATE_DIR = '../template'
TMP_DIR      = '../generated/tmp'

#
# Association of template with controller item is analogous to RoR.
#
#
# All methods with a '_' prefix are ignored in generation,
# the rest are all called.
#
# Normally, the method name corresponds with a template name.
# The corresponding templates should be named '<method_name>.erb'
#
# See method 'generate' for an override.
#
class GenPages < WebController

	#
	# Generate basic website pages
	#
	def generate
		add_source_dir "../Doc"

    @html_output_subdir = ''

	  [
			"Basics.md",
			"BuildInstructions.md",
			"Examples.md",
			"FAQ.md",
			"GettingStarted.md",
			"Issues.md",
			"Profiling.md",
			"TODO.md"
  	].each { |f|
    	puts "generating page for #{f}"
      init_inline_parts
      gen_method f
		}
		false  # Avoid direct generation
	end


  def initialize
		@base_output_dir = TARGET_DIR
		super
		add_source_dir TEMPLATE_DIR
  end
end


c = GenPages.new
c.gen []


#
# Copy assets
#

dirs = [
  # Source                                          Target subdir
  # ======                                          =============
  [ "../Doc/images",                                ""]
]

files = []


dirs.each { |d|
  FileUtils.mkdir_p TARGET_DIR + d[1]
  FileUtils.cp_r Dir.glob(d[0]), TARGET_DIR + d[1]
}


files.each { |d|
  FileUtils.cp d[0], TARGET_DIR + d[1]
}
