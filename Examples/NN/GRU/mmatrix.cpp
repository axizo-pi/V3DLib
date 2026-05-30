#include "mmatrix.h"
#include "./kernel.h"
#include "global.h"
#include "model.h"
#include "Support/Helpers.h"  // bit_diff()

namespace {

float s_max = 0;

float softmax(float x) {
  return (float) exp(x - s_max);
}

}

MMatrix::MMatrix() : m_qpu(true) {}

MMatrix::MMatrix(int rows, int columns, float val, bool set_Xf) {
  assert(rows > 0);
  assert(columns > 0);
  assert(val == 0.0f || val == 1.0f);

  if (set_Xf) {
    if (val == 0.0f) {
      m_Xf = MatrixXf::Zero(rows, columns);
    } else {
      m_Xf = MatrixXf::Ones(rows, columns);
    }
    used_fields(true, false);
  } else {
    m_qpu.resize(rows,columns);
    m_qpu.set(val);
    used_fields(false, true);
  }
}


void MMatrix::resize(int rows, int columns, float val) {
  *this = MMatrix(rows, columns, val);
}


MMatrix::MMatrix(MMatrix const &rhs) {
  set(rhs);
}


void MMatrix::set(MatrixXf const &rhs, bool set_qpu) {
  m_Xf = rhs;

  if (set_qpu) {
    m_qpu = copy_m(m_Xf);
    used_fields(true, true);
  } else {
    used_fields(true, false);
  }
}


