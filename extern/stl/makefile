.DEFAULT_GOAL := all

SRCDIR  = src
BASEDIR = ../../obj/stl
OBJDIR  = $(BASEDIR)/src
BINDIR  = $(BASEDIR)/bin
MAINDIR = $(BASEDIR)/main

STL_LIB=$(BINDIR)/libstl.a

INCLUDES = \
-I ./src/

-include ../../script/mak/extern.mak

CPPFLAGS += -Wno-sign-compare $(INCLUDES)


################################################################################
# Rules
################################################################################

all: init $(STL_LIB) $(BINDIR)/test

init:
	@mkdir -p $(OBJDIR)
	@mkdir -p $(BINDIR)
	@mkdir -p $(MAINDIR)

clean:
	rm -rf $(BASEDIR)/*


$(STL_LIB): $(OBJFILES)
	@echo Creating $@
	@mkdir -p $(@D)
	@ar rcs $@ $^


$(MAINDIR)/%.o: main/%.cpp
	@echo Compiling $<
	@mkdir -p $(@D)
	@g++ $(CPPFLAGS) -c $< -o $@


#
# How could I forget??
# Libraries need to be AFTER the object files.
# Otherwise, you get undefined references (idiot...). 
#
$(BINDIR)/test: $(MAINDIR)/main.o $(STL_LIB)
	@echo Linking $@...
	@$(LINK) $^ -o $@
