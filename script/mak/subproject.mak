$(info included subproject.mak)


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
#OBJFILES := $(OBJFILES:%.cpp=%.o)
OBJFILES := $(OBJFILES:%.c=%.o)
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

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@echo Compiling $<
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@echo Compiling $<
	@mkdir -p $(@D)
	@g++ $(CFLAGS) -c $< -o $@
