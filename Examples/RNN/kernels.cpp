#include "kernels.h"


void kernel_sigmoid(Float::Ptr in, Float::Ptr bias, Float::Ptr out, Int N) {

  For (Int h = 0, h < N, ++h)
		Float x = *in;

		x += *bias;
		x = (1.0/(1.0 + exp_e(-1.0*x)));

		*out = x;

		in.inc();
		bias.inc();
		out.inc();
	End
}


void kernel(Float::Ptr input, Float::Ptr mat, Float::Ptr result, Int M, Int N) {
  Float tmp[M];
  Float::Ptr row[M];
  Int offset2 = 4*16*N;

  For (Int h = 0, h < M, ++h)
    tmp[h] = 0;
    row[h] = mat.offset(h*offset2);
	End

  For (Int i = 0, i < N, ++i)
    Float v = *input;
  
  	For (Int h = 0, h < M, ++h)
      tmp[h] += (*row[h])*v;
    End

    If (i < (N - 1))
  		For (Int h = 0, h < M, ++h)
        row[h].inc();
    	End

      input.inc();
    End
 	End 

  Float res = 0;

 	For (Int h = 0, h < M, ++h)
    rotate_sum(tmp[h], tmp[h]);
    res.set_at(h, tmp[h]);
 	End 

  *result = res;
}


// @param N  length of input vectors, in blocks of 16
void outer_product(Float::Ptr left, Float::Ptr right, Float::Ptr out_matrix, Int N) {
	left -= index();
	//Float right_out = toFloat(2*(1 + index()));

  For (Int i = 0, i < 16*N, ++i)
		Float::Ptr start = right;

  	For (Int j = 0, j < N, ++j)
			*out_matrix = *left * *start;
			out_matrix.inc(); start.inc();
		End

		++left;
	End
}


void vector_sub(Float::Ptr left, Float::Ptr right, Float::Ptr out, Int N) {
	Float tmp;

  For (Int i = 0, i < N, ++i)
		tmp = *left - *right; 
		*out = tmp;

		left.inc();
		right.inc();
		out.inc();
	End
}
