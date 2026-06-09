#include "./kernel.h"
#include "V3DLib.h"
#include "helpers.h"

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


/**
 * @brief Do a **per-row division** of lhs values with elements of rhs.
 */
void div_vector_kernel(Float::Ptr in_ret, Float::Ptr in_lhs, Float::Ptr in_rhs, Int rows, Int cols) {
  Float::Ptr rhs = in_rhs;
  rhs -= index();
  Int col_count = cols >> 4;

  For (Int r = 0, r < rows, r++)
    Float val = *rhs;
    Float::Ptr lhs = in_lhs + r*cols;  // Prob not necessary; TODO check
    Float::Ptr ret = in_ret + r*cols;  // idem

    For (Int c = 0, c < col_count, c++)
      *ret = *lhs / val;

      lhs.inc();
      ret.inc();
    End

    rhs += 1;
  End
}


/**
 * Derived from:
 *
 *     ret.m_qpu = (q_ones.qpu() - qpu()).mul_e(h_row.qpu() + qpu()).mul_e(S.qpu());
 *
 * Multi-QPU does not improve performance at all.
 */
void forward_4_kernel(Float::Ptr ret, Float::Ptr x, Float::Ptr h, Float::Ptr S, Int N) {
  Float one = 1.0f;

  For (Int n = 0, n < N, n++)
    Float x_val = *x;
    Float val = (one - x_val) * (*h - x_val) * *S;
    *ret = val;

    ret.inc();
    x.inc();
    h.inc();
    S.inc();
  End
}


/**
 * Result stored in rhs
 */
void forward_5_kernel(Float::Ptr in_rhs, Int cols) {
  Int col_size = cols >> 4;
  Float max;
  Float::Ptr rhs = in_rhs;

  kernel::max_partial(rhs, max, col_size);

  rhs = in_rhs;
  kernel::softmax_partial(rhs, max, col_size);

  //
  // Calculate sum
  //
  rhs = in_rhs;
  Float sum = 0.0f;

  For (Int c = 0, c < col_size, c++)
    sum += *rhs;
    rhs.inc();
  End

  rotate_sum(sum, sum);

  //
  // Divide values by sum
  //
  rhs = in_rhs;

  For (Int c = 0, c < col_size, c++)
    Float tmp = *rhs;
    *rhs = tmp/sum;
    rhs.inc();
  End
}


/**
 * Combined kernel derived from:
 *
 *     ret.m_qpu = (qpu() * (m.U_z.qpu())) + (S.qpu() * (m.W_z.qpu()));
 *
 * This is used several times in GRU test.
 *
 * Derived from `mult_matrix_col()`.
 */
void mult_matrix_col_add_kernel(
  Float::Ptr ret,
  Float::Ptr lhs1, Float::Ptr rhs1,
  Float::Ptr lhs2, Float::Ptr rhs2,
   Int lhs_rows,
  Int inner1, Int inner2,
   Int rhs_cols
) {
  Float::Ptr ret_base = ret;
  ret_base           -= index();

  Int rhs_offset      = index()*rhs_cols;
  Int rhs_inc         = 16*rhs_cols;

  Float::Ptr rhs1_base = rhs1;
  rhs1_base           -= index();
  rhs1_base           += rhs_offset;

  Float::Ptr rhs2_base = rhs2;
  rhs2_base           -= index();
  rhs2_base           += rhs_offset;

  Int block_size1 = inner1 >> 4;
  Int block_size2 = inner2 >> 4;

  For (Int col = me(), col < rhs_cols, col += numQPUs())
    Float::Ptr rhs1_col = rhs1_base + col;
    Float::Ptr rhs2_col = rhs2_base + col;

    For (Int row = 0, row < lhs_rows, row++)
      Float::Ptr lhs1_row = (lhs1 + (row*inner1));  comment("Init lhs row");
      Float::Ptr lhs2_row = (lhs2 + (row*inner2));

      Float acc1 = 0.0f;
      Float acc2 = 0.0f;

      For (Int block = 0, block < block_size1, block++)
        acc1 += *lhs1_row * *rhs1_col;  comment("increment acc1");

        lhs1_row.inc();
        rhs1_col += rhs_inc;
      End

      For (Int block = 0, block < block_size2, block++)
        acc1 += *lhs2_row * *rhs2_col;  comment("increment acc2");

        lhs2_row.inc();
        rhs2_col += rhs_inc;
      End

      acc1 += acc2;
      rotate_sum(acc1, acc1);

      *(ret_base + row*rhs_cols + col) = acc1;
    End
  End
}


