#include "mmatrix.h"
#include "model.h"
#include "./kernel.h"
#include "helpers.h"  // resize_16()

MMatrix::MMatrix(int rows, int columns, float val) {
  assert(rows > 0);
  assert(columns > 0);
  assert(val == 0.0f || val == 1.0f);

  if (val == 0.0f) {
    m_Xf = MatrixXf::Zero(rows, columns);
  } else {
    m_Xf = MatrixXf::Ones(rows, columns);
  }

  m_qpu = copy_m(m_Xf);

  used_fields(true, true);
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
  rhs.need_fields(true, false);

  timers.start("set Xf");
  m_Xf = rhs.m_Xf;
  timers.stop("set Xf");

  timers.start("set qpu");
  // TODO Note that rhs.m_qpu is ignored here
  m_qpu = copy_m(rhs.m_Xf);
  timers.stop("set qpu");

  used_fields(true, true);
}


void MMatrix::reset() {
  m_Xf = MatrixXf::Zero(rows(), cols());
  m_qpu.set(0.0f);

  used_fields(true, true);
}


void MMatrix::sync_qpu() {
  need_fields(true, false);

  m_qpu = copy_m(m_Xf);

  used_fields(true, true);
}


void MMatrix::row(int index, MatrixXf const &val) {
  m_Xf.row(index) = val;
  m_Xf.eval();

  // TODO: Following inefficient and ugly
  m_qpu = copy_m(m_Xf);

  used_fields(true, true);
}


void MMatrix::row(int index, MMatrix const &val, bool sync_qpu) {
  val.need_fields(true, false);

  m_Xf.row(index) = val.m_Xf;

  if (sync_qpu) {
    // TODO: Following inefficient and ugly
    m_qpu = copy_m(m_Xf);
    used_fields(true, true);
  } else {
    used_fields(true, false);
  }
}


