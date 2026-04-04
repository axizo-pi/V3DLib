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

# Get list of executables
# NOTE: grepping on 'main(' is not fool-proof, of course.
EXE1=$(grep -l "main(" Examples/* Tools/* --directories=skip)
EXE2=$(echo "$EXE1" | sed "s/\\.cpp$/  \\\\/g")
EXAMPLES=$(echo "$EXE2" | sed "s/^.*\//  /g")


# Get list of example makefiles
MAKEFILES=$(find Examples -name 'makefile') # | tr '\n' ' ')
#echo $MAKEFILES

# Jezus Christ, it took me ages to figure following out.
# Translate string to array.
eval "ARR=( $MAKEFILES )"
makelist=""
projects=""
for item in "${ARR[@]}"; do
	echo Item: $item
	basedir=$(echo "$item" | awk -F '/' '{print $1}')
	name=$(echo "$item" | awk -F '/' '{print $2}')

	projects="${projects}  ${name} \\
"

	makelist="$makelist
$name: \$(V3DLIB)
	@cd "$basedir/$name" && make DEBUG=\${DEBUG} QPU=\${QPU} 
	"
done

mkdir -p obj

cat << END > sources.mk
#
# This file is generated!  Editing it directly is a bad idea.
#
# Generated on: $(date)
#
###############################################################################

# Library Object files - only used for LIB
OBJ := \\
$OBJ

# All programs in the Examples *and Tools* directory
EXAMPLES := \\
$EXAMPLES

# support files for tests
TESTS_FILES := \\
$OBJ_TEST

#
# sub-projects
#
SUB_PROJECTS := \\
${projects}
$makelist

END
