#include "mmatrix.h"
#include "./kernel.h"
#include "global.h"
#include "model.h"
#include "Support/Helpers.h"  // bit_diff()

namespace {

float s_max = 0;

float s_softmax(float x) {
  return (float) exp(x - s_max);
}


/**
 * Why the `_Xf` postfix is required, is beyond me.
 * Leaving it out results in 'no known conversion' error.
 */
bool same_Xf(MatrixXf const &lhs, MatrixXf const &rhs, int bit_diff,  bool show_max_diff) {
  //warn << "Called same(MatrixXf, MatrixXf)";

  bool ret       = true;
	qpu::CompareStats stats;

  if(lhs.rows() != rhs.rows() || lhs.cols() != rhs.cols() ) {
     warn << "Fail same(MatrixXf, MatrixXf) dimensions differ: "
          << "lhs: " << ::dump_dim(lhs) << ", "
          << "rhs: " << ::dump_dim(rhs);

     return false;
  }

  std::string buf;

  for (int i = 0; i < (int) rhs.rows(); ++i) {
    if (!show_max_diff && !ret) break;

    for (int j = 0; j < (int) rhs.cols(); ++j) {
      if (!qpu::check_precision(lhs(i, j), rhs(i, j), bit_diff, &stats, ret)) {
        if (ret) {  // Show first fail only
          buf << " at (" << i << ", " << j << ")";
        }

        ret = false;
        if (!show_max_diff) break;
      }      
    }
  }

  if (show_max_diff) {
    buf << ", " << stats.dump();
  }

  if (!buf.empty()) {
    warn << "Fail same(MatrixXf, MatrixXf)" << buf ;
  }

  return ret;
}

}

MMatrix::MMatrix() {}

MMatrix::MMatrix(int rows, int columns, float val, bool set_Xf) {
  assert(rows > 0);
  assert(columns > 0);
  assert(val == 0.0f || val == 1.0f);

  if (set_Xf) {
    warn << "This!";
    if (val == 0.0f) {
      m_Xf = MatrixXf::Zero(rows, columns);
    } else {
      m_Xf = MatrixXf::Ones(rows, columns);
    }
    used_fields(true, false);
  } else {
    m_qpu.resize(rows, columns);
    m_qpu.set(val);
    used_fields(false, true);
  }
}


void MMatrix::resize(int rows, int columns, float val) {
  *this = MMatrix(rows, columns, val);
}


MMatrix::MMatrix(MMatrix const &rhs) {
  timers.start("MMatrix ctor &");
  set(rhs);
  timers.stop("MMatrix ctor &");
}


/**
 * Currently same as `ctor &`. It doesn't make a dent in the profiling.
 */
MMatrix::MMatrix(MMatrix const &&rhs) {
  timers.start("MMatrix ctor &&");
  set(rhs);
  timers.stop("MMatrix ctor &&");
}


/**
 * Required because `ctor &&` added.
 *
 * Timing insignificant.
 */
MMatrix &MMatrix::operator=(MMatrix const &rhs) {
  set(rhs);
  return *this;
}


void MMatrix::set(MatrixXf const &rhs, bool set_qpu) {
  m_Xf = rhs;

  if (set_qpu) {
    //timers.start("set MatrixXf qpu");
    m_qpu = copy_m(m_Xf);    // All of time here
    //timers.stop("set MatrixXf qpu");
  }

  used_fields(true, set_qpu);
}


void MMatrix::set(MMatrix const &rhs) {
  if (rhs.m_using_Xf) {
    m_Xf = rhs.m_Xf;
  }

  if (rhs.m_using_qpu) {
    m_qpu = rhs.m_qpu;
  }

  used_fields(rhs.m_using_Xf, rhs.m_using_qpu);
}


void MMatrix::set(float val) {
  assert(m_using_qpu);

  m_qpu.set(val);

  used_fields(false, true);
}


/**
 * Application-specific load from vector.
 */
void MMatrix::set(std::vector<int> const &rhs, int pos) {
  assert(!empty());

  int count = 0;

  while ((pos + count) < (int) rhs.size() && count < rows()) {
    int index = pos + count;
    m_qpu.at(count, rhs[index]) = 1;

    count++;
  }

  used_fields(false, true);
}


bool MMatrix::is_zero() const {
  assert(m_using_Xf || m_using_qpu);

  bool Xf_zero = true;
  if (m_using_Xf) {
    //timers.start("is_zero Xf");
    for (int i = 0; i < m_Xf.rows(); i++) {
      for (int j = 0; j < m_Xf.cols(); j++) {
        if (m_Xf(i, j)) {
          Xf_zero = false;
          break;
        }
      }

      if (Xf_zero) break;
    }
    //timers.stop("is_zero Xf");
  }

  bool qpu_zero = true;
  if (m_using_qpu) {
    //timers.start("is_zero qpu");
    auto &arr = m_qpu.arr();
    auto ptr  = arr.ptr();
    auto size = m_qpu.size();

    // Using ptr 11x faster than using arr[i]
    // Still 7x slower than Xf
    for (int i = 0; i < size; i++) {
      if (*ptr != 0.0f) {
        qpu_zero = false;
        break;
      }

      ptr++;
    }
    //timers.stop("is_zero qpu");
  }

  if (m_using_Xf && m_using_qpu) {
    assert(Xf_zero == qpu_zero);
  }

  if (m_using_qpu) {
    return qpu_zero;
  } else {
    assert(m_using_Xf);
    return Xf_zero;
  }
}


