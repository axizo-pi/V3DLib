#!/bin/bash
#
# Pre: create obj directories beforehand
#
# ------------------------------------------------------------------------------
# NOTES
# =====
#
# * Here-delimiter with quotes does not do parameter/command substitution
#   e.g.:
#
#    cat <<'END'
#    Generated on: $(date)
#    END
#
# ------------------------------------------------------------------------------
# NOTES
# =====
#
# - Consider using make commands as here: https://stackoverflow.com/a/32699423
#
################################################################################

# Make list of Library object files
CPP_FILES=$(find Lib -name '*.cpp')
OBJ_CPP=$(echo "$CPP_FILES" | sed "s/\\.cpp$/\\.o  \\\\/g")

C_FILES=$(find Lib -name '*.c')
OBJ_C=$(echo "$C_FILES" | sed "s/\\.c$/\\.o  \\\\/g")

OBJ_TMP=$(echo "$OBJ_CPP
$OBJ_C
")
OBJ=$(echo "$OBJ_TMP" | sed "s/^Lib\\//  /g")
#echo $OBJ


TEST_FILES=$(find Tests -name '*.cpp')
CPP_OBJ_TEST_TMP=$(echo "$TEST_FILES" | sed "s/\\.cpp$/\\.o  \\\\/g")

C_TEST_FILES=$(find Tests -name '*.c')
C_OBJ_TEST_TMP=$(echo "$C_TEST_FILES" | sed "s/\\.c$/\\.o  \\\\/g")

OBJ_TEST_TMP=$(echo "$CPP_OBJ_TEST_TMP
$C_OBJ_TEST_TMP
")
OBJ_TEST=$(echo "$OBJ_TEST_TMP" | sed "s/^/  /g")

#
# Get list of executables
#
# NOTE: grepping on 'main(' is not fool-proof, of course.
#
EXE1=$(grep -l "main(" Examples/* Tools/* --directories=skip)
EXE2=$(echo "$EXE1" | sed "s/\\.cpp$/ \\\\/g")
EXAMPLES=$(echo "$EXE2" | sed "s/^.*\//  /g")

#
# Generate rules for projects with precompiled kernels.
#
# This works for single-file example apps, example subprojects are more intricate.
#
# Generation for subprojects disabled till I figure it out.
#
GEN_EXE1=$(cd Examples; grep -l "USE_HEX_KERNEL" * --directories=skip)

# This version detects subprojects as well
#GEN_EXE1=$(cd Examples; grep -rl "USE_HEX_KERNEL" *)
echo "GEN_EXE1: $GEN_EXE1"

# For subprojects, this uses directory for project name prefix.
# Single-file project names not affected.
GEN_TMP=$(echo "$GEN_EXE1" | awk -F'/' -v OFS='/' '{print $1}')
GEN_TMP=$(echo "$GEN_TMP" | sed "s/\\.cpp$//g")
echo "GEN_TMP: $GEN_TMP"

GEN_APPS=""

app_rules="
"

eval "ARR2=( $GEN_TMP )"
for item in "${ARR2[@]}"; do
	echo Item2: $item

	app_rules="${app_rules}
\$(OBJDIR)/Examples/${item}_use_hex_kernel.o: Examples/${item}.cpp \$(OBJDIR)/generated/${item}_hex_kernels.cpp | init
	@echo Compiling generated kernels for $<
	@mkdir -p \$(@D)
	@\$(CPP) -DUSE_HEX_KERNEL -o \$@ $< 
"
done

eval "ARR2=( $GEN_TMP )"
for item in "${ARR2[@]}"; do
	#echo Item1: $item

	app_rules="${app_rules}
\$(OBJDIR)/generated/${item}_hex_kernels.cpp: \$(OBJDIR)/bin/${item}
	@echo Generating kernels for $<
	@mkdir -p \$(@D)
	\$(OBJDIR)/bin/${item} -cs -gk=\$@
"
done

eval "ARR2=( $GEN_TMP )"
for item in "${ARR2[@]}"; do
	#echo Item: $item
	app_rules="${app_rules}
${item}_use_hex_kernel: \$(OBJDIR)/bin/${item}_use_hex_kernel
"
	GEN_APPS="${GEN_APPS}  ${item}_use_hex_kernel \\
"
done

#echo "GEN_APPS: $GEN_APPS"

#
# Get list of example makefiles
#
MAKEFILES=$(find Examples -name 'makefile') # | tr '\n' ' ')
#echo "Makefiles: $MAKEFILES"

#
# Translate string to array.
#
# Jezus Christ, it took me ages to figure this out.
#
eval "ARR=( $MAKEFILES )"
makelist=""
projects=""
for item in "${ARR[@]}"; do
	#echo Item: $item
	basedir=$(echo "$item" | awk 'BEGIN{FS=OFS="/"}{NF--; NF--; print}')
	name=$(echo "$item" | awk 'BEGIN{FS=OFS="/"}{print $(NF-1)}')

	projects="${projects}  ${name} \\
"

	makelist="$makelist
$name: \$(V3DLIB)
	@cd "$basedir/$name" && make DEBUG=\${DEBUG}
	"
done

#echo Projects: $projects

mkdir -p obj

#
# Output sources makefile
#
cat << END > sources.mk
#
# This file is generated! Editing it directly is a bad idea.
#
# Generated on: $(date)
#
###############################################################################

# Library Object files - only used for LIB
OBJ := \\
$OBJ


#
# All programs in the Examples and Tools directory
#
EXAMPLES := \\
$EXAMPLES

# support files for tests
TESTS_FILES := \\
$OBJ_TEST

#
# sub-projects
#
SUB_PROJECTS := \\
${projects}${GEN_APPS}
$makelist

#
# Rules for projects with precompiled kernels
#
\$(OBJDIR)/%.o: \$(OBJDIR)/generated/%.cpp | init
	@echo Compiling $<
	@mkdir -p \$(@D)
	@\$(CPP) -o $@ $< 
${app_rules}

END
