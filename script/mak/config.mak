BASE=~/projects/V3DLib

ROOT=$(BASE)/Lib

# Object directory
OBJDIR := ${BASE}/obj

USERLAND_DIR=$(BASE)/extern/userland/build/lib
MESA_LIB=$(BASE)/obj/mesa/bin/libmesa.a
VCSM_LIB=$(USERLAND_DIR)/libvcsm.a

INCLUDE_EXTERN+= \
 -I $(BASE)/../CmdParameter/Lib \
 -I $(BASE)/extern/bitmap

LIB_EXTERN+= \
 -L $(USERLAND_DIR) -lvcsm \
 -l pthread

# For bcm_host shared lib; actually vc4 only
ifeq ($(QPU), 1)

INCLUDE_EXTERN+= \
 -I $(BASE)/extern/userland/host_applications/linux/libs/bcm_host/include \
 -I $(BASE)/extern/userland/

LIB_EXTERN+= \
 -l bcm_host

endif



# NOTE: Last items after single \ required in mesa lib include files

#
# -I is for access to bcm functionality
#
# -Wno-psabi avoids following note (happens in unit tests):
#
#    note: parameter passing for argument of type ‘std::move_iterator<Catch::SectionEndInfo*>’ changed in GCC 7.1
#
#    It is benign: https://stackoverflow.com/a/48149400
#
CXX_FLAGS = \
 -Wall \
 -Wconversion \
 -Wno-psabi \
 -pthread \
 -I $(ROOT) $(INCLUDE_EXTERN) \
 \
 -D HAVE_ENDIAN_H \
 -D HAVE_PTHREAD \
 -D MESA_DEBUG=0 \
 -D HAVE_SECURE_GETENV \
 -D HAVE_STRUCT_TIMESPEC

# NOTE: Last items after single \ required in mesa lib include files

CXX= g++
LINK= g++



# QPU or emulation mode
ifeq ($(QPU), 1)
$(info Building for QPU)

# Check platform before building.
# Can't be indented, otherwise make complains.
RET := $(shell ${BASE}/Tools/detectPlatform.sh 1>/dev/null && echo "yes" || echo "no")
#$(info  info: '$(RET)')
ifneq ($(RET), yes)
$(error QPU-mode specified on a non-Pi platform; aborting)
else
$(info Building on a Pi platform)
endif

  CXX_FLAGS += -DQPU_MODE -I /opt/vc/include
  OBJDIR := $(OBJDIR)/qpu
	LIBS += -L /opt/vc/lib -l bcm_host
else
  OBJDIR := $(OBJDIR)/emu
endif

# Debug mode
ifeq ($(DEBUG), 1)
  CXX_FLAGS += -DDEBUG -g
  OBJDIR := $(OBJDIR)-debug
else
  # -DNDEBUG	disables assertions
  # -g0 still adds debug info, using -s instead
  CXX_FLAGS += -DNDEBUG -s
endif

ifeq ($(DEBUG), 1)
	LIB_EXTERN += -L ${BASE}/../CmdParameter/obj-debug -lCmdParameter
	LIB_DEPEND += ${BASE}/../CmdParameter/obj-debug/libCmdParameter.a
else
	LIB_EXTERN += -L ${BASE}/../CmdParameter/obj -lCmdParameter
	LIB_DEPEND += ${BASE}/../CmdParameter/obj/libCmdParameter.a
endif


INCLUDE_EXTERN+= \
 -I ${BASE}/extern/mesa/include \
 -I ${BASE}/extern/mesa/src \
 -I ${BASE}/extern/mesa/src/gallium/include \
 -I ${BASE}/extern/mesa/src/gallium/auxiliary \
 -I ${BASE}/extern/mesa/build/src

INCLUDE= \
 -I ${BASE}/Lib \
 $(INCLUDE_EXTERN)

# NOTE: Spaces between line and comment is taken into the generation!
# Order important! drm MUST be after mesa, because mesa has dependencies on drm
LIB_EXTERN+= \
 -L $(BASE)/obj/mesa/bin -lmesa \
 -lstdc++fs \
 -ldrm

V3DLIB=$(OBJDIR)/libv3dlib.a

LIBS += -L $(OBJDIR) -lv3dlib $(LIB_EXTERN)


# Following prevents deletion of object files after linking
# Otherwise, deletion happens for targets of the form '%.o'
.PRECIOUS: $(OBJDIR)/%.o

# Rule for creating c++ object files
$(OBJDIR)/%.o: %.cpp | init
	@echo Compiling $<
	@mkdir -p $(@D)
	@$(CXX) -std=c++17 -c -o $@ $< $(CXX_FLAGS) -MMD -MP -MF"$(@:%.o=%.d)" $(INCLUDE) 


# Same thing for C-files
$(OBJDIR)/%.o: %.c | init
	@echo Compiling $<
	@mkdir -p $(@D)
	@$(CXX) -x c -c -o $@ $< $(CXX_FLAGS) $(INCLUDE)

$(OBJDIR)/bin/%: $(OBJDIR)/Examples/%.o $(V3DLIB) $(LIB_DEPEND)
	@echo Linking $@...
	@mkdir -p $(@D)
	@$(LINK) $^ -o $@ $(LIBS)


