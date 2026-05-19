#include "mmatrix.h"
#include "model.h"
#include "./kernel.h"
#include "helpers.h"  // resize_16()

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

  //m_qpu = copy_m(m_Xf);
}


MMatrix::MMatrix(MMatrix const &rhs) {
  set(rhs);
}


void MMatrix::set(MatrixXf const &rhs) {
  //timers.start("set Xf");
  m_Xf = rhs;
  //timers.stop("set Xf");

/*
  timers.start("set qpu");
  m_qpu = copy_m(m_Xf);
  timers.stop("set qpu");
*/
  used_fields(true, false);
}


void MMatrix::set(MMatrix const &rhs) {
  rhs.need_fields(false, true);

  m_qpu = rhs.m_qpu;

  used_fields(false, true);
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
  assert(cols() == val.cols());
  assert(index >=0 && index < rows());

  val.need_fields(false, true);

  for (int i = 0; i < val.cols(); ++i) {
    m_qpu.arr()[index*cols() + i] = val.m_qpu.arr()[i];
  }

  used_fields(false, true);
}


MMatrix MMatrix::row(int index) const {
  need_fields(false, true);

  timers.start("row(index)");

  MMatrix ret;
  ret.m_qpu = m_qpu.row(index);

  timers.stop("row(index)");

  ret.used_fields(false, true);
  // assert(ret.same()); // Fails on diff 4.814825e-35; gimme a break
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


/**
 * Profiling: time negligible
 */
bool MMatrix::same(MMatrix const &rhs, float precision, bool show_max_diff) const {
  assert(m_using_Xf || m_using_qpu);
  assert(rhs.m_using_Xf || rhs.m_using_qpu);
  assert((m_using_Xf == rhs.m_using_Xf) || (m_using_qpu == rhs.m_using_qpu));

  if (m_using_Xf && m_using_qpu) {
    return
      ::same(m_qpu, m_Xf, precision) &&
      ::same(rhs.m_qpu, rhs.m_Xf, precision) &&
      ::same(m_qpu, rhs.m_Xf, precision, show_max_diff);
  }

  if (m_using_qpu) {
    return ::same(m_qpu, rhs.m_qpu, precision, show_max_diff);
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
  need_fields(true, true);

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

  assert(ret.same());
  ret.used_fields(true, true);
  return ret;
}


void MMatrix::operator+=(MMatrix const &rhs) {
  rhs.need_fields(true, false);
  need_fields(true, false);
/*
  timers.start("MMatrix += Xf");
  m_Xf = m_Xf + rhs.m_Xf;
  timers.stop("MMatrix += Xf");
*/
  timers.start("MMatrix += qpu");
  m_qpu += rhs.m_qpu;
  timers.stop("MMatrix += qpu");

  used_fields(true, false);
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
  need_fields(true, true);

  m_Xf /= steps;
  m_qpu = copy_m(m_Xf);
}


MMatrix MMatrix::operator*(MMatrix const &rhs) const {
  rhs.need_fields(true, true);
  need_fields(true, true);

  MMatrix ret;

  timers.start("MMatrix * Xf");
  ret.m_Xf = m_Xf * rhs.m_Xf;
  timers.stop("MMatrix * Xf");

  timers.start("MMatrix * qpu");
  ret.m_qpu = m_qpu.mul_matrix(rhs.m_qpu);
  timers.stop("MMatrix * qpu");

  ret.used_fields(true, true);
	assert(ret.same());
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
    timers.start("MMatrix mul_t qpu matrix");
    ret.m_qpu = rhs.m_qpu.mul_matrix_t(m_qpu);
    timers.stop("MMatrix mul_t qpu matrix");
  }

  //assert(ret.same());
  ret.used_fields(false, true);
  return ret;
}


MMatrix MMatrix::operator*(float val) const {
  need_fields(true, true);

  MMatrix ret;

  //timers.start("MMatrix float * Xf");
  ret.m_Xf = val * m_Xf;
  //timers.stop("MMatrix float * Xf");

  //timers.start("MMatrix float * qpu");
  ret.m_qpu = m_qpu * val;
  //timers.stop("MMatrix float * qpu");

  ret.used_fields(true, true);
  return ret;
}


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
void MMatrix::outer_add_rows(MMatrix const &lhs, MMatrix const &rhs, float precision) {
  int lhs_rows = lhs.rows();

  assert(lhs_rows == rhs.rows());
  assert(lhs_rows > 1);

  //timers.start("outer_add_rows");
  m_qpu.outer_add_rows(lhs.m_qpu, rhs.m_qpu);
  //timers.stop("outer_add_rows");
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


void MMatrix::update_E(int index, MatrixXf const &Y, State const &state) {
  need_fields(true, false);

  timers.start("update_E Xf");

  auto temp_output = state.O.row(index).Xf();
  temp_output.eval();

  m_Xf(0, index) += -1 * Y.row(index).cwiseProduct(temp_output.unaryExpr(&log_matrix)).sum();
  m_Xf.eval();

  timers.stop("update_E Xf");

  used_fields(true, false);
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
    //warn << "need_fields transferring Xf->qpu";

    timers.start("need_fields Xf->qpu");
    m_qpu = copy_m(m_Xf);
    m_using_qpu = true;

    timers.stop("need_fields Xf->qpu");
  }
}


bool same(qpu::matrix const &lhs, MatrixXf const &rhs, float precision, bool show_max_diff) {
  //warn << "Called same(qpu::matrix, MatrixXf)";
  bool ret = true;
  float max_diff = 0.0f;

  // Special case for 2 input vectors: accept transposed vectors
  if (lhs.columns() == 1 && lhs.columns() == rhs.rows() && lhs.rows() == rhs.cols() ) {
    int size = lhs.rows();
    for (int i = 0; i < size; ++i) {
      if (!qpu::check_precision(lhs.at(i, 0), rhs(0, i), precision, &max_diff, ret)) {
        if (ret) {  // Show first fail only
          warn << "Fail same(vector, vector), (i,j): " << i << ", 0)";
        }
        ret = false;
        if (!show_max_diff) break;
      }      
    }

    if (show_max_diff) {
      warn << "same(qpu::matrix, MatrixXf) vector max_diff: " << max_diff;
    }
    return ret;
  }

  //
  // Do full matrices
  //

  if(lhs.rows() != rhs.rows() || lhs.columns() != rhs.cols() ) {
     warn << "Fail same(qpu::matrix, MatrixXf) dimensions differ: "
          << "lhs: " << lhs.dump_dim() << ", "
          << "rhs: " << dump_dim(rhs);

     return false;
  }

  for (int i = 0; i < (int) rhs.rows(); ++i) {
    if (!show_max_diff && !ret) break;

    for (int j = 0; j < (int) rhs.cols(); ++j) {
      if (!qpu::check_precision(lhs.at(i, j), rhs(i, j), precision, &max_diff, ret)) {
        if (ret) {  // Show first fail only
          warn << "Fail same(qpu::matrix, MatrixXf), (i,j): " << i << ", " << j << ")";
        }

        ret = false;
        if (!show_max_diff) break;
      }
    }
  }

  if (show_max_diff) {
    warn << "same(qpu::matrix, MatrixXf) matrix max_diff: " << max_diff;
  }

  return ret;
}


bool same(qpu::matrix const &lhs, qpu::matrix const &rhs, float precision, bool show_max_diff) {
  //warn << "Called same(qpu::matrix, qpu::matrix)";
  bool ret = true;
  float max_diff = 0.0f;

  // Special case for 2 input vectors: accept transposed vectors
  if(lhs.columns() == 1 && lhs.columns() == rhs.rows() && lhs.rows() == rhs.columns() ) {
    int size = lhs.rows();
    for (int i = 0; i < size; ++i) {
      if (!qpu::check_precision(lhs.at(i, 0), rhs.at(0, i), precision)) {
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
      if (!qpu::check_precision(lhs.at(i, j), rhs.at(i, j), precision)) {
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

  int height = (int) rhs.rows();
  int width = (int) rhs.cols();

  qpu::matrix ret(height, width);
  ret.set(0.0f);

  for (int i = 0; i < rhs.rows(); i++) {
    for (int j = 0; j < rhs.cols(); j++) {
      ret.at(i, j) = rhs(i, j);
    }
  }

  return ret;
}


std::string dump(MatrixXf const &m) {
  std::string buf;
  buf << dump_dim(m) << " [\n";

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

  m_qpu.resize(rhs_rows, rhs_cols);

  timers.start("move_rows block");

	int to_offset = step*rhs_cols;
	int size      = rhs.size() - to_offset;

	// Barely better than per-row copy
	copy_block(rhs, 0, to_offset, size);

  timers.stop("move_rows block");

  used_fields(false, true);
}