void MMatrix::reset() {
  m_Xf = MatrixXf::Zero(rows(), cols());
  m_qpu.set(0.0f);

  used_fields(true, true);
}


int MMatrix::rows() const {
  assert(m_using_Xf || m_using_qpu);

  if (m_using_Xf) {
    return (int) m_Xf.rows();
  } else {
    return m_qpu.rows();
  }
}


int MMatrix::cols() const {
  assert(m_using_Xf || m_using_qpu);

  if (m_using_Xf) {
    return (int) m_Xf.cols();
  } else {
    return m_qpu.columns();
  }
}


bool MMatrix::empty() const {
  if (!m_using_Xf && !m_using_qpu) return true;

  return size() == 0;
}


MMatrix MMatrix::transpose() const {
  assert(false); // Warn me if called

  need_fields(true, false);
  //assert(rows() == 1 || cols() == 1);  // Simple case, vectors only

  MMatrix ret(cols(), rows());

  ret.m_Xf = m_Xf.transpose();
  ret.m_Xf.eval();

  ret.used_fields(true, false);
  return ret;
}


void MMatrix::row(int index, MatrixXf const &val) {
  assert(false);  // Check unused
  timers.start("row(index, val)");

  m_Xf.row(index) = val;
  m_Xf.eval();

  timers.stop("row(index, val)");
  used_fields(true, false);
}


void MMatrix::copy_block(MMatrix const &rhs, int from_offset, int to_offset, int in_size) {
  rhs.need_fields(false, true);
  assert(from_offset >= 0 && (from_offset + in_size <= rhs.size()));
  assert(to_offset >= 0 && (to_offset + in_size <= size()));

  auto &rhs_arr  = rhs.m_qpu.arr();

  //timers.start("copy_block qpu");
  // 130x faster than copy loop
  memcpy(m_qpu.arr().ptr() + to_offset, rhs_arr.ptr() + from_offset, sizeof(float)*in_size);
  //timers.stop("copy_block qpu");

  used_fields(false, true);
}


/**
 * Profile timing insignificant.
 */
void MMatrix::row(int index, MMatrix const &val) {
  assert(val.rows() == 1);
  assert(cols() == val.cols());
  assert(index >=0 && index < rows());

  val.need_fields(false, true);
  need_fields(val.m_using_Xf, true);

  bool used_Xf = false;

  if (val.m_using_Xf) {
    m_Xf.row(index) = val.Xf().row(0);
    m_Xf.eval();
    used_Xf = true;
  }

  // Scalar operation, but plenty fast enough
  //timers.start("row(index, MMatrix) qpu");
  for (int i = 0; i < val.cols(); ++i) {
    m_qpu.arr()[index*cols() + i] = val.m_qpu.arr()[i];
  }
  //timers.stop("row(index, MMatrix) qpu");

  used_fields(used_Xf, true);
}


/**
 * Profile timing minimal, inconsequential
 */
MMatrix MMatrix::row(int index) const {
  need_fields(false, true);

  MMatrix ret;
  ret.m_qpu = m_qpu.row(index);

  ret.used_fields(false, true);
  return ret;
}


MatrixXf const &MMatrix::Xf() const {
  need_fields(true, false);
  return m_Xf;
}


qpu::matrix const &MMatrix::qpu() const {
  //need_fields(false, true);
  assert(m_using_qpu);
  return m_qpu;
}


bool MMatrix::same(int bit_diff) const {
  // Only do this if both arrays present
  if (m_using_Xf && m_using_qpu) {
    return ::same(m_qpu, m_Xf, 0.0f, bit_diff);
  }

  return true;
}


bool MMatrix::same(MMatrix const &rhs, int bit_diff, bool show_max_diff) const {
  return same_intern(rhs, bit_diff, show_max_diff);
}


void MMatrix::diff(MMatrix const &rhs, int bit_diff) const {
  if (!(m_using_qpu && rhs.m_using_qpu)) {
		warn << "MMatrix::diff(): at least one qpu matrix missing";
		return;
	}

	qpu::diff(m_qpu, rhs.m_qpu, bit_diff);
}


bool MMatrix::same(MatrixXf const &rhs, int bit_diff, bool show_max_diff) const {
	warn << "Called same(MatrixXf)";
	assert(false);  // Warn me when called, want to refactor

  MMatrix tmp;
  tmp.set(rhs, true);

  return same_intern(tmp, bit_diff, show_max_diff);
}


/**
 * Profiling: time negligible
 */
bool MMatrix::same_intern(MMatrix const &rhs, int bit_diff, bool show_max_diff) const {
	warn << "Called same_intern()";
  assert(m_using_Xf || m_using_qpu);
  assert(rhs.m_using_Xf || rhs.m_using_qpu);
  assert((m_using_Xf == rhs.m_using_Xf) || (m_using_qpu == rhs.m_using_qpu));

  bool ret = true;

  // Internal checks
  if (m_using_Xf && m_using_qpu)         ret = ret && ::same(m_qpu, m_Xf, bit_diff);
  if (rhs.m_using_Xf && rhs.m_using_qpu) ret = ret && ::same(rhs.m_qpu, rhs.m_Xf, bit_diff);

  // Cross checks
  if (m_using_qpu && rhs.m_using_Xf)     ret = ret && ::same(m_qpu, rhs.m_Xf, bit_diff, show_max_diff);
  if (m_using_qpu && rhs.m_using_qpu)    ret = ret && qpu::same(m_qpu, rhs.m_qpu, bit_diff, show_max_diff);

  if (m_using_Xf && !m_using_qpu) {
    if (m_using_Xf  && rhs.m_using_Xf) {
      ret = ret && ::same_Xf(m_Xf, rhs.m_Xf, bit_diff,  show_max_diff);
    } else {
    	if (m_using_Xf  && rhs.m_using_qpu)    assert(false); // Deal with it when it happens
		}
  }

  return ret;
}


