#include "./kernel.h"
#include "Source/Lang.h"
#include "helpers.h"

namespace gru_kernel {

using namespace V3DLib;
using namespace qpu;

namespace {

void back_prop_1_kernel(Float::Ptr ret, Float::Ptr ds_cur, Float::Ptr q_z, Float::Ptr q_h, Int N) {
  For (Int h = 0, h < N, h++)
    Float x = *q_h;
    x = 1.0f - x*x;  // dtanh

    Float x2 = *q_z;
    x2 = (*ds_cur) * (1.0f - x2);

		*ret = x * x2;

		q_h.inc();
		q_z.inc();
		ds_cur.inc();
		ret.inc();
	End
}


/**
 * Original:
 *
 *      m_qpu = dsr.m_qpu.mul_e(temp.S.qpu()).mul_e(qpu::vector(temp.r.qpu()).dsigmoid());
 */
void back_prop_3_kernel(Float::Ptr ret, Float::Ptr dsr, Float::Ptr S, Float::Ptr r, Int N) {
  For (Int h = 0, h < N, h++)
    Float x  = *r;
    Float x2 = (1.0f - x)*x;  // dsigmoid
    Float x3 = (*S) * x2;
    Float x4 = (*dsr) * x3;
		*ret = x4;

		r.inc();
		S.inc();
		dsr.inc();
		ret.inc();
	End
}


bool done_init = false;
std::unique_ptr<BaseKernel> s_back_prop_1;
std::unique_ptr<BaseKernel> s_back_prop_3;


void init_local() {
	if (done_init) return;

  s_back_prop_1 .reset(new BaseKernel(compile(back_prop_1_kernel, settings())));
  s_back_prop_3 .reset(new BaseKernel(compile(back_prop_3_kernel, settings())));

	done_init = true;
}	

} // anon namespace


void back_prop_1(matrix &ret, matrix const &ds_cur, matrix const &q_z, matrix const &q_h) {
	init_local();

	int size = ds_cur.rows()*ds_cur.columns();

  s_back_prop_1->load(&ret.arr(), &ds_cur.arr(), &q_z.arr(), &q_h.arr(),  size/16).run();
}


void back_prop_3(matrix &ret, matrix const &dsr, matrix const &S, matrix const &r) {
	init_local();

	int size = dsr.rows()*dsr.columns();

  s_back_prop_3->load(&ret.arr(), &dsr.arr(), &S.arr(), &r.arr(),  size/16).run();
}


} // namespace gru_kernel
