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
BASEDIR = ../../obj/mesa
OBJDIR = $(BASEDIR)/src
BINDIR = $(BASEDIR)/bin
MAINDIR = $(BASEDIR)/main

MESA_LIB=$(BINDIR)/libmesa.a

-include ../../script/mak/extern.mak

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


CFLAGS += \
-D V3D_VERSION=71 \
-D HAVE_ENDIAN_H \
-D HAVE_PTHREAD \
-D HAVE_STRUCT_TIMESPEC \
-D DETECT_OS_LINUX \
-D HAVE_SYSCONF \
-D GPUVIS_TRACE_UTILS_DISABLE \
-U HAVE_PERFETTO \
-D HAVE_FUNC_ATTRIBUTE_PACKED \
-D USE_V3D_SIMULATOR=0 \
-D MESA_DEBUG=0 \
$(INCLUDES)

#-D HAVE_SECURE_GETENV \


################################################################################
# Rules
################################################################################

compile: init $(MESA_LIB)

init:
	@mkdir -p $(OBJDIR)
	@mkdir -p $(BINDIR)
	@mkdir -p $(MAINDIR)

clean:
	rm -rf $(OBJDIR)/*
	rm -rf $(BINDIR)/*
	rm -rf $(MAINDIR)/*


$(MESA_LIB): $(OBJFILES)
	@echo Creating $@
	@mkdir -p $(@D)
	@ar rcs $@ $^


$(MAINDIR)/%.o: main/%.c
	@echo Compiling $<
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@


$(BINDIR)/%: $(MAINDIR)/%.o $(MESA_LIB)
	@echo Linking $@...
	$(LINK) -L $(BINDIR) $^ -o $@
