NAME=Mandelbrot


-include /home/wim/projects/V3DLib/script/mak/config.mak

TARGET=${OBJDIR}/bin/${NAME}

OBJ_LOCAL=$(OBJDIR)/$(NAME)

-include /home/wim/projects/V3DLib/script/mak/subproject.mak

.PRECIOUS: $(OBJ_LOCAL)/%.o

all: init ${TARGET}
	echo "Here ${DEBUG} ${QPU}"


init:
	mkdir -p $(OBJ_LOCAL)


# From general rule
$(OBJ_LOCAL)/%.o: %.cpp
	@echo Compiling $<
	@mkdir -p $(@D)
	@$(CXX) -std=c++17 -c -o $@ $< $(CXX_FLAGS)


# From general rule
$(OBJDIR)/bin/%: $(OBJ_LOCAL)/%.o $(V3DLIB) $(LIB_DEPEND)
	@echo Linking $@...
	@mkdir -p $(@D)
	@$(LINK) $^ $(LIBS) -o $@
