
# =* required, otherwise preceding whitespace part of value
SRCDIR  =src
BASEDIR =./obj
OBJDIR  =$(BASEDIR)/src
BINDIR  =$(BASEDIR)/bin
TESTDIR =$(BASEDIR)/tests

OUT_LIB=$(BINDIR)/libnnetcpp.a

LIB_CPPUNIT=../cppunit/libcppunit.a

INCLUDES = \
-I $(SRCDIR)              \
-I ../../GRU/Tools/GRU/   \
-I ../cppunit/include     \
-I ../cppunit

-include ../../../../script/mak/extern.mak
#$(info OBJFILES: $(OBJFILES))

CPPFLAGS += -Wno-sign-compare $(INCLUDES)
#$(info CPPFLAGS: $(CPPFLAGS))


all: init $(OUT_LIB) test

test: init $(BINDIR)/tests $(BINDIR)/sequencegeneration

init:
	@mkdir -p $(OBJDIR)
	@mkdir -p $(BINDIR)
	@mkdir -p $(TESTDIR)

clean:
	rm -rf $(BASEDIR)/*


$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@echo Compiling $<
	@mkdir -p $(@D)
	@g++ $(CPPFLAGS) -c $< -o $@

$(OUT_LIB): $(OBJFILES)
	@echo Creating $@
	@mkdir -p $(@D)
	@ar rcs $@ $^

###############################################

TEST_OBJ_FILES :=obj/tests/test_merge.o  obj/tests/test_perceptron.o  obj/tests/test_recurrent.o  obj/tests/tests.o

$(TESTDIR)/%.o: tests/%.cpp
	@echo Compiling test $<
	@mkdir -p $(@D)
	@g++ $(CPPFLAGS) -c $< -o $@

$(BINDIR)/sequencegeneration : obj/tests/sequencegeneration.o $(OUT_LIB) $(LIB_CPPUNIT)
	@echo Linking $@...
	@$(LINK) $^ -o $@

$(BINDIR)/tests: $(TEST_OBJ_FILES) $(OUT_LIB) $(LIB_CPPUNIT)
	@echo Linking $@...
	@$(LINK) $^ -o $@
