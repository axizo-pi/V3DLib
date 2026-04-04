NAME=Rot3D

SRCDIR=./


STL_DIR=../../obj/stl

INCLUDE_EXTERN= \
 -I ../../extern/stl/src

LIB_EXTERN= \
 -L $(STL_DIR)/bin -lstl

STL_LIB=$(STL_DIR)/bin/libstl.a

LIB_DEPEND= \
  $(STL_LIB)

-include /home/wim/projects/V3DLib/script/mak/config.mak
-include /home/wim/projects/V3DLib/script/mak/subproject.mak

$(STL_LIB):
	cd ../../extern/stl && make