std::string MMatrix::dump_dim() const {
  std::string ret;

  if (m_using_Xf) {
    ret << ::dump_dim(m_Xf);
  } else {
    ret << "(?, ?)";
  }

  ret << ", ";

  if (m_using_qpu) {
    ret << m_qpu.dump_dim();
  } else {
    ret << "(?, ?)";
  }

  return ret;
}


std::string MMatrix::dump() const {
  std::string ret;
  ret << dump_dim() << ": \n"
      << "  m_Xf : ";

  if (m_using_Xf) {
    ret << ::dump(m_Xf);
  } else {
    ret << "[]";
  }

  ret << "\n  m_qpu: ";

  if (m_using_qpu) {
    ret << m_qpu.dump();
  } else {
    ret << "[]";
  }

  return ret;
}


MMatrix MMatrix::operator+(MMatrix const &rhs) const {
  //assert(false); // Warn me if called

  rhs.need_fields(true, true);
  need_fields(true, true);

  MMatrix ret;

  timers.start("MMatrix + Xf");
  ret.m_Xf = m_Xf + rhs.m_Xf;
   ret.eval();
  timers.stop("MMatrix + Xf");

  timers.start("MMatrix + qpu");
  ret.m_qpu = m_qpu + rhs.m_qpu;
  timers.stop("MMatrix + qpu");

  ret.used_fields(true, true);

  assert(ret.same());
  return ret;
}


MMatrix MMatrix::operator-(MMatrix const &rhs) const {
  rhs.need_fields(false, true);
  need_fields(false, true);

  MMatrix ret;
/*
  timers.start("MMatrix - Xf");
  ret.m_Xf = m_Xf - rhs.m_Xf;
   ret.eval();
  timers.stop("MMatrix - Xf");
*/
  //timers.start("MMatrix - qpu");
  // Timing 0.068ms, still 5x slower than Xf
  ret.m_qpu = m_qpu - rhs.m_qpu;
  //timers.stop("MMatrix - qpu");

  ret.used_fields(false, true);
  assert(ret.same());
  return ret;
}


void MMatrix::operator+=(MMatrix const &rhs) {
  rhs.need_fields(false, true);
  need_fields(false, true);

/*
  timers.start("MMatrix += Xf");
  m_Xf = m_Xf + rhs.m_Xf;
  timers.stop("MMatrix += Xf");
*/
  //timers.start("MMatrix += qpu");
  m_qpu += rhs.m_qpu;
  //timers.stop("MMatrix += qpu");

  used_fields(false, true);
}


void MMatrix::operator-=(MMatrix const &rhs) {
  rhs.need_fields(false, true);
  need_fields(false, true);
/*
  timers.start("MMatrix -= Xf");
  m_Xf = m_Xf - rhs.m_Xf;
  timers.stop("MMatrix -= Xf");
*/
//  timers.start("MMatrix -= qpu");
  // 2x faster than Xf
  m_qpu -= rhs.m_qpu;
//  timers.stop("MMatrix -= qpu");

  used_fields(false, true);
}


void MMatrix::operator*=(float val) {
  need_fields(false, true);
/*
  timers.start("MMatrix float *= Xf");
  m_Xf *= val;
  timers.stop("MMatrix float *= Xf");
*/
  //timers.start("MMatrix float *= qpu");
  // Max QPU's 2x faster than Xf
  m_qpu *= val;
  //timers.stop("MMatrix float *= qpu");

  used_fields(false, true);
  //assert(same());
}


void MMatrix::operator/=(float val) {
  assert(val > 0);
  //warn << "/=: " << dump();
  *this *= (1.0f/val);
}


MMatrix MMatrix::operator/(float val) const {
  assert(false); // Check not called

  timers.start("MMatrix /");
  MMatrix ret = *this;
  ret /= val;
  timers.stop("MMatrix /");
  return ret;
}


MMatrix MMatrix::operator*(MMatrix const &rhs) const {
  rhs.need_fields(false, true);
  need_fields(false, true);

  MMatrix ret;
/*
  timers.start("MMatrix * Xf");
  ret.m_Xf = m_Xf * rhs.m_Xf;
  timers.stop("MMatrix * Xf");
*/

  // Timing approximately equal to Xf, slightly better
//  timers.start("MMatrix * qpu");
  ret.m_qpu = m_qpu.mul_matrix(rhs.m_qpu);
//  timers.stop("MMatrix * qpu");

  ret.used_fields(false, true);
  //OK assert(ret.same());
  return ret;
}


/**
 * @brief Matrix multiplication with transposed matrices.
 *
 * The actual calculation is:
 *
 *     rhs * lhs^T;    // ^T - transposed
 */
