#include "matrix.h"
#include "scalar.h"
#include "helpers.h"          // frrand()
#include "./dump.h"
#include "Support/Helpers.h"  // to_file()
#include <cmath>              // std::exp()

namespace qpu {
namespace {
  
void frand_array(Float::Array &rhs) {
  for (int i = 0; i < (int) rhs.size(); ++i) {
    rhs[i] = frand();
  }
}


MAYBE_UNUSED bool same(Float::Array const &lhs, Float::Array const &rhs, int size) {
  for (int i = 0; i < (int) size; ++i) {
    if (lhs[i] != rhs[i]) return false;
  }

  return true;
}


MAYBE_UNUSED bool same(matrix const &lhs, matrix const &rhs) {
  assert(lhs.rows() == rhs.rows());
  assert(lhs.columns() == rhs.columns());

  for (int r = 0; r < lhs.rows(); ++r) {
    for (int c = 0; c < lhs.columns(); ++c) {
      if (lhs.at(r, c) != rhs.at(r, c)) return false;
    }
  }

  return true;
}


/**
 * 
 */
MAYBE_UNUSED bool check_dimensions(matrix const &lhs, matrix const &rhs, bool size_only = false) {
  // Allow transposed vectors
  if (lhs.is_vector() && rhs.is_vector()) {
    size_only = true;
  }

  bool check = size_only?
    (lhs.size() == rhs.size()):
    (lhs.columns() == rhs.columns() && lhs.rows() == rhs.rows())
  ;

  if (!check) {
    cerr << "check_dimensions() dimensions don't match: "
         << "this: " << lhs.dump_dim() << ", "
         << "rhs:"   << rhs.dump_dim();
  }
  //assert(check);

  return check;
}


bool done_init = false;
std::unique_ptr<BaseKernel> s_mul_element;
std::unique_ptr<BaseKernel> s_mult_vec_transposed;
std::unique_ptr<BaseKernel> s_mult_vec;
std::unique_ptr<BaseKernel> s_mult_matrix;
std::unique_ptr<BaseKernel> s_mult_matrix_col;
std::unique_ptr<BaseKernel> s_mult_matrix_t;
std::unique_ptr<BaseKernel> s_matrix_add;
std::unique_ptr<BaseKernel> s_matrix_sub;
std::unique_ptr<BaseKernel> s_mul_float;
std::unique_ptr<BaseKernel> s_mul_float_self;
std::unique_ptr<BaseKernel> s_matrix_add_self;
std::unique_ptr<BaseKernel> s_matrix_sub_self;
std::unique_ptr<BaseKernel> s_sub;
std::unique_ptr<BaseKernel> s_add;
std::unique_ptr<BaseKernel> s_op;
std::unique_ptr<BaseKernel> s_op_add;
std::unique_ptr<BaseKernel> s_op_add_rows;
std::unique_ptr<BaseKernel> s_sigmoid;
std::unique_ptr<BaseKernel> s_dsigmoid;
std::unique_ptr<BaseKernel> s_tanh;
std::unique_ptr<BaseKernel> s_dtanh;
std::unique_ptr<BaseKernel> s_ln;
std::unique_ptr<BaseKernel> s_max_row;
std::unique_ptr<BaseKernel> s_softmax;
std::unique_ptr<BaseKernel> s_softmax_rows;
std::unique_ptr<BaseKernel> s_clip;


void init_local() {
  if (done_init) return;

  s_mul_element        .reset(new BaseKernel(compile(kernel::mul_element    , settings())));
  s_mult_vec_transposed.reset(new BaseKernel(compile(kernel::mult_vec_transposed, settings())));
  s_mult_vec           .reset(new BaseKernel(compile(kernel::mult_vec       , settings())));
  s_mult_matrix        .reset(new BaseKernel(compile(kernel::mult_matrix    , settings())));

  s_mult_matrix_col    .reset(new BaseKernel(compile(kernel::mult_matrix_col, settings())));
  //to_file("s_mult_matrix_col.txt", s_mult_matrix_col->dump());

  s_mult_matrix_t      .reset(new BaseKernel(compile(kernel::mult_matrix_t  , settings())));
  s_matrix_add         .reset(new BaseKernel(compile(kernel::matrix_add     , settings())));
  s_matrix_sub         .reset(new BaseKernel(compile(kernel::matrix_sub     , settings())));
  s_mul_float          .reset(new BaseKernel(compile(kernel::mul_float      , settings())));
  s_mul_float_self     .reset(new BaseKernel(compile(kernel::mul_float_self , settings())));
  s_matrix_add_self    .reset(new BaseKernel(compile(kernel::matrix_add_self, settings())));
  s_matrix_sub_self    .reset(new BaseKernel(compile(kernel::matrix_sub_self, settings())));
  s_sub                .reset(new BaseKernel(compile(kernel::vector_sub     , settings())));
  s_add                .reset(new BaseKernel(compile(kernel::vector_add     , settings())));
  s_op                 .reset(new BaseKernel(compile(kernel::outer_product  , settings())));
  s_op_add             .reset(new BaseKernel(compile(kernel::outer_add      , settings())));
  s_op_add_rows        .reset(new BaseKernel(compile(kernel::outer_add_rows , settings())));
  s_sigmoid            .reset(new BaseKernel(compile(kernel::sigmoid        , settings())));
  s_dsigmoid           .reset(new BaseKernel(compile(kernel::dsigmoid       , settings())));
  s_tanh               .reset(new BaseKernel(compile(kernel::tanh           , settings())));
  s_dtanh              .reset(new BaseKernel(compile(kernel::dtanh          , settings())));
  s_ln                 .reset(new BaseKernel(compile(kernel::ln             , settings())));
  s_max_row            .reset(new BaseKernel(compile(kernel::max_row        , settings())));
  s_softmax            .reset(new BaseKernel(compile(kernel::softmax        , settings())));
  s_softmax_rows       .reset(new BaseKernel(compile(kernel::softmax_rows   , settings())));
  s_clip               .reset(new BaseKernel(compile(kernel::clip           , settings())));

  done_init = true;
}

} // anon namespace


/**
 * @brief Explicitly initialize the QPU kernels
 *
 * Just as with `gru_kernel::init()`, there is a significant overhead on
 * initializing the kernels. Explicit call prevents skewed profile timing.
 */
void init() { init_local(); }


////////////////////////////////////////////////
// Class matrix
////////////////////////////////////////////////

matrix::matrix(int rows, int columns) {
  resize(rows, columns);
  init_local();
}


matrix::matrix(matrix const &rhs) {
  assert(rhs.m_arr != nullptr);
  resize(rhs.m_rows, rhs.m_columns);
  set(*rhs.m_arr);                   // All profile timing here
}


/**
 * Profile timing minimal
 */
matrix::matrix(matrix const &&rhs) {
  transfer(rhs);
}


/**
 * Profile timing insignificant; average effectively 0.
 *
 * Note that the cells are not initialized here.
 */
void matrix::resize(int rows, int columns) {
  // Allow initialization of empty matrix.
  if (rows == 0) {
    assert(columns == 0 || columns == 1);  // By syntax, a single empty vector is allowed
    m_rows    = rows;
    m_columns = columns;
    return;
  }

  if (rows <= 0) { 
    cerr << "matrix resize: rows must be positive"    << thrw;
  }
  if (columns <= 0) {
    cerr << "matrix resize: columns must be positive" << thrw;
  }

  int size = rows*columns;

  if (size > 0 && (size > m_size || m_arr == nullptr)) {
    if (m_size > 0 && size != m_size) {
      warn << "matrix resizing from " << m_size << " to " << size;
    }
    m_arr.reset(new Float::Array(size));
    m_size    = size;
  }

  m_rows    = rows;
  m_columns = columns;
}


matrix matrix::row(int index) const {
  assert(index >= 0 && index < rows());

  int width = columns();
  int offset = index * width;
  matrix ret(1, width);

  // 50x faster than basic loop
  memcpy(ret.arr().ptr(), arr().ptr() + offset, sizeof(float)*width);

  return ret;
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

  // 30x faster than basic loop
  std::fill(r.ptr(), r.ptr() + size(), init_val);
}


/**
 * rhs array can be larger than this, larger than required for elements
 * Only the element memory is copied. 
 *
 * Pre: rows/columns assigned correctly rhs -> this.
 *
 * Profile timing inconsequential now that `copyFrom()` uses `mempcy()`.
 */
void matrix::set(Float::Array const &rhs) {
  assert(!empty()); 
  assert(!rhs.empty()); 

  int copy_size = rows()*columns();
  assert(copy_size > 0);
  assert(copy_size <= (int) rhs.size());
  assert(copy_size <= (int) arr().size());

  arr().copyFrom(rhs, copy_size);
  //OK assert(same(arr(), rhs, copy_size));
}


void matrix::frand() {
  assert(arr().size() > 0);

   for (int i = 0; i < (int) arr().size(); ++i) {
    arr()[i] = frrand();
   }
}


matrix &matrix::operator=(matrix const &rhs) {
  resize(rhs.m_rows, rhs.m_columns);

  if (rhs.m_arr != nullptr) {
    set(*rhs.m_arr);                 // All profile timing here
  }

  return *this;
}


/**
 * Profile timing inconsequential, average basically 0.
 */
matrix &matrix::operator=(matrix const &&rhs) {
  transfer(rhs);
  return *this;
}


matrix matrix::operator-(matrix const &rhs) const {
  assert(check_dimensions(*this, rhs));
  assert(m_columns == rhs.columns() && m_rows == rhs.rows());
  matrix ret(m_rows, m_columns);

  //TODO s_matrix_sub->setNumQPUs(2);
  s_matrix_sub->load(&ret.arr(), &arr(), &rhs.arr(), size()/16).run();
  return ret;
}


matrix &matrix::operator-=(matrix const &rhs) {
  assert(check_dimensions(*this, rhs));
  s_matrix_sub_self->load(&arr(), &rhs.arr(), size()/16).run();
  return *this;
}


matrix matrix::operator+(matrix const &rhs) const {
  assert(check_dimensions(*this, rhs));
  assert(m_columns == rhs.columns() && m_rows == rhs.rows());
  matrix ret(m_rows, m_columns);

  s_matrix_add->load(&ret.arr(), &arr(), &rhs.arr(), size()/16).run();
  return ret;
}


matrix &matrix::operator+=(matrix const &rhs) {
  assert(check_dimensions(*this, rhs));

  // Some juggling to take into account non-16 block sizes
  if (size() % 16 == 0) {
    s_matrix_add_self->load(&arr(), &rhs.arr(), size()/16).run();
  } else {
    for (int i = 0; i < (int) m_arr->size(); ++i) {
      arr()[i] += rhs.arr()[i];
    }
  }

  return *this;
}


matrix matrix::operator*(float rhs) const {
  //warn << "matrix * val: " << rhs;
  matrix ret(m_rows, m_columns);

  //
  // Taking non-conformant matrices into account, not passed to kernel.
  // These happen in train GRU.
  //
  // Mainly handles [1,1] matrices.
  //
  if (size() % 16 != 0) {
    if (size() != 1) {  // Warn me if anything else than (1,1) matrix handled
      warn << "matrix float * not 16-matrix: " << dump_dim();
    }

    for (int i = 0; i < (int) m_arr->size(); ++i) {
      ret.arr()[i] = arr()[i]*rhs;
    }
  } else {
    assert(size() % 16 == 0);
    //warn << "matrix float * 16-matrix:" << dump_dim();
    s_mul_float->load(&ret.arr(), &arr(), rhs, size()/16).run();
  }

  //warn << "matrix * ret: " << ret.dump();
  return ret;
}


matrix &matrix::operator*=(float rhs) {
  //warn << "matrix * val: " << rhs;

  //
  // Taking non-conformant matrices into account, not passed to kernel.
  // These happen in train GRU.
  //
  // Mainly handles [1,1] matrices.
  //
  if (size() % 16 != 0) {
    if (size() != 1) {  // Warn me if anything else than (1,1) matrix handled
      warn << "matrix float *= not 16-matrix: " << dump_dim();
    }

    for (int i = 0; i < (int) m_arr->size(); ++i) {
      arr()[i] = arr()[i] * rhs;
    }
  } else {
    assert(size() % 16 == 0);
    s_mul_float_self->setMaxQPUs();
    s_mul_float_self->load(&arr(), rhs, size()/16).run();
  }

  //warn << "matrix * ret: " << ret.dump();
  return *this;
}


/**
 * @brief Perform multiplication between a matrix and a vector
 *
 * Currently rhs must absolutely be a vector.
 * I fully realize the parameter is confusing.
 */
matrix matrix::mul(matrix const &rhs) const {
  //warn << "Called matrix matrix::operator*()";
  //warn << "matrix: " << dump_dim() << ", rhs: " << rhs.dump_dim();
  assert(rhs.is_vector());
  assert(m_columns > 0);
  assert(m_rows > 0);

  if (m_columns != rhs.size()) {
    cerr << "Matrix::mul() Inner dimension does not match. "
         << "this: " << dump_dim() << ", "
         << "rhs:"   << rhs.dump_dim()
         << thrw;
  }

  assert((m_columns % 16) == 0);      // Inner dimension must be multiple of 16

  matrix ret(m_rows, 1);

  s_mult_vec->load(&rhs.arr(), &arr(), &ret.arr(), m_columns/16, m_rows).run();
  return ret;
}


/**
 * @brief Perform row-first matrix multiplication
 *
 * If there are few rows, multi-QPU is not effective.
 */
matrix matrix::mul_matrix(matrix const &rhs) const {
  //warn << "mul_matrix: " << dump_dim();
  //warn << "mul_matrix rhs: " << rhs.dump_dim();
  assert((m_columns % 16) == 0);      // Inner dimension must be multiple of 16
  assert(m_columns == rhs.m_rows);

  matrix ret;

  // Select row-first if there are enough rows to do multi-QPU
  // resize_16() screws up some calculation!
  if (m_rows >= 16 && rhs.m_columns % 16 == 0) {
    timers.start("matrix * row");
    ret.resize(m_rows, resize_16(rhs.m_columns));  // resize_16() screws up some calculations!
    ret.set(0.0f);

    //s_mult_matrix->setMaxQPUs();
    s_mult_matrix->load(&ret.arr(), &arr(), &rhs.arr(), m_rows, m_columns, rhs.m_columns).run();
    timers.stop("matrix * row");
  } else {
    //timers.start("matrix * col");
    ret.resize(m_rows, rhs.m_columns);
    ret.set(0.0f);

    //s_mult_matrix_col->setMaxQPUs();
    s_mult_matrix_col->load(&ret.arr(), &arr(), &rhs.arr(), m_rows, m_columns, rhs.m_columns).run();
    //timers.stop("matrix * col");
  }

  //warn << "ret: " << ret.dump();
  return ret;
}


/**
 * @brief Perform matrix multiplication, in which the matrices are assumed to be transposed.
 */
matrix matrix::mul_matrix_t(matrix const &rhs) const {
  //warn << "matrix mul_matrix_t lhs: " << dump_dim() << ", rhs: " << rhs.dump_dim();
  assert((m_columns % 16) == 0);      // Inner dimension must be multiple of 16
  assert(m_columns == rhs.m_columns);

  matrix ret(m_rows, resize_16(rhs.m_rows));
  ret.set(0.0f);

  //timers.start("matrix * t");
  s_mult_matrix_t->setMaxQPUs();
  s_mult_matrix_t->load(&ret.arr(), &arr(), &rhs.arr(), m_rows, m_columns, rhs.m_rows).run();
  //timers.stop("matrix * t");

  return ret;
}


/**
 * @brief Perform multiplication between a matrix and a vector,
 *        where the matrix is transposed in the calculation
 */
matrix matrix::mul_t(matrix const &rhs) const {
  //warn << "matrix mul_t: " << dump_dim() << "rhs: " << rhs.dump_dim();
  assert(m_columns > 0);
  assert(m_rows > 0);

  if (m_rows != rhs.rows()) {
    cerr << "Matrix::mul_t() Inner dimension does not match. "
         << "this: " << dump_dim() << ", "
         << "rhs:"   << rhs.dump_dim()
         << thrw;
  }

  assert((m_rows % 16) == 0);

  matrix ret(m_columns, 1);
  s_mult_vec_transposed->load(&rhs.arr(), &arr(), &ret.arr(), m_columns, m_rows/16).run();
  return ret;
}


/**
 * @brief Per-element product of two matrices.
 */
matrix matrix::mul_e(matrix const &rhs) const {
  assert(m_columns > 0);
  assert(m_rows > 0);
  assert((size() % 16) == 0);                 // Total dimension must be multiple of 16
  assert(check_dimensions(*this, rhs, true)); // Keep the calc flexible, just check size
  assert(s_mul_element != nullptr);

  matrix ret(m_rows, m_columns);
  s_mul_element->load(&ret.arr(), &arr(), &rhs.arr(), size()/16).run();

  return ret;
}


matrix matrix::tanh() const {
  matrix bias(rows(), columns());

  matrix ret(rows(), columns());
  s_tanh->load(&arr(), &ret.arr(), size()/16).run();
  return ret;  
}


matrix matrix::dtanh() const {
  matrix bias(rows(), columns());

  matrix ret(rows(), columns());
  s_dtanh->load(&arr(), &ret.arr(), size()/16).run();
  return ret;  
}


matrix matrix::ln() const {
  matrix ret(rows(), columns());
  s_ln->load(&arr(), &ret.arr(), size()/16).run();
  return ret;  
}


matrix matrix::sigmoid() {
  matrix bias(rows(), columns());
  bias.set(0.0f);  // Pedantry, prob not necessary

  matrix ret(rows(), columns());
  s_sigmoid->load(&arr(), &bias.arr(), &ret.arr(), size()/16).run();
  return ret;  
}


matrix matrix::sigmoid(matrix const &bias) {
  matrix ret(rows(), columns());
  s_sigmoid->load(&arr(), &bias.arr(), &ret.arr(), size()/16).run();
  return ret;  
}


matrix matrix::dsigmoid() const {
  matrix ret(rows(), columns());
  int size = rows()*columns();
  s_dsigmoid->load(&arr(), &ret.arr(), size/16).run();
  return ret;  
}


/**
 * NOTE: this is currently a scalar operation
 */
matrix matrix::sigmoid_derivative(matrix const &rhs) {
  // rhs rows and cols are ignored. This is to allow transposed vectors
  assert(size() == rhs.size());

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


/**
 * @brief Calculate softmax per row
 */
void matrix::softmax(matrix &max_row) {
  assert(columns() % 16 == 0);
  assert(rows() == 1);               // Expand when required
  assert(max_row.rows() == rows());
  assert(max_row.columns() == 1);
  float max = max_row.at(0, 0);

/*
   timers.start("matrix softmax scalar");
  // TODO: scalar operation, fix

  //auto &arr = *m_arr;

  for (int i = 0; i < columns(); i++) {
    //arr[i] = std::exp(arr[i] - max);
    //(*m_arr)[i] = std::exp((*m_arr)[i] - max);
    at(0, i) = std::exp(at(0, i) - max);
  }

   timers.stop("matrix softmax scalar");
*/  

   //timers.start("matrix softmax qpu");
  s_softmax->load(&arr(), max, columns()).run();
  //s_softmax_rows->load(&arr(), &max_row.arr(), rows(), columns()).run();
   //timers.stop("matrix softmax qpu");

  //warn << "softmax this: " << dump();
}


/**
 * TODO: currently scalar, convert to QPU kernel
 */
float matrix::sum() const {
  auto &arr = *m_arr;
  float ret = 0.0f;

  for (int i = 0; i < size(); i++) {
    ret += arr[i];
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

matrix outer_ret;  

void outer_check(matrix const &lhs, matrix const &rhs) {
  //assert(lhs.is_vector());
  //assert(rhs.is_vector());
  if ((lhs.size() & 0xf) != 0) { cerr << "vector outer: expecting lhs rows to be a multiple of 16" << thrw; }
  if ((rhs.size() & 0xf) != 0) { cerr << "vector outer: expecting rhs rows to be a multiple of 16" << thrw; }
}

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
  outer_check(*this, rhs);
  outer_ret.resize(size(), rhs.size());

  s_op->load(&outer_ret.arr(), &arr(), &rhs.arr(), size(), rhs.size()/16).run();

  //warn << "outer resize: " << outer_ret.dump_dim();
  return outer_ret;  
}


void matrix::outer_add(matrix const &lhs, matrix const &rhs) {
  outer_check(lhs, rhs);
  assert(rows() == lhs.size() && columns() == rhs.size());

  s_op_add->setMaxQPUs();
  //s_op_add_rows->setNumQPUs(2);
  s_op_add->load(&arr(), &lhs.arr(), &rhs.arr(), lhs.size(), rhs.size()/16).run();
}


void matrix::outer_add_rows(matrix const &lhs, matrix const &rhs) {
  assert(rows() == lhs.columns() && columns() == rhs.columns());
  assert(rhs.columns() % 16 == 0);
/*  
  warn << "outer_add_rows lhs: " << lhs.dump_dim() << ", "
       << "rhs: " << rhs.dump_dim() << ", "
       << "ret: " << dump_dim();
*/
  timers.start("matrix::outer_add_rows");
    
  //s_op_add_rows->setMaxQPUs();
  s_op_add_rows->setNumQPUs(1);
  s_op_add_rows->load(&arr(), &lhs.arr(), &rhs.arr(), lhs.rows(), lhs.columns(), rhs.columns()).run();

  timers.stop("matrix::outer_add_rows");
}


std::string matrix::dump_dim() const {
  return src().dump_dim();
}


std::string matrix::dump(bool output_int) const {
  return matrix_dump(src(), output_int);
}


void matrix::transfer(matrix const &rhs) {
  m_columns  = rhs.m_columns;
  m_rows     = rhs.m_rows;
  m_arr      = rhs.m_arr;  // nullptr allowed
  m_size     = rhs.m_size;
}


/**
 * @brief Calculate max per row
 *
 * Output is a row-vector.
 */
void matrix::max_row(matrix &ret) const {
  assert(columns() % 16 == 0);
  assert(ret.rows() == rows());
  assert(ret.columns() == 1);

  s_max_row->load(&ret.arr(), &arr(), rows(), columns()).run();
}


FloatArrayAdapter matrix::src() const {
  assert(m_arr != nullptr);
  return FloatArrayAdapter(*m_arr, m_rows, m_columns);
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

  if (rhs.columns() == 1) {
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
  return rows()*columns();
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


/**
 * Value changed internally, no return value.
 */
void vector::clip(float clip_value) {
  vector ret(rows());

  s_clip->load(&arr(), &ret.arr(), rows()/16, clip_value).run();
  *this = ret;
}


std::string vector::dump(bool output_int) const {
  assert(false); // Warn me if when is called

  auto tmp_src = src();

  std::string ret;
  ret << tmp_src.dump_dim()
      << "[" << vector_dump(tmp_src, 0, output_int) << "]";

  return ret;
}


BaseKernel &vector::op_kernel() {
  init_local();
  return *s_op;
}

/*
vector operator*(matrix const &lhs, matrix const &rhs) {
  //warn << "rhs: " << rhs.dump_dim();
  assert(rhs.is_vector());

  matrix ret;
  ret = lhs.mul(rhs);
  return vector(ret);
}
*/


////////////////////////////////////////
// Class CompareStats
////////////////////////////////////////

void CompareStats::reset() {
  first_i  = -1;
  first_j  = -1;
  total    = 0;
  exact    = 0;
  same     = 0;
  zeroes   = 0;
   max_diff = 0.0f;
   max_bit  = -1;
}


bool CompareStats::failed() const {
  assert((first_i == -1 && first_j == -1) || (first_i != -1 && first_j != -1));
  return (first_i != -1);
}


std::string CompareStats::dump(bool show_first_fail) const {
  std::string ret;

  auto as_percent = [] (int denom, int div) -> std::string {
    std::string ret;

    float val = (float) denom/ (float) div*100.0f;

    ret << (int) floor(val) << "." << (int) floor((val - (long) val)*100.0f) << "%";
    return ret;
  };

  auto width = [] (int val) -> int {
    std::string buf;
    buf << val;
    return (int) buf.size();
  };

  auto format = [&width] (int in_width, int val) -> std::string {
    int spaces = in_width - width(val);

    std::string ret;

    for (int i = 0; i < spaces; ++i) {
      ret << " ";
    }

    ret << val;

    return ret;
  };

  ret << "Compare Stats:";

  if (fail_on_first() && failed()) {
     ret << " failed on first, stats incomplete";
    return ret;
  }

  if (exact == total) {
     ret << " exact";
    return ret;
  }

  if ((exact + same) == total) {
     ret << " same";
    return ret;
  }

  ret << "\n";

  if (show_first_fail) {
    ret << "  first fail: (" << first_i << ", " << first_j  << ")\n";
  }

  int x_width = width(exact);

  ret << "  total     : " << total << "\n"
      << "  exact     : " << exact                  << ", " << as_percent(exact, total)        << "\n"
      << "  same      : " << format(x_width, same)  << ", " << as_percent(exact + same, total) << "\n"
      << "  zeroes    : " << zeroes   << "\n"
      << "  max_diff  : " << max_diff << "\n"
      << "  max_bit   : " << max_bit;

  return ret;
}

// End class CompareStats


bool check_precision(
  float lhs,
  float rhs,
  int bit_diff,
  CompareStats *stats,
  bool do_show
) {
  bool ret = true;
  float diff = abs(lhs - rhs);
  int bit = V3DLib::bit_diff(lhs, rhs, bit_diff);

  bool failed = false;

  if (bit_diff > -1) {
    failed = (bit > -1);
  } else {
    failed = (lhs != rhs);
  }
    
  if (failed) {
    if (do_show) {
      warn << "check_precision fail, diff: " << diff << ", " << "bit: " << bit;
    }
    ret = false;
  }

  if (stats != nullptr) {
    stats->total++;
    if (diff > stats->max_diff) stats->max_diff = diff;
    if (bit  > stats->max_bit)  stats->max_bit = bit;

    if (lhs == rhs) {
      stats->exact++;

      if (lhs == 0.0f) {
        stats->zeroes++;
      } //else {
      //  warn << "Exact but not zero: " << lhs << ", " << rhs;
      //}
    } else {
      if (ret) stats->same++;
    }
  }

  return ret;
}


namespace {

bool same_intern(
  qpu::matrix const &lhs,
  qpu::matrix const &rhs,
  int bit_diff,
  CompareStats &stats
) {
  //warn << "Called same_intern(qpu::matrix, qpu::matrix)";
  bool ret = true;

  // Special case for 2 input vectors: accept transposed vectors
  if (lhs.columns() == 1 && lhs.columns() == rhs.rows() && lhs.rows() == rhs.columns() ) {
    int size = lhs.rows();
    for (int i = 0; i < size; ++i) {
      if (!qpu::check_precision(lhs.at(i, 0), rhs.at(0, i), bit_diff, &stats)) {
         if (ret) {
          stats.first_i = i;
          stats.first_j = 0;
        }

        ret = false;
        if (stats.fail_on_first()) break;
      }      
    }

    return ret;
  }

  //
  // Do full matrices
  //

  if (lhs.rows() != rhs.rows() || lhs.columns() != rhs.columns() ) {
     warn << "Fail same_intern(qpu::matrix, qpu::matrix) dimensions differ: "
          << "lhs: " << lhs.dump_dim() << ", "
          << "rhs: " << rhs.dump_dim();

     return false;
  }

  for (int i = 0; i < (int) rhs.rows(); ++i) {
    if (stats.fail_on_first() && !ret) break;

    for (int j = 0; j < (int) rhs.columns(); ++j) {
      if (!qpu::check_precision(lhs.at(i, j), rhs.at(i, j), bit_diff, &stats, ret)) {
        if (ret) {  // Register first fail only
          stats.first_i = i;
          stats.first_j = j;
        }

        ret = false;
        if (stats.fail_on_first()) break;
      }      
    }
  }

  return ret;
}

} // anon namespace


bool same(qpu::matrix const &lhs, qpu::matrix const &rhs, int bit_diff,  bool show_stats) {
  //warn << "Called same(qpu::matrix, qpu::matrix)";

  CompareStats stats(true);

  bool ret = same_intern(lhs, rhs, bit_diff, stats);

  if (stats.failed()) {
    warn << "Fail same() at (i,j): (" << stats.first_i << ", " << stats.first_j  << ")";
  }

  if (show_stats) {
     warn << stats.dump();
  }
  return ret;
}


void diff(qpu::matrix const &lhs, qpu::matrix const &rhs, int bit_diff) {
  //warn << "Called diff(qpu::matrix, qpu::matrix)";
  CompareStats stats(false);
  same_intern(lhs, rhs, bit_diff, stats);
   warn << stats.dump(true);
}


bool same(qpu::vector const &lhs, qpu::vector const &rhs) {
  assert(false); // Warn me when called

  for (int i = 0; i < (int) rhs.size(); ++i) {
    if (!check_precision(lhs[i], rhs[i])) {
      warn << "Fail same(qpu::vector, qpu::vector), index: " << i;
      return false;
    }
  }

  return true;
}

} // namespace qpu

