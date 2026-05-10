#include "mmatrix.h"
#include "model.h"
#include "./kernel.h"

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
}


void MMatrix::set(MatrixXf const &rhs) {
  timers.start("set Xf");
  m_Xf = rhs;
  timers.stop("set Xf");

  timers.start("set qpu");
  m_qpu = copy_m(m_Xf);
  timers.stop("set qpu");
}


void MMatrix::sync_qpu() {
  m_qpu = copy_m(m_Xf);
}


bool MMatrix::same(MMatrix const &rhs, float precision) const {
  return
    ::same(m_qpu, m_Xf, precision) &&
    ::same(rhs.m_qpu, rhs.m_Xf, precision) &&
    ::same(m_qpu, rhs.m_Xf, precision);
}


std::string MMatrix::dump_dim() const {
  std::string ret;
  ret << ::dump_dim(m_Xf) << ", " << m_qpu.dump_dim();
  return ret;
}


void MMatrix::operator+=(MMatrix const &rhs) {
  timers.start("MMatrix += Xf");
  m_Xf = m_Xf + rhs.m_Xf;
  timers.stop("MMatrix += Xf");

  timers.start("MMatrix += qpu");
  m_qpu += rhs.m_qpu;
  timers.stop("MMatrix += qpu");
}


void MMatrix::operator-=(MMatrix const &rhs) {
  //timers.start("MMatrix -= Xf");
  m_Xf = m_Xf - rhs.m_Xf;
  //timers.stop("MMatrix -= Xf");

  //timers.start("MMatrix -= qpu");
  m_qpu -= rhs.m_qpu;
  //timers.stop("MMatrix -= qpu");
}


void MMatrix::operator/=(float steps) {
  m_Xf /= steps;
  m_qpu = copy_m(m_Xf);
}


MMatrix MMatrix::operator*(MMatrix const &rhs) const {
  MMatrix ret;

  timers.start("MMatrix * Xf");
  ret.m_Xf = rhs.m_Xf * m_Xf.transpose().eval();
  timers.stop("MMatrix * Xf");

  assert(rhs.m_qpu.is_vector());
  timers.start("MMatrix * qpu");
  ret.m_qpu = m_qpu * rhs.m_qpu;
  timers.stop("MMatrix * qpu");

  //OK assert(same());
  return ret;
}


MMatrix MMatrix::operator*(float val) const {
  MMatrix ret;

  //timers.start("MMatrix float * Xf");
  ret.m_Xf = val * m_Xf;
  //timers.stop("MMatrix float * Xf");

  //timers.start("MMatrix float * qpu");
  ret.m_qpu = m_qpu * val;
  //timers.stop("MMatrix float * qpu");

  return ret;
}


void MMatrix::mul_e(MMatrix const &rhs, State const &temp) {
  timers.start("MMatrix mul_e state Xf");
  m_Xf = rhs.m_Xf.cwiseProduct(temp.r.Xf());
  timers.stop("MMatrix mul_e state Xf");

  timers.start("MMatrix mul_e state qpu");
  m_qpu = rhs.m_qpu.mul_e(temp.r.qpu());
  timers.stop("MMatrix mul_e state qpu");
}


MMatrix MMatrix::mul_e(MMatrix const &rhs) {
  MMatrix ret;

  timers.start("MMatrix mul_e Xf");
  ret.m_Xf = m_Xf.cwiseProduct(rhs.m_Xf);
  timers.stop("MMatrix mul_e Xf");

  timers.start("MMatrix mul_e qpu");
  ret.m_qpu = m_qpu.mul_e(rhs.m_qpu);
  timers.stop("MMatrix mul_e qpu");

  return ret;
}


MMatrix MMatrix::outer(MMatrix const &rhs) const {
  MMatrix ret;
  timers.start("MMatrix outer Xf");
  ret.m_Xf  = m_Xf.transpose().eval() * rhs.m_Xf;
  timers.stop("MMatrix outer Xf");

  timers.start("MMatrix outer qpu");
  ret.m_qpu = m_qpu.outer(rhs.m_qpu);
  timers.stop("MMatrix outer qpu");

  //OK assert(same());
  return ret;
}