MMatrix MMatrix::mul_t(MMatrix const &rhs, bool Xf_only) const {
  assert(!rhs.m_qpu.is_vector()); // Vector not supported any more
  rhs.need_fields(Xf_only, !Xf_only);
  need_fields(Xf_only, !Xf_only);

  MMatrix ret;

  if (Xf_only) {
    timers.start("MMatrix mul_t Xf");
    ret.m_Xf = rhs.m_Xf * m_Xf.transpose().eval();
    timers.stop("MMatrix mul_t Xf");
  } else {
    timers.start("MMatrix mul_t qpu matrix");  // Timing as good as possible 
    // 4x faster than Xf
    ret.m_qpu = rhs.m_qpu.mul_matrix_t(m_qpu);
    timers.stop("MMatrix mul_t qpu matrix");
  }

  ret.used_fields(Xf_only, !Xf_only);
  //OK assert(ret.same());
  return ret;
}


MMatrix MMatrix::operator*(float val) const {
  need_fields(false, true);

  MMatrix ret;
/*
  timers.start("MMatrix float * Xf");
  ret.m_Xf = val * m_Xf;
  timers.stop("MMatrix float * Xf");
*/
//  timers.start("MMatrix float * qpu");
  // Comparable to Xf
  ret.m_qpu = m_qpu * val;
//  timers.stop("MMatrix float * qpu");

  ret.used_fields(false, true);
  //warn << "MMatrix float * ret" << ret.dump();
  //OK assert(ret.same());
  return ret;
}


/**
 * @brief Per-element product of two matrices.
 */
MMatrix MMatrix::mul_e(MMatrix const &rhs) const {
  rhs.need_fields(false, true);
  need_fields(false, true);

  MMatrix ret;
/*
  timers.start("MMatrix mul_e Xf");
  ret.m_Xf = m_Xf.cwiseProduct(rhs.m_Xf);
  timers.stop("MMatrix mul_e Xf");
*/
//  timers.start("MMatrix mul_e qpu");
  // 3x slower than Xf
  ret.m_qpu = m_qpu.mul_e(rhs.m_qpu);
//  timers.stop("MMatrix mul_e qpu");

  ret.used_fields(false, true);
  return ret;
}


MMatrix MMatrix::div_e(MMatrix const &rhs) const {
  need_fields(false, true);

  MMatrix ret(rows(), cols());
  gru_kernel::divide_vector(ret.m_qpu, m_qpu, rhs.m_qpu);

  ret.need_fields(false, true);
  return ret;
}


MMatrix MMatrix::tanh() const {
  //need_fields(true, true);
  assert(m_using_Xf || m_using_qpu);

  MMatrix ret;

  if (m_using_Xf) {
    timers.start("MMatrix tanh Xf");
    ret.m_Xf = m_Xf.unaryExpr(&::tanh_activation);
    timers.stop("MMatrix tanh Xf");
  }

  if (m_using_qpu) {
    timers.start("MMatrix tanh qpu");
    ret.m_qpu = m_qpu.tanh();
    timers.stop("MMatrix tanh qpu");
  }

  //ret.used_fields(true, true);
  ret.used_fields(m_using_Xf, m_using_qpu);
  //assert(ret.same(11));       // Very bad convergence, 11 often not enough
  return ret;
}


MMatrix MMatrix::sigmoid() const {
  need_fields(false, true);

  MMatrix ret;
/*
  timers.start("MMatrix sigmoid Xf");
  ret.m_Xf = m_Xf.unaryExpr(&::sigmoid);
  timers.stop("MMatrix sigmoid Xf");
*/
  //timers.start("MMatrix sigmoid qpu");
  // Timing 0.046ms, still 9x slower than Xf
  ret.m_qpu = m_qpu.sigmoid();
  //timers.stop("MMatrix sigmoid qpu");

  ret.used_fields(false, true);
  assert(ret.same());
  return ret;
}


/**
 * Currently unused
 */
MMatrix MMatrix::outer(MMatrix const &rhs) const {
  //assert(false);  // Warn me when called

  rhs.need_fields(true, true);
  need_fields(true, true);

  MMatrix ret;
  timers.start("MMatrix outer Xf");
  ret.m_Xf  = m_Xf.transpose().eval() * rhs.m_Xf;
  timers.stop("MMatrix outer Xf");

  timers.start("MMatrix outer qpu");
  ret.m_qpu = m_qpu.outer(rhs.m_qpu);
  timers.stop("MMatrix outer qpu");

  //OK assert(same());
  ret.used_fields(true, true);
  return ret;
}


void MMatrix::outer_add(MMatrix const &lhs, MMatrix const &rhs) {
  lhs.need_fields(false, true);
  rhs.need_fields(false, true);
  need_fields(false, true);

/*  
  timers.start("MMatrix outer_add Xf");
  m_Xf  = m_Xf + lhs.m_Xf.transpose().eval() * rhs.m_Xf;
  timers.stop("MMatrix outer_add Xf");
*/  

  timers.start("MMatrix outer_add qpu");
  m_qpu.outer_add(lhs.m_qpu, rhs.m_qpu);
  timers.stop("MMatrix outer_add qpu");

  used_fields(false, true);
}


/**
 * This is not a full outer product of tensors,
 * but a per-row calculation of outer products, which are all added to current instance.
 *
 * Note that the rows of rhs are technically transposed for the outer products.
 */
void MMatrix::outer_add_rows(MMatrix const &lhs, MMatrix const &rhs) {
  lhs.need_fields(false, true);
  rhs.need_fields(false, true);
  need_fields(false, true);

  int lhs_rows = lhs.rows();

  assert(lhs_rows == rhs.rows());
  assert(lhs_rows > 1);

  //timers.start("outer_add_rows");
  m_qpu.outer_add_rows(lhs.m_qpu, rhs.m_qpu);
  //timers.stop("outer_add_rows");

  used_fields(false, true);
}


