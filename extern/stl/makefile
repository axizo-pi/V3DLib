.DEFAULT_GOAL := all

SRCDIR = src
BASEDIR = ./obj
OBJDIR = $(BASEDIR)/src
BINDIR = $(BASEDIR)/bin

STL_LIB=$(BINDIR)/libstl.a

INCLUDES =

-include ../../script/mak/extern.mak

CFLAGS += \
-Wno-sign-compare \
$(INCLUDES)


################################################################################
# Rules
################################################################################

all: init $(BINDIR)/stl $(STL_LIB)

init:
	@mkdir -p $(OBJDIR)
	@mkdir -p $(BINDIR)

clean:
	rm -rf $(BASEDIR)/*


$(STL_LIB): $(OBJFILES)
	@echo Creating $@
	@mkdir -p $(@D)
	@ar rcs $@ $^


$(BINDIR)/stl: $(OBJFILES)
	@echo Linking $@...
	@$(LINK) -L $(BINDIR) $^ -o $@