void MMatrix::outer_add(MMatrix const &lhs, MMatrix const &rhs) {
  timers.start("MMatrix outer_add Xf");
  m_Xf  = m_Xf + lhs.m_Xf.transpose().eval() * rhs.m_Xf;
  timers.stop("MMatrix outer_add Xf");

  timers.start("MMatrix outer_add qpu");
  m_qpu.outer_add(lhs.m_qpu, rhs.m_qpu);
  timers.stop("MMatrix outer_add qpu");
  //assert(::same(tmp, m_Xf));

  //OK assert(same());
}


void MMatrix::back_prop_1(MMatrix const &ds_cur, State const &temp) {
  MMatrix ones(1, ds_cur.cols(), 1.0f);

  timers.start("back_prop_1 Xf");
  m_Xf = ds_cur.m_Xf.cwiseProduct(ones.m_Xf - temp.z.m_Xf).cwiseProduct(temp.h.Xf().unaryExpr(&tanh_grad));  //.cwiseProduct(temp_S.unaryExpr(&tanh_grad));
  timers.stop("back_prop_1 Xf");

  m_qpu.resize(ds_cur.cols(), 1);

  timers.start("back_prop_1 qpu");
  //Original: m_qpu = ds_cur.m_qpu.mul_e(ones.m_qpu - temp.q_z).mul_e(temp.q_h.dtanh());
  gru_kernel::back_prop_1(m_qpu, ds_cur.m_qpu, temp.z.m_qpu, temp.h.qpu());
  timers.stop("back_prop_1 qpu");
}


void MMatrix::back_prop_2(State const &temp, MMatrix const &dreluInput_h, float precision) {
  timers.start("back_prop_2 Xf");
  m_Xf = temp.S.Xf().cwiseProduct(temp.r.Xf()).transpose().eval() * dreluInput_h.Xf();
  timers.stop("back_prop_2 Xf");

  timers.start("back_prop_2 qpu");
  m_qpu = temp.S.qpu().mul_e(temp.r.qpu()).outer(dreluInput_h.qpu());
  timers.stop("back_prop_2 qpu");

  //OK assert(::same(m_qpu, m_Xf, precision)); 
}


void MMatrix::back_prop_3(MMatrix const &dsr, State const &temp, float precision) {
  timers.start("back_prop_3 Xf");
  m_Xf = dsr.m_Xf.cwiseProduct(temp.S.Xf()).cwiseProduct(temp.r.Xf().unaryExpr(&sigmoid_grad));
  timers.stop("back_prop_3 Xf");

  m_qpu.resize(dsr.cols(), dsr.rows()); // sic; dimensions reversed

  timers.start("back_prop_3 qpu");
  gru_kernel::back_prop_3(m_qpu, dsr.m_qpu, temp.S.m_qpu, temp.r.m_qpu);
  timers.stop("back_prop_3 qpu");
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
}


void MMatrix::update_E(int index, MatrixXf const &Y, State const &state) {
  auto temp_output = state.O.row(index);
  temp_output.eval();

  m_Xf(0, index) += -1 * (Y.row(index).cwiseProduct(temp_output.unaryExpr(&log_matrix)).sum());
  m_Xf.eval();
}


void MMatrix::set_decay(float decay, MMatrix const &rhs) {
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
}


bool same(qpu::matrix const &lhs, MatrixXf const &rhs, float precision) {
  //warn << "Called same(qpu::matrix, MatrixXf)";

  // Special case for 2 input vectors: accept transposed vectors
  if(lhs.columns() == 1 && lhs.columns() == rhs.rows() && lhs.rows() == rhs.cols() ) {
    int size = lhs.rows();
    for (int i = 0; i < size; ++i) {
      if (!qpu::check_precision(lhs.at(i, 0), rhs(0, i), precision)) {
        warn << "Fail same(vector, vector), (i,j): " << i << ", 0)";
        return false;
      }      
    }

    return true;
  }

  // Do full matrices
  if(lhs.rows() != rhs.rows() || lhs.columns() != rhs.cols() ) {
     warn << "Fail same(qpu::matrix, MatrixXf) dimensions differ: "
          << "lhs: " << lhs.dump_dim() << ", "
          << "rhs: " << dump_dim(rhs);

     return false;
  }

  for (int i = 0; i < (int) rhs.rows(); ++i) {
    for (int j = 0; j < (int) rhs.cols(); ++j) {
      if (!qpu::check_precision(lhs.at(i, j), rhs(i, j), precision)) {
        warn << "Fail same(qpu::matrix, MatrixXf), (i,j): " << i << ", " << j << ")";
        return false;
      }      
    }
  }

  return true;
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