void MMatrix::outer_rows(MMatrix const &lhs, MMatrix const &rhs) {
  assert(false); // Check unused

  assert(lhs.rows() == rhs.rows());
  assert(lhs.rows() > 1);
  MMatrix tmp(lhs.cols(), rhs.cols());

  for (int i = 0; i < lhs.rows(); ++i) {
    tmp.outer_add(lhs.row(i), rhs.row(i));
  }

  set(tmp);
}


/**
 * TODO: make private eventually
 */
MMatrix MMatrix::max_row() const {
  need_fields(false, true);

  MMatrix ret(rows(), 1);

  timers.start("max_row scalar");

  for (int r = 0; r < rows(); r++) {
    bool did_first = false;
    float val;

    for (int c = 0; c < cols(); c++) {
      float in_val = m_qpu.at(r, c);

      if (!did_first) {
        val = in_val;
        did_first = true;
        continue;
      }

      if ( val < in_val) {
        val = in_val;
      }
    }

    ret.m_qpu.at(r, 0) = val;
  }

  ret.used_fields(false, true);

  timers.stop("max_row scalar");

  // 2.5x slower for (1,64) src matrix
  timers.start("max_row qpu");

  MMatrix ret2(rows(), 1);
   m_qpu.max_row(ret2.m_qpu);
  ret2.used_fields(false, true);

  timers.stop("max_row qpu");

  //warn << "ret: " << ret.dump();
  //warn << "ret2: " << ret2.dump();
  assert(ret.same(ret2));

  return ret;
}


/**
 * @brief Calculate sums per row
 *
 * TODO: convert to QPU, currently scalar
 */
MMatrix MMatrix::sum_row() const {
  assert(m_using_Xf || m_using_qpu);
  //need_fields(true, true);

  int height = rows();
  int width  = cols();

  MMatrix ret(height, 1);
  //ret.need_fields(true, true);
  ret.need_fields(m_using_Xf, m_using_qpu);

  if (m_using_Xf) {
    timers.start("sum_row Xf");
    for (int r = 0; r < height; r++) {
      ret.m_Xf(r, 0)  = m_Xf.row(r).sum();
    }
    timers.stop("sum_row Xf");
  }

  if (m_using_qpu) {
    timers.start("sum_row qpu");

    for (int r = 0; r < height; r++) {
      float val = 0.0f;

      for (int c = 0; c < width; c++) {
        float in_val = m_qpu.at(r, c);
        val += in_val;
      }

      //warn << "r, val: " << r << ", " << val;
      ret.m_qpu.at(r, 0) = val;
    }

    timers.stop("sum_row qpu");
  }

  return ret;
}


/**
 * =================================
 * Notes
 * -----
 *
 * - A direct sum() call returns a different value
 *
 *     float temp_sum = temp_output.Xf().sum();
 *     float temp_sum_1 = temp_output.sum();
 *
 *     WARNING: diff: 3.814697e-06
 *
 *   It should be exact, the underlying Xf calculation is identical.
 *   Unclear why this happens, ignoring.
 */
float MMatrix::sum() const {
  assert(m_qpu.is_vector());

  MMatrix tmp = sum_row();
  //OK assert(tmp.same());
  return tmp.m_qpu.at(0,0);
}


/**
 * Calculate softmax per row
 */
void MMatrix::softmax() {
  assert(false); // Warn me when called

  need_fields(true, true);
  //warn << "softmax rows: " << rows();  // always 1 in current implementation
  //warn << "softmax pre: " << dump();

  MMatrix max = max_row();

  for (int r = 0; r < rows(); r++) {
    auto tmp = row(r);
    tmp.need_fields(true, true);

    timers.start("softmax Xf");

    // s_max is a global used in softmax()
    s_max    = tmp.m_Xf.maxCoeff();
    tmp.m_Xf = tmp.m_Xf.unaryExpr(&::s_softmax);
    tmp.m_Xf.eval();

    timers.stop("softmax Xf");

    timers.start("softmax qpu");
    tmp.m_qpu.softmax(max.m_qpu);
    timers.stop("softmax qpu");

    row(r, tmp);
  }

  used_fields(true, true);
  assert(same());
}


/**
 * @brief Replace every element of matrix with inverse of value (1/val).
 *
 * TODO: scalar operation, consider making qpu.
 * TODO: prob not necessary, examine
 */
MMatrix MMatrix::invert() const {
  need_fields(false, true);
  assert(size() > 0);

  MMatrix ret(rows(), cols());

  for (int i = 0; i < size(); i++) {
    ret.m_qpu.arr()[i] = 1.0f/m_qpu.arr()[i];
  }

  ret.used_fields(false, true);
  return ret;
}


