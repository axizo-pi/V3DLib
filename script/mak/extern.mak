
XX_FLAGS = -MMD -MP -MF"$(@:%.o=%.d)"

CFLAGS = -g -Wall -Wno-unused-function -O3 -pthread \
$(XX_FLAGS)

LINK := g++


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
SRCFILES_C := $(filter %.c,$(CODEFILES))
#$(info src c: $(SRCFILES_C))
OBJFILES_C := $(SRCFILES_C:%.c=%.o)
#$(info obj c: $(OBJFILES_C))


SRCFILES_CPP := $(filter %.cpp,$(CODEFILES))
#$(info src c++: $(SRCFILES_CPP))
OBJFILES_CPP := $(SRCFILES_CPP:%.cpp=%.o)
#$(info obj c++: $(OBJFILES_CPP))

HDRFILES := $(filter %.h,$(CODEFILES))
OBJFILES_TMP := $(subst $(SRCDIR),$(OBJDIR)$(SRC_DIR),$(SRCFILES))
#$(info  obj: $(OBJFILES_TMP))

OBJFILES := $(OBJFILES_C)
OBJFILES += $(OBJFILES_CPP)
OBJFILES := $(subst $(SRCDIR),$(OBJDIR)$(SRC_DIR),$(OBJFILES))
#$(info  obj: $(OBJFILES))

# Dependencies from list of object files
DEPS := $(OBJFILES:.o=.d)
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

