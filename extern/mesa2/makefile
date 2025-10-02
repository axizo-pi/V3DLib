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
SRCDIR = src
BASEDIR = ../../obj/mesa2
OBJDIR = $(BASEDIR)/src
BINDIR = $(BASEDIR)/bin
MAINDIR = $(BASEDIR)/main

XX_FLAGS = -MMD -MP -MF"$(@:%.o=%.d)"

MESA_LIB=$(BINDIR)/libmesa2.a

INCLUDES = \
-I ./src/ \
-I ./src/broadcom/ \
-I ./include/ \
-I ./src/util/ \
-I ./build/src/ \
-I ./build/src/compiler/ \
-I ./build/src/compiler/nir/ \
-I ./src/compiler/ \
-I ./src/gallium/include/ \
-I ./src/gallium/auxiliary/ \
-I ./include/drm-uapi \
-I ./src/gallium/drivers/vc4 \


CFLAGS = -g -Wall -Wno-unused-function -O3 -pthread \
-D V3D_VERSION=71 \
-D HAVE_ENDIAN_H \
-D HAVE_PTHREAD \
-D HAVE_STRUCT_TIMESPEC \
-D DETECT_OS_LINUX \
-D HAVE_SYSCONF \
-D GPUVIS_TRACE_UTILS_DISABLE \
-U HAVE_PERFETTO \
-D HAVE_FUNC_ATTRIBUTE_PACKED \
$(INCLUDES) \
$(XX_FLAGS)


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

# Filter Only Specific Files                                
SRCFILES := $(filter %.c %.cpp,$(CODEFILES))
#$(info  src: $(SRCFILES))
HDRFILES := $(filter %.h,$(CODEFILES))
OBJFILES := $(subst $(SRCDIR),$(OBJDIR)$(SRC_DIR),$(SRCFILES))
OBJFILES := $(OBJFILES:%.cpp=%.o)
OBJFILES := $(OBJFILES:%.c=%.o)
#OBJFILES := $(OBJDIR)/$(SRCFILES:%.c=%.o)
#$(info  obj: $(OBJFILES))

DEPEND_FILES := $(addprefix $(SRCDIR)/,%.c %.h)
#$(info depend: $(DEPEND_FILES))


#LIB = $(patsubst %,$(OBJDIR)/%,$(OBJFILES))
LIB = $(OBJFILES)
#$(info lib:  $(LIB))

# Dependencies from list of object files
DEPS := $(LIB:.o=.d)
#$(info deps:  $(DEPS))
-include $(DEPS)


################################################################################
# Rules
################################################################################

#compile: init $(BINDIR)/qpu_disasm
compile: init $(MESA_LIB)

init:
	@mkdir -p $(OBJDIR)
	@mkdir -p $(BINDIR)
	@mkdir -p $(MAINDIR)

clean:
	rm -rf $(OBJDIR)/*
	rm -rf $(BINDIR)/*
	rm -rf $(MAINDIR)/*


$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@echo Compiling $<
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@echo Compiling $<
	mkdir -p $(@D)
	g++ $(CFLAGS) -c $< -o $@

$(MESA_LIB): $(OBJFILES)
	@echo Creating $@
	mkdir -p $(@D)
	@ar rcs $@ $^


$(MAINDIR)/%.o: main/%.c
	@echo Compiling $<
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BINDIR)/%: $(MAINDIR)/%.o $(MESA_LIB)
	@echo Linking $@...
	$(LINK) -L $(BINDIR) $^ -o $@