void MMatrix::back_prop_1(MMatrix const &ds_cur, State const &temp) {
  ds_cur.need_fields(false, true);
  temp.h.need_fields(false, true);
  temp.z.need_fields(false, true);
/*
  timers.start("back_prop_1 Xf");
  // Xf disabled for improving need_fields()
  MMatrix ones(ds_cur.rows(), ds_cur.cols(), 1.0f, true);
  m_Xf = ds_cur.m_Xf.cwiseProduct(ones.m_Xf - temp.z.m_Xf).cwiseProduct(temp.h.Xf().unaryExpr(&tanh_grad));  //.cwiseProduct(temp_S.unaryExpr(&tanh_grad));
  timers.stop("back_prop_1 Xf");
*/
  m_qpu.resize(ds_cur.rows(), ds_cur.cols());

  //timers.start("back_prop_1 qpu");
  // QPU 8x WORSE than Xf
  gru_kernel::back_prop_1(m_qpu, ds_cur.m_qpu, temp.z.m_qpu, temp.h.m_qpu);
  //timers.stop("back_prop_1 qpu");

  used_fields(false, true);
}


void MMatrix::back_prop_3(MMatrix const &dsr, State const &temp) {
  dsr.need_fields(false, true);
  temp.S.need_fields(false, true);
  temp.r.need_fields(false, true);
/*
  timers.start("back_prop_3 Xf");
  //m_Xf = dsr.m_Xf.cwiseProduct(temp.S.Xf()).cwiseProduct(temp.r.Xf().unaryExpr(&sigmoid_grad));
  MatrixXf tmp1 = dsr.m_Xf.cwiseProduct(temp.S.Xf());
  MatrixXf tmp2 = temp.r.Xf().unaryExpr(&sigmoid_grad);
  m_Xf = tmp1.cwiseProduct(tmp2);
  timers.stop("back_prop_3 Xf");
*/
  m_qpu.resize(dsr.rows(), dsr.cols());

  //timers.start("back_prop_3 qpu");
  gru_kernel::back_prop_3(m_qpu, dsr.m_qpu, temp.S.m_qpu, temp.r.m_qpu);
  //timers.stop("back_prop_3 qpu");

  used_fields(false, true);
}


/**
 * Xf/qpu slowly diverge upon sequential loops.
 */
void MMatrix::back_prop_4(MMatrix const &ds_cur_bk, State const &temp) {
  ds_cur_bk.need_fields(false, true);
  temp.S.need_fields(false, true);

/*
  // Xf actually performs slightly better than qpu

  timers.start("back_prop_4 Xf");
  auto dz = ds_cur_bk.Xf().cwiseProduct(temp.S.Xf() - temp.h.Xf());
  m_Xf = dz.cwiseProduct(temp.z.Xf().unaryExpr(&sigmoid_grad));
  timers.stop("back_prop_4 Xf");
*/  

  m_qpu.resize(ds_cur_bk.rows(), ds_cur_bk.cols());

  //timers.start("back_prop_4 qpu");
  gru_kernel::back_prop_4(m_qpu, ds_cur_bk.qpu(), temp.z.qpu(), temp.S.qpu(), temp.h.qpu());
  used_fields(false, true);
  //timers.stop("back_prop_4 qpu");

  //assert(same());  // convergence Xf/qpu gets progressively worse
}


void MMatrix::divide_matrix(MMatrix const &gradient, MMatrix const &in_cache) {
  gradient.need_fields(false, true);
  in_cache.need_fields(false, true);
/*
  timers.start("divide_matrix Xf");

  auto const &grad  = gradient.Xf();
  auto const &cache = in_cache.Xf();

  for (int i = 0; i < m_Xf.rows(); ++i) {
    for (int j = 0; j < m_Xf.cols(); ++j) {
      m_Xf(i, j) = grad(i, j) / (float) (sqrt(cache(i, j)) + 0.00000001f);
    }
  }

  timers.stop("divide_matrix Xf");
*/  
  //timers.start("divide_matrix qpu");

  m_qpu.resize(gradient.rows(), gradient.cols());

  // Single QPU 9x faster than Xf
  gru_kernel::divide_matrix(m_qpu, gradient.m_qpu, in_cache.m_qpu);

  //timers.stop("divide_matrix qpu");

  //OK assert(same());
  used_fields(false, true);
}


MMatrix MMatrix::ln() const {
  need_fields(false, true);

  MMatrix ret(rows(), cols());
/*
  timers.start("ln Xf");
  ret.m_Xf  = m_Xf.unaryExpr(&log_matrix);
  timers.stop("ln Xf");
*/
  //timers.start("ln qpu");
  // Timing 0.044ms, still 22x slower than Xf
  ret.m_qpu = m_qpu.ln();
  //timers.stop("ln qpu");

  ret.used_fields(false, true);
  return ret;
}


MMatrix MMatrix::calc_E(MMatrix const &Y, MMatrix const &O) const {
  assert(m_qpu.is_vector());

  timers.start("calc_E");

  MMatrix temp_ln = O.ln();
  assert(temp_ln.same());

  MMatrix ret = -1.0f * Y.mul_e(temp_ln).sum_row();
  // DON'T DO THIS, done in calc: ret.used_fields(true, true);

  timers.stop("calc_E");
  assert(ret.same());
  return ret;
}


/**
 * Timing insignificant, average 0 for both Xf and qpu
 */
void MMatrix::col_E(int index, MMatrix const &rhs) {
  rhs.need_fields(false, true);
  assert(rhs.size() == 1);
  assert(rows() == 1);
  assert(0 <= index && index < cols());
/*
  timers.start("col_E Xf");
  m_Xf(0, index) += rhs.m_Xf(0,0);
  timers.stop("col_E Xf");
*/
//  timers.start("col_E qpu");
  m_qpu.at(0, index) += rhs.m_qpu.at(0,0);
//  timers.stop("col_E qpu");

  used_fields(false, true);
  //assert(same());
}


