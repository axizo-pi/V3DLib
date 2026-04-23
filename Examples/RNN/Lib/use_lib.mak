
RNN_LIB=RNNSupport

INCLUDE_EXTERN+= \
 -I $(BASE)/Examples/RNN/Lib

LIB_EXTERN+= \
 -l$(RNN_LIB)

$(NAME) : $(RNN_LIB) all

LIB_DEPEND += ${OBJDIR}/lib$(RNN_LIB).a

$(RNN_LIB):
	cd ../RNN/Lib && make all
