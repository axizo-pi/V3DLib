#
# Before using make, you need to create the file dependencies:
#
# > script/gen.sh
#
# There are four builds possible, with output directories:
#
#   obj/emu          - using emulator
#   obj/emu-debug    - output debug info, using emulator
#   obj/qpu          - using hardware
#   obj/qpu-debug    - output debug info, using hardware
#
#==============================================================================
# NOTES
# =====
#
# * valgrind goes into a fit when used on `runTests`:
# 
#    --8346-- WARNING: Serious error when reading debug info
#    --8346-- When reading debug info from /lib/arm-linux-gnueabihf/ld-2.28.so:
#    --8346-- Ignoring non-Dwarf2/3/4 block in .debug_info
#    --8346-- WARNING: Serious error when reading debug info
#    --8346-- When reading debug info from /lib/arm-linux-gnueabihf/ld-2.28.so:
#    --8346-- Last block truncated in .debug_info; ignoring
#    ==8346== Conditional jump or move depends on uninitialised value(s)
#    ==8346==    at 0x401A5D0: index (in /lib/arm-linux-gnueabihf/ld-2.28.so)
#    ...
#
#  This has to do with valgrind not being able to read compressed debug info.
#  Note that this happens in system libraries.
#
#  Tried countering this with compiler options:
#
#  * -Wa,--nocompress-debug-sections -Wl,--compress-debug-sections=none
#  * -gz=none
#
#  ...with no effect. This is somewhat logical, the system libraries are
#  pre-compiled.
#
###############################################################################

QPU=1
DEBUG=1

ifeq ($(QPU), 1)
ifeq ($(DEBUG), 1)
$(info Using default values QPU=1 and DEBUG=1)
$(info To override, call make with `make (-e) QPU=0` and/or `DEBUG=0`)
endif
endif

## sudo required for QPU-mode on Pi
ifeq ($(QPU), 1)
	SUDO := sudo
else
	SUDO :=
endif

-include script/mak/config.mak
-include sources.mk

LIB = $(patsubst %,$(OBJDIR)/Lib/%,$(OBJ))

EXAMPLE_TARGETS = $(patsubst %,$(OBJDIR)/bin/%,$(EXAMPLES))
TESTS_OBJ = $(patsubst %,$(OBJDIR)/%,$(TESTS_FILES))

#
# Dependencies from list of object files
#
-include $(LIB:.o=.d)
-include $(TESTS_OBJ:.o=.d)

.PHONY: help clean all lib test $(EXAMPLES) init

help:
	@echo 'Usage:'
	@echo
	@echo '    make [target]* [QPU=1] [DEBUG=1]'
	@echo
	@echo 'Where target:'
	@echo
	@echo '    help          - Show this text'
	@echo '    all           - Build all example programs'
	@echo '    clean         - Delete all interim and target files'
	@echo '    test          - Run the unit tests'
	@echo '    gen           - Create source dependencies for the makefile'
	@echo
	@echo '    one of the example programs - $(EXAMPLES)'
	@echo
	@echo 'Flags:'
	@echo
	@echo '    QPU=1         - Output code for hardware. If not specified, the code is compiled for the emulator'
	@echo '    DEBUG=1       - If specified, the source code and target code is shown on stdout when running a test'
	@echo

all: $(V3DLIB) $(EXAMPLES) ${SUB_PROJECTS}

clean:
	@rm -rf obj/emu obj/emu-debug obj/qpu obj/qpu-debug obj/test

init:
	@./script/install.sh
	@mkdir -p $(OBJDIR)/bin 

# Part of init:
#@./script/detect_tabs.sh   - This is for github, which has awkward tabs for displaying source code

#
# Targets for static library
#

$(V3DLIB): $(LIB) $(MESA_LIB) $(VCSM_LIB)
	@echo Creating $@
	@ar rcs $@ $^

$(MESA_LIB):
	cd extern/mesa && make compile

$(VCSM_LIB):
	cd extern/userland/ && make

$(OBJDIR)/bin/%: $(OBJDIR)/Tools/%.o $(V3DLIB) $(LIB_DEPEND)
	@echo Linking $@...
	@mkdir -p $(@D)
	@$(LINK) $^ -o $@ $(LIBS)


$(EXAMPLES) :% : $(OBJDIR)/bin/%


#
# Targets for Unit Tests
#

UNIT_TESTS := $(OBJDIR)/bin/runTests

$(UNIT_TESTS): $(TESTS_OBJ) $(V3DLIB) $(LIB_DEPEND)
	@echo Linking unit tests
	@mkdir -p $(@D)
	@$(LINK) $^ -o $@ $(LIBS)


runTests: $(UNIT_TESTS)


make_test: runTests ID Hello Rot3D ReqRecv GCD Tri detectPlatform OET 

#
# Running unit test [fft][test2] in combination with the rest *sometimes* results in IO timeouts (Pi4 64bit).
# Running it separately appears to work fine.
# The infuriating bit is 'sometimes'.
#
# [rot3d] is similar, works fine when run separately, *sometimes* issues otherwise.
# I am incline to believe this is an issue with doctest.
#
# "*[pass*" - sic, intentional
#
test : make_test
	@echo Running unit tests with \'$(SUDO) $(UNIT_TESTS)\'
	@$(SUDO) $(UNIT_TESTS) -tce="*[pass*"
	@$(SUDO) $(UNIT_TESTS) -tc="*[pass2]*"
	@$(SUDO) $(UNIT_TESTS) -tc="*[pass3]*"


###############################
# Gen stuff
################################

gen : $(OBJDIR)/sources.mk

$(OBJDIR)/sources.mk :
	@mkdir -p $(@D)
	@script/gen.sh