MMatrix MMatrix::row(int index) const {
  //warn << "row: " << dump();
  need_fields(true, true);

  MMatrix ret; //(1, cols());
  ret.m_Xf = m_Xf.row(index);
  ret.m_qpu = m_qpu.row(index);

  ret.used_fields(true, true);
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


bool MMatrix::same(MMatrix const &rhs, float precision, bool show_max_diff) const {
  need_fields(true, true);

  return
    ::same(m_qpu, m_Xf, precision) &&
    ::same(rhs.m_qpu, rhs.m_Xf, precision) &&
    ::same(m_qpu, rhs.m_Xf, precision, show_max_diff);
}


std::string MMatrix::dump_dim() const {
  need_fields(true, true);

  std::string ret;
  ret << ::dump_dim(m_Xf) << ", " << m_qpu.dump_dim();
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


void MMatrix::operator+=(MMatrix const &rhs) {
  rhs.need_fields(true, true);
  need_fields(true, true);

  timers.start("MMatrix += Xf");
  m_Xf = m_Xf + rhs.m_Xf;
  timers.stop("MMatrix += Xf");

  timers.start("MMatrix += qpu");
  m_qpu += rhs.m_qpu;
  timers.stop("MMatrix += qpu");

  used_fields(true, true);
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


/**
 * Name is a misnomer, the actual calculation is:
 *
 *     rhs * lhs^T;    // ^T - transposed
 */
MMatrix MMatrix::operator*(MMatrix const &rhs) const {
  rhs.need_fields(true, true);
  need_fields(true, true);

  MMatrix ret;

  timers.start("MMatrix * Xf");
  ret.m_Xf = rhs.m_Xf * m_Xf.transpose().eval();
  timers.stop("MMatrix * Xf");

  if (rhs.m_qpu.is_vector()) {
    timers.start("MMatrix * qpu vec");
    ret.m_qpu = m_qpu * rhs.m_qpu;
    timers.stop("MMatrix * qpu vec");
  } else {
    timers.start("MMatrix * qpu matrix");
    ret.m_qpu = rhs.m_qpu.mul_matrix_t(m_qpu);
    timers.stop("MMatrix * qpu matrix");
  }

  //OK assert(same());
  ret.used_fields(true, true);
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
  rhs.need_fields(true, true);
  need_fields(true, true);

  MMatrix ret;

  timers.start("MMatrix mul_e Xf");
  ret.m_Xf = m_Xf.cwiseProduct(rhs.m_Xf);
  timers.stop("MMatrix mul_e Xf");

  timers.start("MMatrix mul_e qpu");
  ret.m_qpu = m_qpu.mul_e(rhs.m_qpu);
  timers.stop("MMatrix mul_e qpu");

  ret.used_fields(true, true);
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
  lhs.need_fields(true, true);
  rhs.need_fields(true, true);
  need_fields(true, true);

  timers.start("MMatrix outer_add Xf");
  m_Xf  = m_Xf + lhs.m_Xf.transpose().eval() * rhs.m_Xf;
  timers.stop("MMatrix outer_add Xf");

  timers.start("MMatrix outer_add qpu");
  m_qpu.outer_add(lhs.m_qpu, rhs.m_qpu);
  timers.stop("MMatrix outer_add qpu");
  //assert(::same(tmp, m_Xf));

  //OK assert(same());
  used_fields(true, true);
}



/**
 * This is not a full outer product of tensors,
 * but a per-row calculation of outer products, which are all added to current instance.
 *
 * Note that the rows of rhs are technically transposed for the outer products.
 */
void MMatrix::outer_add_rows(MMatrix const &lhs, MMatrix const &rhs) {
  assert(lhs.rows() == rhs.rows());
  assert(lhs.rows() > 1);
  MMatrix tmp(lhs.cols(), rhs.cols());
	//warn << "tmp: " << tmp.dump_dim();

	timers.start("outer_add_rows");

  for (int i = 0; i < lhs.rows(); ++i) {
    tmp.outer_add(lhs.row(i), rhs.row(i));
  }

	timers.stop("outer_add_rows");

  *this += tmp;
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


void MMatrix::back_prop_1(MMatrix const &ds_cur, State const &temp) {
  ds_cur.need_fields(true, true);
  temp.h.need_fields(true, true);
  temp.z.need_fields(true, true);

  //MMatrix ones(resize_16(ds_cur.rows()), ds_cur.cols(), 1.0f);
  MMatrix ones(ds_cur.rows(), ds_cur.cols(), 1.0f);

  timers.start("back_prop_1 Xf");
  m_Xf = ds_cur.m_Xf.cwiseProduct(ones.m_Xf - temp.z.m_Xf).cwiseProduct(temp.h.Xf().unaryExpr(&tanh_grad));  //.cwiseProduct(temp_S.unaryExpr(&tanh_grad));
  timers.stop("back_prop_1 Xf");

  m_qpu.resize(ds_cur.rows(), ds_cur.cols());

  timers.start("back_prop_1 qpu");
  //Original: m_qpu = ds_cur.m_qpu.mul_e(ones.m_qpu - temp.q_z).mul_e(temp.q_h.dtanh());
  gru_kernel::back_prop_1(m_qpu, ds_cur.m_qpu, temp.z.m_qpu, temp.h.m_qpu);
  timers.stop("back_prop_1 qpu");

  used_fields(true, true);
}


void MMatrix::back_prop_2(State const &temp, MMatrix const &dreluInput_h, float precision) {
  timers.start("back_prop_2 Xf");
  m_Xf = temp.S.Xf().cwiseProduct(temp.r.Xf()).transpose().eval() * dreluInput_h.Xf();
  timers.stop("back_prop_2 Xf");

  timers.start("back_prop_2 qpu");
  m_qpu = temp.S.qpu().mul_e(temp.r.qpu()).outer(dreluInput_h.qpu());
  timers.stop("back_prop_2 qpu");

  used_fields(true, true);
  //OK assert(::same(m_qpu, m_Xf, precision)); 
}


void MMatrix::back_prop_3(MMatrix const &dsr, State const &temp, float precision) {
  //warn << "temp.S: "     << temp.S.dump_dim();

  dsr.need_fields(true, true);
  temp.S.need_fields(true, true);
  temp.r.need_fields(true, true);

  timers.start("back_prop_3 Xf");
  //m_Xf = dsr.m_Xf.cwiseProduct(temp.S.Xf()).cwiseProduct(temp.r.Xf().unaryExpr(&sigmoid_grad));
  MatrixXf tmp1 = dsr.m_Xf.cwiseProduct(temp.S.Xf());
  MatrixXf tmp2 = temp.r.Xf().unaryExpr(&sigmoid_grad);
	m_Xf = tmp1.cwiseProduct(tmp2);
  timers.stop("back_prop_3 Xf");

  m_qpu.resize(dsr.rows(), dsr.cols());

  timers.start("back_prop_3 qpu");
  gru_kernel::back_prop_3(m_qpu, dsr.m_qpu, temp.S.m_qpu, temp.r.m_qpu);
  timers.stop("back_prop_3 qpu");

  used_fields(true, true);
	assert(same(precision));
}


/**
 * Xf/qpu slowly diverge upon sequential loops.
 */
void MMatrix::back_prop_4(MMatrix const &ds_cur_bk, State const &temp, float precision) {
  timers.start("back_prop_4 Xf");
  auto dz = ds_cur_bk.Xf().cwiseProduct(temp.S.Xf() - temp.h.Xf());
  m_Xf = dz.cwiseProduct(temp.z.Xf().unaryExpr(&sigmoid_grad));
  timers.stop("back_prop_4 Xf");

  m_qpu.resize(ds_cur_bk.rows(), ds_cur_bk.cols());

  timers.start("back_prop_4 qpu");
  gru_kernel::back_prop_4(m_qpu, ds_cur_bk.qpu(), temp.z.qpu(), temp.S.qpu(), temp.h.qpu());
  timers.stop("back_prop_4 qpu");

  //warn << "m_Xf: " << dump(m_Xf);
  //warn << "m_qpu: " << m_qpu.dump();
  //Good enough assert(same(precision));
  used_fields(true, true);
}


void MMatrix::divide_matrix(MMatrix const &gradient, MMatrix const &in_cache) {
  timers.start("divide_matrix");

  auto const &grad  = gradient.Xf();
  auto const &cache = in_cache.Xf();

  for (int i = 0; i < m_Xf.rows(); ++i) {
    for (int j = 0; j < m_Xf.cols(); ++j) {
      m_Xf(i, j) = grad(i, j) / (float) (sqrt(cache(i, j)) + 0.00000001f);
    }
  }

  timers.stop("divide_matrix");

  used_fields(true, false);
}


void MMatrix::update_E(int index, MatrixXf const &Y, State const &state) {
  need_fields(true, false);

  auto temp_output = state.O.row(index).Xf();
  temp_output.eval();

  m_Xf(0, index) += -1 * Y.row(index).cwiseProduct(temp_output.unaryExpr(&log_matrix)).sum();
  m_Xf.eval();
}


void MMatrix::set_decay(float decay, MMatrix const &rhs) {
  rhs.need_fields(true, true);
  need_fields(true, true);

  //timers.start("set_decay Xf");
  m_Xf = decay * m_Xf + (1 - decay) * (rhs.m_Xf.cwiseProduct(rhs.m_Xf)).eval();
  //timers.stop("set_decay Xf");

  //timers.start("set_decay qpu");
  // 10X slower than Xf
  //m_qpu = copy_m(m_Xf);

  // 25x slower than Xf
  m_qpu = decay * m_qpu + (1 - decay) * rhs.m_qpu.mul_e(rhs.m_qpu);
  //timers.stop("set_decay qpu");

  //OK assert(::same(m_qpu, m_Xf));
  used_fields(true, true);
}


void MMatrix::need_fields(bool need_XF, bool need_qpu) const {
  if (need_XF && !m_using_Xf) {
    assert(m_using_qpu);
    warn << "need_fields transferring qpu->Xf";
    assert(false);  // TODO
  }

  if (need_qpu && !m_using_qpu) {
    assert(m_using_Xf);
    //warn << "need_fields transferring Xf->qpu";
    m_qpu = copy_m(m_Xf);
    m_using_qpu = true;
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


bool same(qpu::matrix const &lhs, qpu::matrix const &rhs, float precision) {
  //warn << "Called same(qpu::matrix, qpu::matrix)";

  // Special case for 2 input vectors: accept transposed vectors
  if(lhs.columns() == 1 && lhs.columns() == rhs.rows() && lhs.rows() == rhs.columns() ) {
    int size = lhs.rows();
    for (int i = 0; i < size; ++i) {
      if (!qpu::check_precision(lhs.at(i, 0), rhs.at(0, i), precision)) {
        warn << "Fail same(vector, vector), (i,j): " << i << ", 0)";
        return false;
      }      
    }

    return true;
  }

  // Do full matrices
  if(lhs.rows() != rhs.rows() || lhs.columns() != rhs.columns() ) {
     warn << "Fail same(qpu::matrix, qpu::matrix) dimensions differ: "
          << "lhs: " << lhs.dump_dim() << ", "
          << "rhs: " << rhs.dump_dim();

     return false;
  }

  for (int i = 0; i < (int) rhs.rows(); ++i) {
    for (int j = 0; j < (int) rhs.columns(); ++j) {
      if (!qpu::check_precision(lhs.at(i, j), rhs.at(i, j), precision)) {
        warn << "Fail same(qpu::matrix, qpu::matrix), (i,j): " << i << ", " << j << ")";
        return false;
      }      
    }
  }

  return true;
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


MMatrix move_rows(int step, MMatrix const &rhs) {
  if (step == 0) return rhs; // Nothing to do

  //warn << "Called move_rows(" << step << ")";
  assert(abs(step) < rhs.rows());

  MMatrix ret(rhs.rows(), rhs.cols());

  for (int i = 0; i < rhs.rows(); ++i) {
    if (i + step < 0) continue;
    if (i + step >= rhs.rows()) continue;

    ret.row(i + step, rhs.row(i), false);
  }

  ret.sync_qpu();
/*
  warn << "move_rows(" << step << "):\n"
       << "rhs: " << rhs.dump() << "\n"
       << "ret: " << ret.dump() << "\n";
*/
  return ret;
}
