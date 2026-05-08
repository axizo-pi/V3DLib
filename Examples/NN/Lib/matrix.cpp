#include "matrix.h"
#include "scalar.h"
#include "helpers.h"  // frrand()

namespace qpu {
namespace {
  
void frand_array(Float::Array &rhs) {
  for (int i = 0; i < (int) rhs.size(); ++i) {
    rhs[i] = frand();
  }
}

bool done_init = false;
std::unique_ptr<BaseKernel> s_mul_element;
std::unique_ptr<BaseKernel> s_mult_vec_transposed;
std::unique_ptr<BaseKernel> s_mult_vec;
std::unique_ptr<BaseKernel> s_matrix_add;
std::unique_ptr<BaseKernel> s_matrix_add_self;
std::unique_ptr<BaseKernel> s_sub;
std::unique_ptr<BaseKernel> s_add;
std::unique_ptr<BaseKernel> s_op;
std::unique_ptr<BaseKernel> s_sigmoid;
std::unique_ptr<BaseKernel> s_dsigmoid;
std::unique_ptr<BaseKernel> s_tanh;
std::unique_ptr<BaseKernel> s_dtanh;
std::unique_ptr<BaseKernel> s_clip;


void init_local() {
	if (done_init) return;

  s_mul_element        .reset(new BaseKernel(compile(kernel::mul_element    , settings())));
  s_mult_vec_transposed.reset(new BaseKernel(compile(kernel::mult_vec_transposed, settings())));
  s_mult_vec           .reset(new BaseKernel(compile(kernel::mult_vec       , settings())));
  s_matrix_add         .reset(new BaseKernel(compile(kernel::matrix_add     , settings())));
  s_matrix_add_self    .reset(new BaseKernel(compile(kernel::matrix_add_self, settings())));
  s_sub                .reset(new BaseKernel(compile(kernel::vector_sub     , settings())));
  s_add                .reset(new BaseKernel(compile(kernel::vector_add     , settings())));
  s_op                 .reset(new BaseKernel(compile(kernel::outer_product  , settings())));
  s_sigmoid            .reset(new BaseKernel(compile(kernel::sigmoid        , settings())));
  s_dsigmoid           .reset(new BaseKernel(compile(kernel::dsigmoid       , settings())));
  s_tanh               .reset(new BaseKernel(compile(kernel::tanh           , settings())));
  s_dtanh              .reset(new BaseKernel(compile(kernel::dtanh          , settings())));
  s_clip               .reset(new BaseKernel(compile(kernel::clip           , settings())));

	done_init = true;
}

} // anon namespace


////////////////////////////////////////////////
// Class matrix
////////////////////////////////////////////////

matrix::matrix(int rows, int columns) {
	resize(rows, columns);
  init_local();
}


matrix::matrix(matrix const &rhs) :
	m_arr(rhs.m_arr),
	m_rows(rhs.m_rows),
	m_columns(rhs.m_columns),
	m_size(rhs.m_size)
{
  //warn << "Called ctor matrix(matrix &)";
  //// *this = rhs;
}


void matrix::resize(int rows, int columns) {
  // Allow initialization of empty matrix.
  if (rows == 0) {
    assert(columns == 0 || columns == 1);  // By syntax, a single empty vector is allowed
		m_rows    = rows;
		m_columns = columns;
    return;
  }

  if (rows <= 0)     { cerr << "matrix ctor: rows must be positive"    << thrw; }
  if (columns <= 0)  { cerr << "matrix ctor: columns must be positive" << thrw; }

	int size      = rows*columns;

	if (size > m_size) {
		if (m_size > 0) {
			warn << "matrix resizing from " << m_size << " to " << size;
		}
    m_arr.reset(new Float::Array(size));
		m_size    = size;
	}

	m_rows    = rows;
	m_columns = columns;
}


Float::Array &matrix::operator &() {
  assert(m_arr != nullptr);
  return (*m_arr);
}


Float::Array &matrix::arr() {
  assert(m_arr != nullptr);
  return *m_arr;
}


Float::Array const &matrix::arr() const {
  assert(m_arr != nullptr);
  return *m_arr;
}


void matrix::rand() {
  frand_array(arr());
}


void matrix::set(float init_val) {
  auto &r = arr();

  for (int i = 0; i < m_rows*m_columns; ++i) {
    r[i] = init_val;
  }
}


void matrix::set(Float::Array const &rhs) {
  assert(arr().size() == rhs.size());

  for (int i = 0; i < (int) arr().size(); ++i) {
    arr()[i] = rhs[i];
  }
}


void matrix::frand() {
  assert(arr().size() > 0);

   for (int i = 0; i < (int) arr().size(); ++i) {
    arr()[i] = frrand();
    //arr()[i] = ::frand();
   }
}


matrix &matrix::operator=(matrix const &rhs) {
  transfer(rhs);
  return *this;
}


matrix matrix::operator-(matrix const &rhs) const {
  assert(
	 // Allow transposed vectors
	 (m_rows == 1 && rhs.columns() == 1 && m_rows == rhs.columns()) ||
	 (m_columns == rhs.columns() && m_rows == rhs.rows())
	);
  matrix ret(m_rows, m_columns);

  for (int i = 0; i < (int) m_arr->size(); ++i) {
    ret.arr()[i] = arr()[i] - rhs.arr()[i];
  }

  return ret;
}


matrix &matrix::operator-=(matrix const &rhs) {
  assert(m_columns == rhs.columns() && m_rows == rhs.rows());

  for (int i = 0; i < (int) m_arr->size(); ++i) {
    arr()[i] -= rhs.arr()[i];
  }

  return *this;
}


matrix matrix::operator+(matrix const &rhs) const {
  if (!(m_columns == rhs.columns() && m_rows == rhs.rows())) {
		warn << "matrix operator+ dimensions don't match: "
			   << "this: " << dump_dim() << ", "
			   << "rhs: " << rhs.dump_dim();
	}
  assert(m_columns == rhs.columns() && m_rows == rhs.rows());
  matrix ret(m_rows, m_columns);

  s_matrix_add->load(&ret.arr(), &arr(), &rhs.arr(), m_rows*m_columns/16).run();
/*
  for (int i = 0; i < (int) m_arr->size(); ++i) {
    ret.arr()[i] = arr()[i] + rhs.arr()[i];
  }
*/
  return ret;
}


matrix &matrix::operator+=(matrix const &rhs) {
	if (is_vector() && rhs.is_vector()) {
		// Allow transposed vectors
		assert(size() == rhs.size());
	} else {
  	if (m_columns != rhs.columns() || m_rows != rhs.rows()) {
			warn << "matrix::operator+= dimensions don't match: "
				   << "this: " << dump_dim() << ", "
					 << "rhs:"   << rhs.dump_dim()
					 << thrw;
		}
  	assert(m_columns == rhs.columns() && m_rows == rhs.rows());
	}

  s_matrix_add_self->load(&arr(), &rhs.arr(), size()/16).run();

/*	
  for (int i = 0; i < (int) m_arr->size(); ++i) {
    arr()[i] += rhs.arr()[i];
  }
*/	

  return *this;
}


matrix matrix::operator*(float rhs) const {
  matrix ret(m_rows, m_columns);

  for (int i = 0; i < (int) m_arr->size(); ++i) {
    ret.arr()[i] = arr()[i]*rhs;
  }

  return ret;
}


/**
 * @brief Perform multiplication between a matrix and a vector
 *
 * Currently rhs must absolutely be a vector.
 * I fully realize the parameter is confusing.
 */
matrix matrix::mul(matrix const &rhs) const {
  //warn << "Called matrix matrix::operator*()";
  //warn << "matrix: " << dump_dim() << "rhs: " << rhs.dump_dim();
	assert(rhs.is_vector());
  assert(m_columns > 0);
  assert(m_rows > 0);

  if (m_columns != rhs.size()) {
    cerr << "Matrix::mul() Inner dimension does not match. "
         << "this(rows, columns): (" << m_rows << ", " << m_columns << "), "
         << "rhs(rows, columns):  (" << rhs.rows() << ", " << rhs.columns() << ")"
          << thrw;
  }

  assert((m_columns % 16) == 0);      // Inner dimension must be multiple of 16

  matrix ret(m_rows, 1);

  s_mult_vec->load(&rhs.arr(), &arr(), &ret.arr(), m_columns/16, m_rows).run();
  return ret;
}


/**
 * @brief Perform multiplication between a matrix and a vector,
 *        where the matrix is transposed in the calculation
 */
matrix matrix::mul_t(matrix const &rhs) const {
  //warn << "Called matrix matrix::mul_t()";
  //warn << "matrix: " << dump_dim() << "rhs: " << rhs.dump_dim();
  assert(m_columns > 0);
  assert(m_rows > 0);

  if (m_rows != rhs.rows()) {
    cerr << "Matrix::mul_t() Inner dimension does not match. "
         << "this(rows, columns): (" << m_rows << ", " << m_columns << "), "
         << "rhs(rows, columns):  (" << rhs.rows() << ", " << rhs.columns() << ")"
          << thrw;
  }

  assert((m_rows % 16) == 0);

  matrix ret(m_columns, 1);

  s_mult_vec_transposed->load(&rhs.arr(), &arr(), &ret.arr(), m_columns, m_rows/16).run();

  return ret;
}


/**
 * @brief By-element product of the two matrices.
 */
matrix matrix::mul_e(matrix const &rhs) const {
  //warn << "Called matrix matrix::mul_e()";
  //warn << "matrix: " << dump_dim() << "rhs: " << rhs.dump_dim();
  assert(m_columns > 0);
  assert(m_rows > 0);

	// Keep the calc flexible, just check size
	int size = m_rows*m_columns;
	int in_size = rhs.rows()*rhs.columns();

  if (size != in_size) {
    cerr << "Matrix::mul_e() Dimensions don't match. "
         << "this(rows, columns): (" << m_rows << ", " << m_columns << "), "
         << "rhs(rows, columns):  (" << rhs.rows() << ", " << rhs.columns() << ")"
          << thrw;
  }

  assert((size % 16) == 0);      // Total dimension must be multiple of 16

  matrix ret(m_rows, m_columns);

	assert(s_mul_element != nullptr);
  s_mul_element->load(&ret.arr(), &arr(), &rhs.arr(), size/16).run();

  return ret;
}


/**
 * NOTE: this is currently a scalar operation
 */
matrix matrix::sigmoid_derivative(matrix const &rhs) {
  assert(m_columns == rhs.columns() && m_rows == rhs.rows());

  matrix ret(m_rows, m_columns);

  for (int i = 0; i < (int) m_arr->size(); ++i) {
    float el = arr()[i];
    ret.arr()[i] = el*(1 - el);
  }

  for (int i = 0; i < (int) ret.arr().size(); ++i) {
    float a = ret.arr()[i];
    float b = rhs.arr()[i];

    ret.arr()[i] = a*b;
  }

  return ret;
}


matrix matrix::transpose() const {
  auto &t_3 = V3DLib::timers.start("matrix transpose");
  matrix ret(m_columns, m_rows);

  for (int h = 0; h < m_rows; ++h) {
    for (int w = 0; w < m_columns; ++w) {
      ret.arr()[w*m_rows + h] = arr()[h*m_columns + w];
    }
  }

  t_3.stop();
  return ret;
}


namespace {

matrix outer_ret(true);	

} // anon namespace



/**
 * @brief Calculate outer product of two vectors.
 *
 * This and rhs are both defined as matrix, because this fits the programming logic.
 * Vectors are special cases of matrices anyway.
 *
 * ==================================
 * Notes
 * -----
 *
 * - Unfortunately, defining the return value as a static does not have much effect
 *   on the performance.
 *
 * - Due to use of a static return value, this method is **not thread-safe**.
 *   This is not really important, because QPU kernel call should only be called
 *   from a single thread.
 */
matrix matrix::outer(matrix const &rhs) const {
	assert(is_vector());
	assert(rhs.is_vector());
  if ((size() & 0xf) != 0)     { cerr << "vector outer: expecting rows to be a multiple of 16" << thrw; }
  if ((rhs.size() & 0xf) != 0) { cerr << "vector outer: expecting rhs rows to be a multiple of 16" << thrw; }

  outer_ret.resize(size(), rhs.size());

  s_op->load(&arr(), &rhs.arr(), &outer_ret.arr(), size(), rhs.size()/16).run();

	//warn << "outer resize: " << outer_ret.dump_dim();
  return outer_ret;  
}


std::string matrix::dump_dim() const {
  std::string ret;
  ret << "(" << m_rows << ", " << m_columns << ") ";
  return ret;
}


std::string matrix::dump(bool output_int) const {
  assert(m_arr != nullptr);
  std::string ret;

  ret << dump_dim();

  if (m_columns == 1) {
    ret << "(tr) ";  // Signal transposed
    ret << "[" << vector_dump(*m_arr, m_rows, 0, output_int) << "]";
  } else {
    ret << "\n";

    for (int h = 0; h < m_rows; ++h) {
       ret << h << ": [" << vector_dump(*m_arr, m_columns, h*m_columns, output_int) << "]\n";
    }
  }

  return ret;
}


void matrix::transfer(matrix const &rhs) {
  assert(rhs.m_arr != nullptr);

  m_columns  = rhs.m_columns;
  m_rows = rhs.m_rows;

  //m_arr.reset(rhs.m_arr.release());
  m_arr = rhs.m_arr;
  //assert(rhs.m_arr == nullptr);
}


matrix operator*(float scalar, matrix const &mat) {
  auto ret = mat * scalar;
  return ret;
}


////////////////////////////////////////////////
// Class vector 
////////////////////////////////////////////////

vector::vector(vector &rhs) : matrix(rhs) {
  *this = rhs;
  init_local();
}


void vector::transpose() {
  //warn << "vector transposing";
  int tmp   = m_rows;
	m_rows    = m_columns;
  m_columns = tmp;
}


vector::vector(matrix rhs) : matrix(rhs) {
  //warn << "Called ctor vector(matrix) m_arr:" << hex << (unsigned long) rhs.arr().ptr();
  //warn << "vector m_arr:" << hex << (unsigned long) arr().ptr();
  assert(
    (rhs.rows() > 1  && rhs.columns() == 1) ||
    (rhs.rows() == 1 && rhs.columns() > 1)
  );

  *this = rhs;

  if (rhs.rows() == 1) {
		transpose();
  }

  init_local();
}


vector::vector(int rows, float val) : matrix(rows, 1) {
  if ((rows & 0xf) != 0) {
    cerr << "vector ctor: " << rows << " rows passed in,  must be a multiple of 16" << thrw;
  }

  if (!empty()) {
    auto &r = matrix::arr();

    for (int i = 0; i < rows; i++) {
      r[i] = val;
    }
  }

  init_local();
}


void vector::set(float *rhs, int in_size) {
  assert(rows() >= in_size);

  auto &r = matrix::arr();

  for (int i = 0; i < in_size; ++i) {
    r[i] = rhs[i];
  }

  for (int i = in_size; i < rows(); ++i) {
    r[i] = 0;
  }
}


void vector::set(float init_val) {
  auto &r = arr();

  for (int i = 0; i < size(); ++i) {
    r[i] = init_val;
  }
}


float &vector::operator[](int index) {
  assert(rows() > index);

  auto &r = arr();
  return r[index];
}


float vector::operator[](int index) const {
  assert(rows() > index);

  auto &r = arr();
  return r[index];
}


int vector::size() const {
  assert(columns() == 1);
  return rows();
}


vector vector::operator-(vector const &rhs) {
  assert(columns() == 1);
  assert(columns() == rhs.columns());
  assert(rows() == rhs.rows());
  if ((rows() & 0xf) != 0) { cerr << "vector sub: rows must be a multiple of 16" << thrw; }

  vector ret(rows());

  s_sub->load(&arr(), &rhs.arr(), &ret.arr(), rows()/16).run();
  return ret;
}


vector vector::operator+(vector const &rhs) {
  assert(columns() == 1);
  assert(columns() == rhs.columns());
  assert(rows() == rhs.rows());
  if ((rows() & 0xf) != 0) { cerr << "vector sub: rows must be a multiple of 16" << thrw; }

  vector ret(rows());

  s_add->load(&arr(), &rhs.arr(), &ret.arr(), rows()/16).run();
  return ret;
}


/**
 * @brief By-element product of the two vectors.
 */
vector vector::mul(vector const &rhs) {
  assert(columns() == 1);
  assert(columns() == rhs.columns());
  assert(rows() == rhs.rows());
  if ((rows() & 0xf) != 0) { cerr << "vector sub: rows must be a multiple of 16" << thrw; }

	matrix ret = mul_e(rhs);
  return vector(ret);
/*
	vector ret(rows());
  for (int i = 0; i < rows(); i++) {
    ret[i] = (*this)[i]*rhs[i];
  }
	return ret;
*/
}


vector &vector::operator=(matrix const &rhs) {
  assert(rhs.is_vector());
  transfer(rhs);
  return *this;
}


vector &vector::operator=(vector const &rhs) {
  assert(rhs.columns() == 1);
  transfer(rhs);
  return *this;
}



vector vector::sigmoid(vector const &bias) {
  vector ret(rows());
  s_sigmoid->load(&arr(), &bias.arr(), &ret.arr(), rows()/16).run();
  return ret;  
}


vector vector::dsigmoid() const {
  vector ret(rows());
  s_dsigmoid->load(&arr(), &ret.arr(), rows()/16).run();
  return ret;  
}


vector vector::tanh() {
  vector ret(rows());

  s_tanh->load(&arr(), &ret.arr(), rows()/16).run();
  return ret;  
}


vector vector::dtanh() const {
  vector ret(rows());

  s_dtanh->load(&arr(), &ret.arr(), rows()/16).run();
  return ret;  
}


/**
 * Value changed internally, no return value.
 */
void vector::clip(float clip_value) {
  vector ret(rows());

  s_clip->load(&arr(), &ret.arr(), rows()/16, clip_value).run();
  *this = ret;
}


std::string vector::dump(bool output_int) const {
  std::string ret;
  ret << dump_dim()
      << "[" << vector_dump(arr(), rows(), 0, output_int) << "]";

  return ret;
}


BaseKernel &vector::op_kernel() {
  init_local();
  return *s_op;
}


vector operator*(matrix const &lhs, matrix const &rhs) {
	//warn << "rhs: " << rhs.dump_dim();
  assert(rhs.is_vector());

  matrix ret;
  ret = lhs.mul(rhs);
  return vector(ret);
}


bool check_precision(float lhs, float rhs, float precision) {
  bool ret = true;

  if (precision == 0.0f) {
    if (lhs != rhs) {
      warn << "check_precision fail";
      ret = false;
    }
  } else {
    float diff = abs(lhs - rhs);
    if (diff > precision) {
      warn << "check_precision fails with "
           << "diff: " << diff << " for precision: " << precision;
      ret = false;
    }
  }

  return ret;
}


bool same(qpu::vector const &lhs, qpu::vector const &rhs, float precision) {
  for (int i = 0; i < (int) rhs.size(); ++i) {
    if (!check_precision(lhs[i], rhs[i], precision)) {
      warn << "Fail same(qpu::vector, qpu::vector), index: " << i;
      return false;
    }
  }

  return true;
}

} // namespace qpu