bool done_init = false;
std::unique_ptr<BaseKernel> s_back_prop_1;
std::unique_ptr<BaseKernel> s_back_prop_3;
std::unique_ptr<BaseKernel> s_back_prop_4;
std::unique_ptr<BaseKernel> s_set_decay;
std::unique_ptr<BaseKernel> s_divide_matrix;
std::unique_ptr<BaseKernel> s_div_vector;
std::unique_ptr<BaseKernel> s_forward_4;
std::unique_ptr<BaseKernel> s_forward_5;
std::unique_ptr<BaseKernel> s_mult_matrix_col_add;


void init_local() {
  if (done_init) return;

  s_back_prop_1.reset(new BaseKernel(compile(back_prop_1_kernel, settings())));
  //to_file("s_back_prop_1.txt", s_back_prop_1->dump());

  s_back_prop_3  .reset(new BaseKernel(compile(back_prop_3_kernel  , settings())));
  s_back_prop_4  .reset(new BaseKernel(compile(back_prop_4_kernel  , settings())));
  s_set_decay    .reset(new BaseKernel(compile(set_decay_kernel    , settings())));
  s_divide_matrix.reset(new BaseKernel(compile(divide_matrix_kernel, settings())));
  s_div_vector   .reset(new BaseKernel(compile(div_vector_kernel   , settings())));
  s_forward_4    .reset(new BaseKernel(compile(forward_4_kernel    , settings())));
  s_forward_5    .reset(new BaseKernel(compile(forward_5_kernel    , settings())));

  s_mult_matrix_col_add.reset(new BaseKernel(compile(mult_matrix_col_add_kernel, settings())));

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


void divide_vector(matrix &ret, matrix &lhs, matrix const &rhs) {
  init_local();
  assert(lhs.rows() == rhs.rows());
  assert(rhs.is_vector());

  s_div_vector->load(&ret.arr(), &lhs.arr(), &rhs.arr(), lhs.rows(), lhs.columns()).run();
}


void forward_4(matrix &ret, matrix &X, matrix &h, matrix &S) {
  init_local();
  assert(X.size() == ret.size());
  assert(X.size() == h.size());
  assert(X.size() == S.size());

  s_forward_4->load(&ret.arr(), &X.arr(), &h.arr(), &S.arr(), X.size()/16).run();
}


void forward_5(matrix &rhs) {
  assert(rhs.rows() == 1);
  assert(!rhs.empty());

  s_forward_5->load(&rhs.arr(), rhs.columns()).run();  // size() should work as well
}


void mult_matrix_col_add(matrix &ret, matrix &lhs1, matrix &rhs1, matrix &lhs2, matrix &rhs2) {
   //timers.start("mult_matrix_col_add");
  assert(lhs1.rows() == lhs2.rows());
  assert(rhs1.columns() == rhs2.columns());

  s_mult_matrix_col_add->setMaxQPUs();  // After #QPU = 8 not much improvement
  s_mult_matrix_col_add->load(
    &ret.arr(),
    &lhs1.arr(), &rhs1.arr(),
    &lhs2.arr(), &rhs2.arr(),
     lhs1.rows(),
    lhs1.columns(), lhs2.columns(),
     rhs1.columns()
  ).run();

   //timers.stop("mult_matrix_col_add");
}


/**
 * @brief Explicitly initialize the GRU kernels
 *
 * `init_local()` takes about 1ms on Pi5 (vc7), which kills the performance
 * of a kernel when called implicitly - typically, runtime for a kernel is 0.05 ms.
 * This is of course, a single call overall, but it screws up the profile timinig.
 *
 * The overhead should be even worse on the other Pi's.  
 * Better to do the init explicitly.
 */
void init() {
  init_local();
}

} // namespace gru_kernel