/**
 * `this` corresponds to X.
 *
 * Derived from:
 *
 *     MMatrix temp2 = (x_X * m.U_z) + (x_state.S*m.W_z);
 */
MMatrix MMatrix::forward_1(Model &m, MMatrix &S) {
  S.need_fields(false, true);
  m.U_z.need_fields(false, true);
  m.W_z.need_fields(false, true);
  need_fields(false, true);

  MMatrix ret(rows(), m.U_z.cols());
/*
  timers.start("forward_1 Xf");
  ret.m_Xf = (Xf() * (m.U_z.Xf())) + (S.Xf() * (m.W_z.Xf()));
  ret.m_Xf.eval();
  timers.stop("forward_1 Xf");
*/
  //timers.start("forward_1 qpu");

  // Timing approximately equal to Xf
  //ret.m_qpu = (qpu() * (m.U_z.qpu())) + (S.qpu() * (m.W_z.qpu()));

  // 2.5x faster than QPU atomic
  gru_kernel::mult_matrix_col_add(ret.m_qpu, m_qpu, m.U_z.m_qpu, S.m_qpu, m.W_z.m_qpu);

  //timers.stop("forward_1 qpu");

  ret.used_fields(false, true);
  //OK assert(ret.same());
  return ret;
}


/**
 * `this` corresponds to X.
 *
 * Derived from:
 *
 *     temp = (X_row.Xf() * (m.U_r.Xf())) + (S_row.Xf() * (m.W_r.Xf()));
 */
MMatrix MMatrix::forward_2(Model &m, MMatrix &S) {
  S.need_fields(false, true);
  m.U_r.need_fields(false, true);
  m.W_r.need_fields(false, true);
  need_fields(false, true);

  MMatrix ret(rows(), m.U_r.cols());
/*
  timers.start("forward_2 Xf");
  ret.m_Xf = (Xf() * (m.U_r.Xf())) + (S.Xf() * (m.W_r.Xf()));
  ret.m_Xf.eval();
  timers.stop("forward_2 Xf");
*/  

  //timers.start("forward_2 qpu");
  // Timing approximately equal to Xf
  //ret.m_qpu = (qpu() * (m.U_r.qpu())) + (S.qpu() * (m.W_r.qpu()));

  // 2.5x faster than QPU atomic
  gru_kernel::mult_matrix_col_add(ret.m_qpu, m_qpu, m.U_r.m_qpu, S.m_qpu, m.W_r.m_qpu);

  //timers.stop("forward_2 qpu");

  ret.used_fields(false, true);
  //OK assert(ret.same());
  return ret;
}


/**
 * `this` corresponds to X.
 *
 * Derived from:
 *
 *     MatrixXf tempb = (X_row.Xf() * (m.U_h.Xf())) + (S_row.Xf().cwiseProduct(state.r.row(i).Xf())) * (m.W_h.Xf());
 */
MMatrix MMatrix::forward_3(Model &m, MMatrix &S, MMatrix const &r_row) {
  r_row.need_fields(false, true);
  S.need_fields(false, true);
  m.U_h.need_fields(false, true);
  m.W_h.need_fields(false, true);
  need_fields(false, true);

  MMatrix ret(rows(), m.U_h.cols());
/*
  timers.start("forward_3 Xf");
   ret.m_Xf = (Xf() * (m.U_h.Xf())) + (S.Xf().cwiseProduct(r_row.Xf())) * (m.W_h.Xf());
  ret.m_Xf.eval();
  timers.stop("forward_3 Xf");
*/
  //timers.start("forward_3 qpu");

  // Timing slightly larger than Xf ~20%
   //ret.m_qpu = (qpu() * (m.U_h.qpu())) + (S.qpu().mul_e(r_row.qpu())) * (m.W_h.qpu());

  // 2.5x faster than QPU atomic, including tmp
   auto tmp = S.qpu().mul_e(r_row.qpu());
  gru_kernel::mult_matrix_col_add(ret.m_qpu, m_qpu, m.U_h.m_qpu, tmp, m.W_h.m_qpu);

  //timers.stop("forward_3 qpu");

  ret.used_fields(false, true);
  return ret;
}


/**
 * `this` corresponds to z_row.
 *
 * Derived from:
 *
 *     temp_hidden = (ones - z_row.Xf()).cwiseProduct(state.h.row(i).Xf() + z_row.Xf()).cwiseProduct(S_row.Xf());
 */
MMatrix MMatrix::forward_4(MMatrix &S, MMatrix const &h_row) {
  S.need_fields(false, true);
  h_row.need_fields(false, true);
  need_fields(false, true);

  MMatrix ret(rows(), cols());
/*
  timers.start("forward_4 Xf");
  MatrixXf ones = MatrixXf::Ones(S.rows(), S.cols());
  ret.m_Xf = (ones - Xf()).cwiseProduct(h_row.Xf() + Xf()).cwiseProduct(S.Xf());
  ret.m_Xf.eval();
  timers.stop("forward_4 Xf");
*/
  //timers.start("forward_4 qpu");
  // Timing 0.046ms, still 9x slower than Xf
  gru_kernel::forward_4(ret.m_qpu, m_qpu,  h_row.m_qpu, S.m_qpu);
  //timers.stop("forward_4 qpu");

  ret.used_fields(false, true);
  //OK assert(ret.same(2));  // Usually exact
  return ret;
}


/**
 * Again, differences with direct calculation: `diff: 2.793968e-08` (max detected)
 *
 * Unknown why, ignoring.
 */
