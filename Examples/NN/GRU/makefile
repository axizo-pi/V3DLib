NAME=GRU

RNN_LIB=RNNSupport


INCLUDE_EXTERN+= \
 -I $(BASE)/Examples/RNN/Lib

LIB_EXTERN+= \
 -l$(RNN_LIB)


$(NAME) : $(RNN_LIB) all

SRCDIR=./

-include ../../script/mak/config.mak

LIB_DEPEND += ${OBJDIR}/lib$(RNN_LIB).a

-include ../../script/mak/subproject.mak

$(RNN_LIB):
	cd ../RNN/Lib && make all
