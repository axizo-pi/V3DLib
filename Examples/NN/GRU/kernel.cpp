#include "./kernel.h"
#include "Source/Lang.h"
#include "helpers.h"
#include "Support/Helpers.h"

namespace gru_kernel {

using namespace V3DLib;
using namespace qpu;

namespace {

/**
 * Multi-QPU is disappointing; only 20% improvement.
 */
void back_prop_1_kernel(Float::Ptr ret, Float::Ptr ds_cur, Float::Ptr q_z, Float::Ptr q_h, Int N) {
	Int s_16 = 16;
	Int s_64 = 4*16;

	Int offset      = s_16*numQPUs();    comment("Start back_prop_1_kernel");
	Int init_offset = s_64*me();

  q_h    = q_h.offset(init_offset);    comment("Init pointers for multi-QPU");
  q_z    = q_z.offset(init_offset);
  ds_cur = ds_cur.offset(init_offset);
  ret    = ret.offset(init_offset);

  For (Int h = me(), h < N, h += numQPUs())
    Float x  = *q_h;     comment("Start Loop");
    Float x2 = *q_z;
    x = 1.0f - x*x;     // dtanh

		Float x3 = *ds_cur;
    x2 = (1.0f - x2);
    *ret = x3 * x * x2;

    q_h    += offset;   comment("Update pointers");
    q_z    += offset;
    ds_cur += offset;
    ret    += offset;
  End
}


/**
 * Original:
 *
 *      m_qpu = dsr.m_qpu.mul_e(temp.S.qpu()).mul_e(qpu::vector(temp.r.qpu()).dsigmoid());
 */
void back_prop_3_kernel(Float::Ptr ret, Float::Ptr dsr, Float::Ptr S, Float::Ptr r, Int N) {
	Int offset = 16*numQPUs();
	Int init_offset = 4*16*me();

  r   = r.offset(init_offset);
  S   = S.offset(init_offset);
  dsr = dsr.offset(init_offset);
  ret = ret.offset(init_offset);

  For (Int h = me(), h < N, h += numQPUs())
    Float x  = *r;
    Float x2 = (1.0f - x)*x;  // dsigmoid
    Float x3 = (*S) * x2;
    Float x4 = (*dsr) * x3;
    *ret = x4;

    r   += offset;
    S   += offset;
    dsr += offset;
    ret += offset;
  End
}


/**
 * Multi-QPU: negligible performance increase, +-5%
 *
 * Original:
 *
 *      qpu::matrix q_dz = ds_cur_bk.qpu().mul_e(temp.S.qpu() - temp.h.qpu());
 *      m_qpu = q_dz.mul_e(qpu::vector(temp.z.qpu()).dsigmoid());
 */
void back_prop_4_kernel(Float::Ptr ret, Float::Ptr ds_cur_bk, Float::Ptr z, Float::Ptr S, Float::Ptr h, Int N) {
	Int offset = 16*numQPUs();
	Int init_offset = 4*16*me();

  h   = h.offset(init_offset);
  S   = S.offset(init_offset);
  z   = z.offset(init_offset);
  ds_cur_bk = ds_cur_bk.offset(init_offset);
  ret = ret.offset(init_offset);

  For (Int n = me(), n < N, n += numQPUs())
    Float x    = (*S) - (*h);
    Float q_dz = *ds_cur_bk * x;
    Float x2 = *z;
    Float x3 = (1.0f - x2)*x2;  // dsigmoid
    *ret = q_dz * x3;


    h   += offset;
    S   += offset;
    z   += offset;
    ds_cur_bk += offset;
    ret += offset;
  End
}


void set_decay_kernel(Float::Ptr lhs, Float::Ptr rhs, Float decay, Int N) {
  For (Int n = 0, n < N, n++)
		Float x1 = *rhs;
		Float x2 = (1.0f - decay) * x1 * x1;
  	Float x3 = decay * *lhs + x2;
		*lhs = x3;

		lhs.inc();
		rhs.inc();
	End
}


void divide_matrix_kernel(Float::Ptr ret, Float::Ptr lhs, Float::Ptr rhs, Int N) {
  Float err = 0.00000001f;

  For (Int n = 0, n < N, n++)
		*ret = *lhs / (sqrt_f(*rhs) + err);

		lhs.inc();
		rhs.inc();
		ret.inc();
	End
}


bool done_init = false;
std::unique_ptr<BaseKernel> s_back_prop_1;
std::unique_ptr<BaseKernel> s_back_prop_3;
std::unique_ptr<BaseKernel> s_back_prop_4;
std::unique_ptr<BaseKernel> s_set_decay;
std::unique_ptr<BaseKernel> s_divide_matrix;


void init_local() {
  if (done_init) return;

  s_back_prop_1.reset(new BaseKernel(compile(back_prop_1_kernel, settings())));
	//to_file("s_back_prop_1.txt", s_back_prop_1->dump());

  s_back_prop_3  .reset(new BaseKernel(compile(back_prop_3_kernel  , settings())));
  s_back_prop_4  .reset(new BaseKernel(compile(back_prop_4_kernel  , settings())));
  s_set_decay    .reset(new BaseKernel(compile(set_decay_kernel    , settings())));
  s_divide_matrix.reset(new BaseKernel(compile(divide_matrix_kernel, settings())));

  done_init = true;
}  

} // anon namespace


/**
 * Original:
 *
 *     m_qpu = ds_cur.m_qpu.mul_e(ones.m_qpu - temp.q_z).mul_e(temp.q_h.dtanh());
 */
void back_prop_1(matrix &ret, matrix const &ds_cur, matrix const &q_z, matrix const &q_h) {
  init_local();

  int size = ds_cur.rows()*ds_cur.columns();

	s_back_prop_1->setMaxQPUs();
  s_back_prop_1->load(&ret.arr(), &ds_cur.arr(), &q_z.arr(), &q_h.arr(),  size/16).run();
}


void back_prop_3(matrix &ret, matrix const &dsr, matrix const &S, matrix const &r) {
  init_local();

  int size = dsr.rows()*dsr.columns();

	s_back_prop_3->setMaxQPUs();
  s_back_prop_3->load(&ret.arr(), &dsr.arr(), &S.arr(), &r.arr(),  size/16).run();
}


void back_prop_4(matrix &ret, matrix const &ds_cur_bk, matrix const &z, matrix const &S, matrix const &h) {
  init_local();

  int size = ds_cur_bk.rows()*ds_cur_bk.columns();

	s_back_prop_4->setMaxQPUs();
  s_back_prop_4->load(&ret.arr(), &ds_cur_bk.arr(), &z.arr(), &S.arr(), &h.arr(), size/16).run();
}


void set_decay(matrix &lhs, matrix const &rhs, float decay) {
  init_local();

  int size = rhs.rows()*rhs.columns();

  s_set_decay->load(&lhs.arr(), &rhs.arr(), decay, size/16).run();
}


void divide_matrix(matrix &ret, matrix &lhs, matrix const &rhs) {
  init_local();

  int size = rhs.rows()*rhs.columns();

  s_divide_matrix->load(&ret.arr(), &lhs.arr(), &rhs.arr(), size/16).run();
}


} // namespace gru_kernel
