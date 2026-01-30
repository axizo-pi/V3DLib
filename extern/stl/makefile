#
# To get all external includes under dir `drivers` with path `include`
#
#     grep -r "#include <linux/.*>" drivers | sed "s/^.*:#include </include\//" | sed "s/>.*$//" | sort | uniq | xargs git checkout master --
#
# This assumes -I./include
#
# ------------------------------------------------------------------------------
# 
# * The v3d simulator is **closed source**. You need to register with MESA to use it.
#
#   So there is no point in enabling `-DUSE_V3D_SIMULATOR`
#
# * Compute shaders are only used for versions >= 41
#
#   See `v3d_launch_grid()` in `gallium/drivers/v3d/v3dX_draw.c`
#
################################################################################
.DEFAULT_GOAL := all

SRCDIR = src
BASEDIR = ./obj
OBJDIR = $(BASEDIR)/src
BINDIR = $(BASEDIR)/bin

XX_FLAGS = -MMD -MP -MF"$(@:%.o=%.d)"

INCLUDES =

CFLAGS = -g -Wall -Wno-unused-function -O3 -pthread \
$(INCLUDES) \
$(XX_FLAGS) \
-Wno-sign-compare

LINK= g++


################################################################################
# Source Files
################################################################################

# Get Only the Internal Structure of SRCDIR Recursively
STRUCTURE := $(shell find $(SRCDIR) -type d)     
#$(info structure: $(STRUCTURE))

# Get All Files inside the STRUCTURE Variable
CODEFILES := $(addsuffix /*,$(STRUCTURE))
CODEFILES := $(wildcard $(CODEFILES))            
#$(info codefiles: $(CODEFILES))

# Filter Only Specific Files                                
SRCFILES := $(filter %.c %.cpp,$(CODEFILES))
#$(info  src: $(SRCFILES))
HDRFILES := $(filter %.h,$(CODEFILES))
OBJFILES := $(subst $(SRCDIR),$(OBJDIR)$(SRC_DIR),$(SRCFILES))
OBJFILES := $(OBJFILES:%.cpp=%.o)
#$(info  obj: $(OBJFILES))

# Dependencies from list of object files
DEPS := $(OBJFILES:.o=.d)
#$(info deps:  $(DEPS))
-include $(DEPS)


################################################################################
# Rules
################################################################################

all: init $(BINDIR)/stl

init:
	@mkdir -p $(OBJDIR)
	@mkdir -p $(BINDIR)

clean:
	rm -rf $(BASEDIR)/*


$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@echo Compiling $<
	@mkdir -p $(@D)
	@g++ $(CFLAGS) -c $< -o $@


$(BINDIR)/stl: $(OBJFILES)
	@echo Linking $@...
	@$(LINK) -L $(BINDIR) $^ -o $@