MMatrix MMatrix::forward_5() const {
/*  
  timers.start("forward_5");
  MMatrix ret = *this;
  ret.softmax();
  float temp_sum = ret.sum();
  ret /= temp_sum;

  ret.used_fields(true, true);
  //warn << "forward_5 ret: " << ret.dump();
  timers.stop("forward_5");
*/

  //timers.start("forward_5 qpu");
  MMatrix ret2 = *this;
  // single-QPU 8x faster than combined Xf/QPU; with bit_diff == 3
  gru_kernel::forward_5(ret2.m_qpu);
  ret2.used_fields(false, true);
  //timers.stop("forward_5 qpu");

  return ret2;
}


void MMatrix::set_decay(float decay, MMatrix const &rhs) {
  rhs.need_fields(false, true);
  need_fields(false, true);
/*
  timers.start("set_decay Xf");
  m_Xf = decay * m_Xf + (1 - decay) * (rhs.m_Xf.cwiseProduct(rhs.m_Xf)).eval();
  timers.stop("set_decay Xf");
*/
  //timers.start("set_decay qpu");
  gru_kernel::set_decay(m_qpu, rhs.m_qpu, decay);
  //timers.stop("set_decay qpu");

  //OK assert(::same(m_qpu, m_Xf));
  used_fields(false, true);
}


void MMatrix::need_fields(bool need_XF, bool need_qpu) const {
  static int call_count = 0;  // Recursion check

  call_count++;
  if (call_count > 1) {
    warn << "need_fields " << call_count;
    breakpoint;
  }

  if (need_XF && !m_using_Xf) {
    assert(m_using_qpu);

    timers.start("need_fields qpu->Xf");

    m_Xf = MatrixXf::Zero(m_qpu.rows(), m_qpu.columns());

    for (int i = 0; i < m_qpu.rows(); i++) {
      for (int j = 0; j < m_qpu.columns(); j++) {
        m_Xf(i, j) = m_qpu.at(i, j);
      }
    }
    timers.stop("need_fields qpu->Xf");

    m_using_Xf = true;
  }

  if (need_qpu && !m_using_qpu) {
    assert(m_using_Xf);

    timers.start("need_fields Xf->qpu");
    copy_m(m_qpu, m_Xf);
    timers.stop("need_fields Xf->qpu");

    m_using_qpu = true;
  }

  call_count--;
}


bool same(qpu::matrix const &lhs, MatrixXf const &rhs, int bit_diff, bool show_max_diff) {
  MMatrix rhs_temp;
  rhs_temp.set(rhs, true);

  return same(lhs, rhs_temp.qpu(), bit_diff, show_max_diff);
}


/**
 * ========================================
 *
 * - Use direct buffers:
 *   * [Copying from buffer](https://stackoverflow.com/a/39660576/1223531)
 *   * [Eigen is col-major, how to handle](https://runebook.dev/en/docs/eigen3/group__tutorialmapclass?page=2)
 */
void copy_m(qpu::matrix &dst, MatrixXf const &rhs) {
  int height = (int) rhs.rows();
  int width  = (int) rhs.cols();

  dst.resize(height, width);
  if (height*width == 0) return;

  timers.start("copy_m dst");

  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      dst.at(i, j) = rhs(i, j);
    }
  }

  timers.stop("copy_m dst");
}


qpu::matrix copy_m(MatrixXf const &rhs) {
  timers.start("copy_m");
  qpu::matrix ret;
  copy_m(ret, rhs);
  timers.stop("copy_m");
  return ret;
}


std::string dump(MatrixXf const &m) {
  std::string buf;
  buf << dump_dim(m) << " ";

  if (m.rows() * m.cols() == 0) {
    buf = "[]";
    return buf;
  }
 
  buf  << "[\n";

  for (int i = 0; i < m.rows(); ++i) {
    buf << "  " << i << ": [";

    for (int j = 0; j < m.cols(); ++j) {
      if (m(i,j) == 0.0f) {
        buf << "0";
      } else {
        buf  << m(i,j);
      }

      buf << ", ";
    }
    buf << "]\n";
  }
  buf << "]";

  return buf;
}


void MMatrix::move_rows(int step, MMatrix const &rhs) {
  rhs.need_fields(false, true);
  if (step == 0) {
    m_qpu = rhs.m_qpu;
    need_fields(false, true);
    return;
  }

  assert(step > 0);
  assert(abs(step) < rhs.rows()); // Originally meant to move up as well

  int rhs_rows = rhs.rows();
  int rhs_cols = rhs.cols();

  assert(m_qpu.rows() == rhs_rows && m_qpu.columns() == rhs_cols);
  // Retain the lhs value at i == 0

  //timers.start("move_rows block");

  int to_offset = step*rhs_cols;
  int size      = rhs.size() - to_offset;

  copy_block(rhs, 0, to_offset, size);

  //timers.stop("move_rows block");

  used_fields(false, true);
}


MMatrix remove_last_rows(int num, MMatrix const &rhs) {
  if (num == 0) return rhs; // Nothing to do

  assert(num > 0 && num < rhs.rows());

  timers.start("remove_last_rows");

  MMatrix ret(rhs.rows() - num, rhs.cols());

  for (int i = 0; i < rhs.rows() - num; ++i) {
    ret.row(i, rhs.row(i));
  }

  timers.stop("remove_last_rows");

  return ret;
}
