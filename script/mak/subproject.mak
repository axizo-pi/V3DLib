
COMPILE := $(CXX) -std=c++17 -c $(CXX_FLAGS) $(INCLUDE)
TARGET=${OBJDIR}/bin/${NAME}

OBJ_LOCAL=${OBJDIR}/${NAME}
$(info OBJ_LOCAL: $(OBJ_LOCAL))

STRUCTURE := $(shell ls -r)     
$(info structure: $(STRUCTURE))
SRCFILES := $(filter %.c %.cpp,$(STRUCTURE))
$(info  src: $(SRCFILES))
OBJFILES := $(subst .cpp,.o,$(SRCFILES))
OBJFILES := $(subst .c,.o,$(OBJFILES))
$(info  obj: $(OBJFILES))
OBJFILES := $(addprefix $(OBJ_LOCAL)/,$(OBJFILES))
$(info  obj: $(OBJFILES))

HEADERFILES := $(filter %.h ,$(STRUCTURE))

# Dependencies from list of object files
#DEPS := $(OBJFILES:.o=.d)
#$(info deps:  $(DEPS))
#-include $(DEPS)

.PRECIOUS: $(OBJ_LOCAL)/%.o

all: init ${TARGET}

#	echo "Here ${DEBUG} ${QPU}"

$(TARGET) : $(OBJFILES)

init:
	@mkdir -p ${OBJ_LOCAL}

#
# Haven't figured out yet how to compile on header change
# Following is a bit overkill.
#

$(OBJ_LOCAL)/%.o: %.cpp $(HEADERFILES)
	@echo Compiling $<
	@$(COMPILE) -o $@ $< -MMD -MP -MF"$(@:%.o=%.d)" $(INCLUDE) 


$(OBJDIR)/bin/$(NAME): $(OBJFILES) $(V3DLIB) $(LIB_DEPEND)
	@echo Linking $@...
	@$(LINK) $^ $(CXX_FLAGS) $(LIBS) -o $@
