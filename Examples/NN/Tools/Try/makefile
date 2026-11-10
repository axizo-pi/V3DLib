# =* required, otherwise preceding whitespace part of value
SRCDIR  =./
BASEDIR =./obj
OBJDIR  =$(BASEDIR)/src
BINDIR  =$(BASEDIR)/bin

V3DLIB =/home/wim/projects/V3DLib

INCLUDES = \
-I ../eigen3/        \
-I ../nnetcpp/src/   \
-I ../nnetcpp/tests/ \
-I $(V3DLIB)/Lib/
$(info INCLUDES: $(INCLUDES))

LIBS = \
../nnetcpp/obj/bin/libnnetcpp.a \
$(V3DLIB)/obj/debug/libv3dlib.a

TARGET=$(BINDIR)/try

# Needs to be before include
all: init $(TARGET)

-include ../../../../script/mak/extern.mak
$(info OBJFILES: $(OBJFILES))

# Disabled warnings (except first) for Eigen3
CPPFLAGS += \
-DDEBUG                      \
-Wno-sign-compare            \
-Wno-deprecated-declarations \
-Wno-misleading-indentation  \
-Wno-int-in-bool-context     \
$(INCLUDES)
$(info CPPFLAGS: $(CPPFLAGS))


init:
	@mkdir -p $(OBJDIR)
	@mkdir -p $(BINDIR)

clean:
	rm -rf $(BASEDIR)/*

$(OBJDIR)/%.o: %.cpp
	@echo Compiling $<
	@mkdir -p $(@D)
	@g++ $(CPPFLAGS) -c $< -o $@

$(TARGET) : $(OBJFILES) $(LIBS)
	@echo Linking $@...
	@$(LINK) $^ -o $@