void MMatrix::set(MMatrix const &rhs) {
  if (rhs.m_using_Xf) {
		m_Xf = rhs.m_Xf;
	}

  if (rhs.m_using_qpu) {
  	m_qpu = rhs.m_qpu;
	}

  used_fields(rhs.m_using_Xf, rhs.m_using_qpu);
	//warn << "set(): " << m_qpu.dump();
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


MMatrix MMatrix::transpose() const {
  need_fields(true, false);
  assert(rows() == 1 || cols() == 1);  // Simple case, vectors only

  MMatrix ret(cols(), rows());

  ret.m_Xf = m_Xf.transpose();

  ret.used_fields(true, false);
  return ret;
}


void MMatrix::row(int index, MatrixXf const &val) {
  timers.start("row(index, val)");

  m_Xf.row(index) = val;
  m_Xf.eval();

  timers.stop("row(index, val)");
  used_fields(true, false);
}


void MMatrix::copy_row(int from_index, int to_index, MMatrix const &val) {
  assert(false);  // Check unused
  assert(cols() == val.cols());
  assert(from_index >= 0 && from_index < val.rows());
  assert(to_index >= 0 && to_index < rows());

  val.need_fields(false, true);

  timers.start("copy_row");

  auto &lhs_arr  = m_qpu.arr();
  int lhs_offset = to_index*cols();

  auto &rhs_arr  = val.m_qpu.arr();
  int rhs_offset = from_index*val.cols();

  for (int i = 0; i < val.cols(); ++i) {
    lhs_arr[lhs_offset++] = rhs_arr[rhs_offset++];
  }

  timers.stop("copy_row");
  used_fields(false, true);
}


void MMatrix::copy_block(MMatrix const &rhs, int from_offset, int to_offset, int in_size) {
  assert(from_offset >= 0 && (from_offset + in_size <= rhs.size()));
  assert(to_offset >= 0 && (to_offset + in_size <= size()));

  auto &lhs_arr  = m_qpu.arr();
  auto &rhs_arr  = rhs.m_qpu.arr();

  for (int i = 0; i < in_size; ++i) {
    lhs_arr[to_offset++] = rhs_arr[from_offset++];
  }
}


void MMatrix::row(int index, MMatrix const &val) {
  assert(val.rows() == 1);
  assert(cols() == val.cols());
  assert(index >=0 && index < rows());

  val.need_fields(false, true);
  need_fields(false, true);

	bool used_Xf = false;

	if (val.m_using_Xf) {
		//warn << "row doing Xf";
	  m_Xf.row(index) = val.Xf().row(0);
		m_Xf.eval();
		used_Xf = true;
/*		
	} else {
		warn << "row skipping Xf";
*/		
	}

  for (int i = 0; i < val.cols(); ++i) {
    m_qpu.arr()[index*cols() + i] = val.m_qpu.arr()[i];
  }

  used_fields(used_Xf, true);
}


MMatrix MMatrix::row(int index) const {
  need_fields(false, true);

  //timers.start("row(index)");  // Time minimal, inconsequential

  MMatrix ret;
  ret.m_qpu = m_qpu.row(index);

  //timers.stop("row(index)");

  ret.used_fields(false, true);
  return ret;
}


MatrixXf const &MMatrix::Xf() const {
  need_fields(true, false);
  return m_Xf;
}


qpu::matrix const &MMatrix::qpu() const {
  need_fields(false, true);
  return m_qpu;
}


bool MMatrix::same(float precision) const {
  // Only do this if both arrays present
  if (m_using_Xf && m_using_qpu) {
		return ::same(m_qpu, m_Xf, precision, -1);
  }

  return true;
}


bool MMatrix::same_b(int bit_diff) const {
  // Only do this if both arrays present
  if (m_using_Xf && m_using_qpu) {
		return ::same(m_qpu, m_Xf, 0.0f, bit_diff);
  }

  return true;
}


bool MMatrix::same(MatrixXf const &rhs, float precision, bool show_max_diff) const {
  MMatrix tmp;
  tmp.set(rhs, true);

  return same_intern(tmp, precision, -1, show_max_diff);
}


bool MMatrix::same_b(MMatrix const &rhs, int bit_diff, bool show_max_diff) const {
	return same_intern(rhs, 0.0f, bit_diff, show_max_diff);
}


bool MMatrix::same_b(MatrixXf const &rhs, int bit_diff, bool show_max_diff) const {
  MMatrix tmp;
  tmp.set(rhs, true);

  return same_intern(tmp, 0.0f, bit_diff, show_max_diff);
}

/**
 * Profiling: time negligible
 */
bool MMatrix::same(MMatrix const &rhs, float precision, bool show_max_diff) const {
	return same_intern(rhs, precision, -1, show_max_diff);
}


/**
 * Profiling: time negligible
 */
bool MMatrix::same_intern(MMatrix const &rhs, float precision, int bit_diff, bool show_max_diff) const {
  assert(m_using_Xf || m_using_qpu);
  assert(rhs.m_using_Xf || rhs.m_using_qpu);
  assert((m_using_Xf == rhs.m_using_Xf) || (m_using_qpu == rhs.m_using_qpu));

  if (m_using_Xf && m_using_qpu) {
    return
      ::same(m_qpu, m_Xf, precision, bit_diff) &&
      ::same(rhs.m_qpu, rhs.m_Xf, precision, bit_diff) &&
      ::same(m_qpu, rhs.m_Xf, precision, bit_diff, show_max_diff);
  }

  if (m_using_qpu) {
    return ::same(m_qpu, rhs.m_qpu, precision, bit_diff, show_max_diff);
  }

  assert(false);  // safeguard for m_Xf
}


std::string MMatrix::dump_dim() const {
  assert(m_using_Xf || m_using_qpu);

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
  //need_fields(true, true);

  std::string ret;
  ret << dump_dim() << ": \n"
      << "  m_Xf : " << ::dump(m_Xf) << "\n"
      << "  m_qpu: " << m_qpu.dump();
  return ret;
}


MMatrix MMatrix::operator+(MMatrix const &rhs) const {
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

  // See TODO operator -
  assert(ret.same(4*Precision));
  return ret;
}


MMatrix MMatrix::operator-(MMatrix const &rhs) const {
  rhs.need_fields(true, true);
  need_fields(true, true);
  //assert(rhs.same());
  //assert(same());
  //warn <<"this -: " << dump();

  MMatrix ret;

  timers.start("MMatrix - Xf");
  ret.m_Xf = m_Xf - rhs.m_Xf;
   ret.eval();
  timers.stop("MMatrix - Xf");

  timers.start("MMatrix - qpu");
  ret.m_qpu = m_qpu - rhs.m_qpu;
  timers.stop("MMatrix - qpu");

  ret.used_fields(true, true);

  // The following should be EXACT.
  // I investigated this but can not find a reason
  // TODO: examine further
  assert(ret.same()); //4*Precision));
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
  rhs.need_fields(true, true);
  need_fields(true, true);

  //timers.start("MMatrix -= Xf");
  m_Xf = m_Xf - rhs.m_Xf;
  //timers.stop("MMatrix -= Xf");

  //timers.start("MMatrix -= qpu");
  m_qpu -= rhs.m_qpu;
  //timers.stop("MMatrix -= qpu");

  used_fields(true, true);
}


void MMatrix::operator/=(float steps) {
	assert(steps > 0);
	*this *= (1.0f/steps);
}


void MMatrix::operator*=(float val) {
  need_fields(true, true);

	timers.start("MMatrix float *= Xf");
  m_Xf *= val;
	timers.stop("MMatrix float *= Xf");

	timers.start("MMatrix float *= qpu");
	m_qpu = m_qpu * val;
	timers.stop("MMatrix float *= qpu");

  used_fields(true, true);
	assert(same_b());
}


MMatrix MMatrix::operator/(float steps) const {
	assert(false); // Check not called

	timers.start("MMatrix /");
	MMatrix ret = *this;
 	ret /= steps;
	timers.stop("MMatrix /");
	return ret;
}


MMatrix MMatrix::operator*(MMatrix const &rhs) const {
  rhs.need_fields(true, true);
  need_fields(true, true);

  MMatrix ret;

  timers.start("MMatrix * Xf");
  ret.m_Xf = m_Xf * rhs.m_Xf;
  timers.stop("MMatrix * Xf");

	// Timing approximately equal to Xf
  timers.start("MMatrix * qpu");
  ret.m_qpu = m_qpu.mul_matrix(rhs.m_qpu);
  timers.stop("MMatrix * qpu");

  ret.used_fields(true, true);
  assert(ret.same_b());
  return ret;
}


/**
 * @brief Matrix multiplication with transposed matrices.
 *
 * The actual calculation is:
 *
 *     rhs * lhs^T;    // ^T - transposed
 */
MMatrix MMatrix::mul_t(MMatrix const &rhs) const {
  rhs.need_fields(false, true);
  need_fields(false, true);

  MMatrix ret;
/*
  timers.start("MMatrix mul_t Xf");
  ret.m_Xf = rhs.m_Xf * m_Xf.transpose().eval();
  timers.stop("MMatrix mul_t Xf");
*/
  if (rhs.m_qpu.is_vector()) {
    assert(false); // Check not used
    timers.start("MMatrix mul_t qpu vec");
    ret.m_qpu = m_qpu * rhs.m_qpu;
    timers.stop("MMatrix mul_t qpu vec");
  } else {
    //timers.start("MMatrix mul_t qpu matrix");  // Timing as good as possible 
    ret.m_qpu = rhs.m_qpu.mul_matrix_t(m_qpu);
    //timers.stop("MMatrix mul_t qpu matrix");
  }

  //OK assert(ret.same());
  ret.used_fields(false, true);
  return ret;
}


MMatrix MMatrix::operator*(float val) const {
  need_fields(true, true);

  MMatrix ret;

  timers.start("MMatrix float * Xf");
  ret.m_Xf = val * m_Xf;
  timers.stop("MMatrix float * Xf");

  timers.start("MMatrix float * qpu");
  ret.m_qpu = m_qpu * val;
  timers.stop("MMatrix float * qpu");

  ret.used_fields(true, true);
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
  timers.start("MMatrix mul_e qpu");
  // 3x slower than Xf
  ret.m_qpu = m_qpu.mul_e(rhs.m_qpu);
  timers.stop("MMatrix mul_e qpu");

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
  need_fields(true, true);

  MMatrix ret;

  timers.start("MMatrix tanh Xf");
  ret.m_Xf = m_Xf.unaryExpr(&::tanh_activation);
  timers.stop("MMatrix tanh Xf");

  timers.start("MMatrix tanh qpu");
  ret.m_qpu = m_qpu.tanh();
  timers.stop("MMatrix tanh qpu");

  ret.used_fields(true, true);
  //assert(ret.same(Precision));  // -1 < tanh < 1, this basically always succeeds
  //assert(ret.same_b(11));       // Very bad convergence, 11 often not enough
  return ret;
}


MMatrix MMatrix::sigmoid() const {
  need_fields(true, true);

  MMatrix ret;

  timers.start("MMatrix sigmoid Xf");
  ret.m_Xf = m_Xf.unaryExpr(&::sigmoid);
  timers.stop("MMatrix sigmoid Xf");

  timers.start("MMatrix sigmoid qpu");
  ret.m_qpu = m_qpu.sigmoid();
  timers.stop("MMatrix sigmoid qpu");

  ret.used_fields(true, true);
  assert(ret.same(4*Precision));  // TODO check precision when forward prop done
  return ret;
}


/**
 * Currently unused
 */
MMatrix MMatrix::outer(MMatrix const &rhs) const {
  assert(false); // Check unused
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

	timers.start("max_row qpu");

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

	timers.stop("max_row qpu");

  ret.used_fields(false, true);
  return ret;
}


/**
 * @brief Calculate sums per row
 *
 * TODO: convert to QPU, currently scalar
 */
MMatrix MMatrix::sum_row() const {
  need_fields(true, true);
  //warn << "sum_row: " << dump_dim();

	int height = rows();
	int width  = cols();

  MMatrix ret(height, 1);
  ret.need_fields(true, true);

  timers.start("sum_row Xf");
  for (int r = 0; r < height; r++) {
    ret.m_Xf(r, 0)  = m_Xf.row(r).sum();
  }
  timers.stop("sum_row Xf");
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

  //warn << "sum_row ret: " << ret.dump();
  return ret;
}


/**
 * =================================
 * Notes
 * -----
 *
 * - For sum reason, a direct sum() call returns a different value
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
  need_fields(true, true);
	assert(m_qpu.is_vector());

	MMatrix tmp = sum_row();
	//OK assert(tmp.same());
	return tmp.m_qpu.at(0,0);
}


/**
 * Calculate softmax per row
 */
void MMatrix::softmax() {
  need_fields(true, true);

	MMatrix max = max_row();

  for (int r = 0; r < rows(); r++) {
    auto tmp = row(r);
  	tmp.need_fields(true, true);

  	timers.start("softmax Xf");

  	// s_max is a global used in softmax()
  	s_max    = tmp.m_Xf.maxCoeff();
  	tmp.m_Xf = tmp.m_Xf.unaryExpr(&::softmax);
  	tmp.m_Xf.eval();

  	timers.stop("softmax Xf");
	  timers.start("softmax qpu");

    tmp.m_qpu.softmax(max.m_qpu.at(r, 0));  // TODO: scalar operation, fix

  	timers.stop("softmax qpu");

		//warn << "softmax tmp: " << tmp.dump();
    row(r, tmp);
  }

  used_fields(true, true);
	//warn << "softmax: " << dump();
	assert(same(2));  // Usually <= 1
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


void MMatrix::back_prop_1(MMatrix const &ds_cur, State const &temp, float precision) {
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
  //assert(same(precision)); 
}


void MMatrix::back_prop_3(MMatrix const &dsr, State const &temp, float precision) {
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
  //assert(same(precision));
}


/**
 * Xf/qpu slowly diverge upon sequential loops.
 */
void MMatrix::back_prop_4(MMatrix const &ds_cur_bk, State const &temp, float precision) {
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
  //timers.stop("back_prop_4 qpu");

  //Good enough assert(same(precision));
  used_fields(false, true);
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
  need_fields(true, true);

  MMatrix ret(rows(), cols());

  timers.start("ln Xf");
   ret.m_Xf  = m_Xf.unaryExpr(&log_matrix);
  timers.stop("ln Xf");
  timers.start("ln qpu");
  ret.m_qpu = m_qpu.ln();
  timers.stop("ln qpu");

  ret.used_fields(true, true);
  return ret;
}


MMatrix MMatrix::calc_E(MMatrix const &Y, MMatrix const &O) const {
  //warn << "calc_E O: " << O.dump();
  Y.need_fields(true, true);
  O.need_fields(true, true);
  need_fields(true, true);
  assert(m_qpu.is_vector());

  timers.start("calc_E");

  MMatrix temp_ln = O.ln();
  //warn << "temp_ln: " << temp_ln.dump();
  assert(temp_ln.same(5*Precision));

  MMatrix ret = -1.0f * Y.mul_e(temp_ln).sum_row();

  timers.stop("calc_E");

  assert(ret.same());
  ret.used_fields(true, true);
  return ret;
}


void MMatrix::col_E(int index, MMatrix const &rhs) {
  rhs.need_fields(true, true);
  assert(rhs.size() == 1);
  assert(rows() == 1);
  assert(0 <= index && index < cols());

  m_Xf(0, index) += rhs.m_Xf(0,0);

  used_fields(true, false);
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

	MMatrix ret;
/*
  timers.start("forward_1 Xf");
  ret.m_Xf = (Xf() * (m.U_z.Xf())) + (S.Xf() * (m.W_z.Xf()));
	ret.m_Xf.eval();
  timers.stop("forward_1 Xf");
*/

	// Timing approximately equal to Xf
  timers.start("forward_1 qpu");
  ret.m_qpu = (qpu() * (m.U_z.qpu())) + (S.qpu() * (m.W_z.qpu()));
  timers.stop("forward_1 qpu");

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

	MMatrix ret;
/*
  timers.start("forward_2 Xf");
  ret.m_Xf = (Xf() * (m.U_r.Xf())) + (S.Xf() * (m.W_r.Xf()));
	ret.m_Xf.eval();
  timers.stop("forward_2 Xf");
*/	

	// Timing approximately equal to Xf
  timers.start("forward_2 qpu");
  ret.m_qpu = (qpu() * (m.U_r.qpu())) + (S.qpu() * (m.W_r.qpu()));
  timers.stop("forward_2 qpu");

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
	S.need_fields(false, true);
	m.U_r.need_fields(false, true);
	m.W_r.need_fields(false, true);
	need_fields(false, true);

	MMatrix ret;
/*
  timers.start("forward_3 Xf");
 	ret.m_Xf = (Xf() * (m.U_h.Xf())) + (S.Xf().cwiseProduct(r_row.Xf())) * (m.W_h.Xf());
	ret.m_Xf.eval();
  timers.stop("forward_3 Xf");
*/
	// Timing slightly larger than Xf ~20%
  timers.start("forward_3 qpu");
 	ret.m_qpu = (qpu() * (m.U_h.qpu())) + (S.qpu().mul_e(r_row.qpu())) * (m.W_h.qpu());
  timers.stop("forward_3 qpu");

	ret.used_fields(false, true);
	//OK assert(ret.same_b(2));
	return ret;
}


/**
 * `this` corresponds to z_row.
 *
 * Derived from:
 *
 *     temp_hidden = (ones - z_row.Xf()).cwiseProduct(state.h.row(i).Xf() + z_row.Xf()).cwiseProduct(S_row.Xf());
 *
 * Timing (average, dim (1,128)):
 *     Xf          :  0.000005s
 *     qpu atomic  :  0.000182s
 *     kernel 1 QPU:  0.000046s
 *
 * Should become better with larger matrices. Kernel timing is probably largely call overhead.
 */
MMatrix MMatrix::forward_4(MMatrix &S, MMatrix const &h_row) {
	MatrixXf ones = MatrixXf::Ones(S.rows(), S.cols());
	MMatrix q_ones(S.rows(), S.cols(), 1.0f);

	S.need_fields(true, true);
	h_row.need_fields(true, true);
	need_fields(true, true);

	MMatrix ret(rows(), cols());

  timers.start("forward_4 Xf");
  ret.m_Xf = (ones - Xf()).cwiseProduct(h_row.Xf() + Xf()).cwiseProduct(S.Xf());
	ret.m_Xf.eval();
  timers.stop("forward_4 Xf");

  timers.start("forward_4 qpu");
  //ret.m_qpu = (q_ones.qpu() - qpu()).mul_e(h_row.qpu() + qpu()).mul_e(S.qpu());
	gru_kernel::forward_4(ret.m_qpu, m_qpu,  h_row.m_qpu, S.m_qpu);
  timers.stop("forward_4 qpu");

	assert(ret.same_b(2));  // Usually exact
	return ret;
}


/**
 * Again, differences with direct calculation: `diff: 2.793968e-08` (max detected)
 *
 * Unknown why, ignoring.
 */
MMatrix MMatrix::forward_5() const {
  timers.start("forward_5");
	MMatrix ret = *this;

  ret.softmax();
  float temp_sum = ret.sum();
  ret /= temp_sum;

  timers.stop("forward_5");

	return ret;
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
  if (need_XF && !m_using_Xf) {
    assert(m_using_qpu);
    //warn << "need_fields transferring qpu->Xf";

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
    m_qpu = copy_m(m_Xf);
    m_using_qpu = true;

    timers.stop("need_fields Xf->qpu");
  }
}


bool same(qpu::matrix const &lhs, MatrixXf const &rhs, float precision, int bit_diff, bool show_max_diff) {
  MMatrix rhs_temp;
  rhs_temp.set(rhs, true);

  return same(lhs, rhs_temp.qpu(), precision, bit_diff, show_max_diff);
}


bool same(qpu::matrix const &lhs, qpu::matrix const &rhs, float precision, int bit_diff,  bool show_max_diff) {
  //warn << "Called same(qpu::matrix, qpu::matrix)";
  bool ret = true;
  float max_diff = 0.0f;

  // Special case for 2 input vectors: accept transposed vectors
  if(lhs.columns() == 1 && lhs.columns() == rhs.rows() && lhs.rows() == rhs.columns() ) {
    int size = lhs.rows();
    for (int i = 0; i < size; ++i) {
      if (!qpu::check_precision(lhs.at(i, 0), rhs.at(0, i), precision, bit_diff, &max_diff)) {
        warn << "Fail same(vector, vector), (i,j): " << i << ", 0)";
        ret = false;
        if (!show_max_diff) break;
      }      
    }

    if (show_max_diff) {
      warn << "same(qpu::matrix, qpu::matrix) vector max_diff: " << max_diff;
    }
    return ret;
  }

  //
  // Do full matrices
  //

  if(lhs.rows() != rhs.rows() || lhs.columns() != rhs.columns() ) {
     warn << "Fail same(qpu::matrix, qpu::matrix) dimensions differ: "
          << "lhs: " << lhs.dump_dim() << ", "
          << "rhs: " << rhs.dump_dim();

     return false;
  }

  for (int i = 0; i < (int) rhs.rows(); ++i) {
    if (!show_max_diff && !ret) break;

    for (int j = 0; j < (int) rhs.columns(); ++j) {
      if (!qpu::check_precision(lhs.at(i, j), rhs.at(i, j), precision, bit_diff, &max_diff)) {
        if (ret) {  // Show first fail only
          warn << "Fail same(qpu::matrix, qpu::matrix), (i,j): " << i << ", " << j << ")";
        }

        ret = false;
        if (!show_max_diff) break;
      }      
    }
  }

  if (show_max_diff) {
    warn << "same(qpu::matrix, qpu::matrix) matrix max_diff: " << max_diff;
  }

  return ret;
}


qpu::matrix copy_m(MatrixXf const &rhs) {
  //assert(rhs.rows() == 1 || rhs.rows() % 16 == 0);  // Taking vectors into account
  //assert(rhs.cols() % 16 == 0);

	timers.start("copy_m");

  int height = (int) rhs.rows();
  int width  = (int) rhs.cols();

  qpu::matrix ret(height, width);

	if (height*width > 0) {
  	ret.set(0.0f);

	  for (int i = 0; i < height; i++) {
	    for (int j = 0; j < width; j++) {
	      ret.at(i, j) = rhs(i, j);
	    }
		}
	}

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
 
	buf	<< "[\n";

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

  //m_qpu.resize(rhs_rows, rhs_cols);
  //m_qpu.set(0.0f);

  timers.start("move_rows block");

  int to_offset = step*rhs_cols;
  int size      = rhs.size() - to_offset;

  // Barely better than per-row copy
  copy_block(rhs, 0, to_offset, size);

  timers.stop("move_rows block");

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
