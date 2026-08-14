#include "global.h"
#include "forward.h"
#include "kernel.h"           // gru_kernel::init()
#include "LibSettings.h"

using namespace Log;

namespace {

MAYBE_UNUSED bool same(qpu::vector const &lhs, MatrixXf const &rhs) {
  assert(rhs.rows() == 1);

  for (int i = 0; i < (int) rhs.cols(); ++i) {
    if (!check_precision(lhs[i], rhs(0, i))) {
      warn << "Fail same(qpu::vector, MatrixXf), index: " << i;
      return false;
    }      
  }

  return true;
}


/**
 * Possibly - Move to forward propagation or Train
 */
float calculate_cost(MMatrix const &E, int time_steps) {
  return (E.sum() / (float) time_steps);
}


class LoopState {
private:  
  State m_temp;

public:  
  MMatrix temp_X;
  MMatrix ds_cur_bk;
  MMatrix dreluInput_h;
  MMatrix dreluInput_r;
  MMatrix dreluInput_z;
  MMatrix dsr;

  LoopState(int time_steps, int input_dim, int hidden_dim);

  State const &temp() const { return m_temp; }

  void x_set_step(int step, State state, MMatrix const &X);

  void init_drelu(MMatrix const &ds_cur, Model const &m);
  void update(MMatrix &ds_cur, Model const &m) const;
  void update_gradient_rows(Model &grad) const;
};


LoopState::LoopState(int time_steps, int input_dim, int hidden_dim) :
  m_temp(true),
  temp_X(time_steps, input_dim),
  ds_cur_bk(time_steps, hidden_dim)
{
  m_temp.init(time_steps, hidden_dim, -1);
}


void LoopState::x_set_step(int step, State x_state, MMatrix const &X) {
  m_temp.move_rows(step, x_state);
  temp_X.move_rows(step, X);
}


void LoopState::init_drelu(MMatrix const &ds_cur, Model const &m) {
  ds_cur_bk = ds_cur;

  dreluInput_h.back_prop_1(ds_cur, m_temp);

  dsr = m.W_h.mul_t(dreluInput_h);
  dreluInput_r.back_prop_3(dsr, m_temp);

  dreluInput_z.back_prop_4(ds_cur_bk, m_temp);
}


void LoopState::update(MMatrix &ds_cur, Model const &m) const {
  ds_cur  = dsr.mul_e(m_temp.r);
  ds_cur += m.W_r.mul_t(dreluInput_r);
  ds_cur += ds_cur_bk.mul_e(m_temp.z);
  ds_cur += m.W_z.mul_t(dreluInput_z);
}


/**
 * Verified that values change here.
 *
 * Biggest time hog in calculating first entire epoch of train:
 *
 * numQPUs == 1:
 *
 *     x_step update_gradient_rows: 2704.828095s in 132840 steps, average:  0.020361s
 *
 * Using multiple QPU's dramatically improves this. numQPUs == 16:
 *
 *     x_step update_gradient_rows: 241.045550s in 132840 steps, average:  0.001814s
 */
void LoopState::update_gradient_rows(Model &grad) const {
  grad.U_h.outer_add_rows(temp_X, dreluInput_h);   // Changes val

  MMatrix tmp = m_temp.S().mul_e(m_temp.r);
  grad.W_h.outer_add_rows(tmp, dreluInput_h);      // Changes val, > 99% same


  grad.U_r.outer_add_rows(temp_X, dreluInput_r);   // Changes val, > 99% same
  grad.W_r.outer_add_rows(m_temp.S(), dreluInput_r); // Changes 1 of 16384 values
  grad.U_z.outer_add_rows(temp_X, dreluInput_z);   // changes val, > 83% same
  grad.W_z.outer_add_rows(m_temp.S(), dreluInput_z); // Changes val, > 99% same
}


/**
 * @brief Calculate gradient delta.
 *
 * At time of writing, this does nothing because input parameters are zero.
 *
 * @param grad_v    Output parameter
 * @param S         Input parameter
 * @param delta_y_x Input parameter
 *
 * ------------------------------------------------
 *
 * Notes
 * =====
 *
 * 1. Final `eval()` important. Otherwise, non-zero values are returned.
 *    Expecting tmp to be zeroes when all inputs are zero.  
 *    Can't do afterward, e.g.  `tmp.eval()`; apparently, needs to be in calculation.
 */
void gradient_delta(MMatrix &grad_V, MMatrix const &S, MMatrix const &delta_y_x, int time_steps) {
  assert(grad_V.is_zero());

#if 0
	//
	// S is now non_zero; check when delta_y_x changes
	//
  bool input_zero = (grad_V.is_zero() && delta_y_x.is_zero());
	if (!input_zero) {
	  warn << "Called gradient_delta()"
	       << ", Zero (grad.V, S, delta_y_x): "
	       << "(" << grad_V.is_zero() << ", " << S.is_zero() << ", " << delta_y_x.is_zero() << ")";

  	//warn << "S: " << S.dump();
  	//warn << "delta_y_x: " << delta_y_x.dump();
  	//assert(delta_y_x.is_zero());  // Warn me when this changes
	}
#endif	

  MMatrix grad_V_pre = grad_V; // Remember original state to compare with
  MMatrix grad_V_x   = grad_V; // Copy param for qpu

	{ // Xf
  	timers.start("delta set Xf");

	  for (int time_step = time_steps - 1; time_step >= 0; time_step--) {
			// See Note 1 for following line.
	    auto tmp = (S.row(time_step + 1).Xf().transpose() * delta_y_x.Xf().row(time_step)).eval();
	    grad_V.set(grad_V.Xf() + tmp);
  	}

	  timers.stop("delta set Xf");
	}

  //
  // TODO: continue with this if values become non-zero
  //
	{ // qpu
	  timers.start("delta set qpu");

	  for (int time_step = time_steps - 1; time_step >= 0; time_step--) {
	    // Following always non-zero
	    //warn << "delta_y_x(" << time_step << "): " << delta_y_x.row(time_step).dump();

	    // delta_y_x is non-zero for every time_step
	    auto tmp = S.row(time_step + 1).outer(delta_y_x.row(time_step));
	    grad_V_x.set(grad_V_x + tmp);
	  }

	  timers.stop("delta set qpu");
	}

	//
	// Post Check: grad_V and similar should be changing
	//
	// They are changing now; check QPU and Xf are same
	//
  assert(grad_V_x.same(grad_V));
  assert(!grad_V.is_zero());
}

} // anon namespace


/**
 * =============================
 * Notes
 * -----
 *
 * 1. Xf/qpu values diverge slightly in the back_prop_x() calls, on the order of <= 1.0e-10.
 *    Due to error accumulation, comparison assertions fail. So be it. QPU is leading now
 *
 * 2. QPU back prop fully operational, timing before optimization:
 *
 *      train limit loop          : 45.632033s in    10 steps, average:  4.563203s
 */
void back_propagation(
  Model const &m,
  Model &grad,
  State const &state,
  MMatrix const &X,
  MMatrix const &Y,
  int input_dim,
  int hidden_dim,
  int output_dim,
  int time_steps
) {
  //warn << "Called back_propagation";

  timers.start("back_propagation");

  /* gradients = dLdV, dLdU0, dLdU1, dLdU2, dLdW0, dLdW1, dLdW2 */
  grad.init_val(m.input_dim(), m.hidden_dim(), m.output_dim(), 0.0f, false);

	//warn << "state.O: " << state.O.dump();
	//warn << "Y: " << Y.dump();  // Non-zero

  MMatrix delta_y_x = state.O - Y;
	//warn << "delta_y_x: " << delta_y_x.dump();

  State x_state = state;
  x_state.S() = remove_last_rows(1, x_state.S());

  gradient_delta(grad.V, state.S(), delta_y_x, time_steps);

  // Difference in calculations between Xf and MMatrix larger than expected
  // All other mul_t() calls work fine.
  // TODO: examine further later
	bool do_Xf_calc = false;
  MMatrix ds_single = m.V.mul_t(delta_y_x, do_Xf_calc);

  LoopState x_ls(time_steps, input_dim, hidden_dim);

  MMatrix x_ds_cur; x_ds_cur.set(ds_single);

  timers.start("x_step");

  int x_step = time_steps;
  for (int x = 0; x < x_step; ++x) {
    //warn << "x_step x: " << x;

    timers.start("x_step x_set_step");
    x_ls.x_set_step(x, x_state, X);
    timers.stop("x_step x_set_step");

    timers.start("x_step init_drelu");
    x_ls.init_drelu(x_ds_cur, m);
    timers.stop("x_step init_drelu");

    timers.start("x_step update");
    x_ls.update(x_ds_cur, m);
    timers.stop("x_step update");

    timers.start("x_step update_gradient_rows");
    x_ls.update_gradient_rows(/*x_ */ grad);  // Bulk of the time here
    timers.stop("x_step update_gradient_rows");
  }

  timers.stop("x_step");

  grad.grad_div_steps(time_steps);

  timers.stop("back_propagation");
}


void rms_prop(Model &m, Model &grad, Model &cache, float learning_rate, int input_dim, int hidden_dim, int output_dim) {
    float decay = 0.9f;

    Model grad_total;
    //No effect grad_total.init(m.input_dim(), m.hidden_dim(), m.output_dim());
    grad_total.init_val(m.input_dim(), m.hidden_dim(), m.output_dim(), 0.0f, false);

    cache.cache_decay(decay, grad);
    grad_total.divide(grad, cache);
    m.adjust_learning_rate(learning_rate, grad_total);
    grad.init_val(m.input_dim(), m.hidden_dim(), m.output_dim(), 0.0f, true);

    m.eval();
}


void gradient_descent(Model &m, Model &grad, float learning_rate) {
  m.adjust_learning_rate(learning_rate, grad);
  m.eval();
  grad.init_val(m.input_dim(), m.hidden_dim(), m.output_dim(), 0.0f, true);
}


int get_input_size(std::string filename) {
    std::ifstream inputFile(filename);
    assert(!inputFile.fail());

    int n, inputSize = 0;
    while(!inputFile.eof()){
        inputFile >> n;
        inputSize++;
    }

    inputFile.close();
    return inputSize;
}


namespace {

std::vector<int> x_input;
std::vector<int> x_output;

std::vector<int> load_file(std::string filename) {
  std::vector<int> ret;

  filename.replace(filename.end() - 3, filename.end(), "bin");
  //warn << "filename: " << filename;

  std::ifstream file_input(filename, std::ios::binary);
  assert(!file_input.fail());

  file_input.seekg(0, std::ios::beg);
  uint32_t a = 0;

  while(!file_input.eof()) {
    file_input.read((char*)&a, sizeof(uint32_t));
    ret.push_back((int) a);
  }

  file_input.close();

  return ret;
}


void load_x_y(std::string filename_input, std::string filename_output) {
  assert(x_input.empty());
  assert(x_output.empty());

  x_input  = load_file(filename_input);
  x_output = load_file(filename_output);
}


void read_x_y(MMatrix &x, MMatrix &y, int pos) {
  //timers.start("read_x_y");
  assert(!x_input.empty());
  assert(!x_output.empty());

  x.set(x_input, pos);
  y.set(x_output, pos);

  //timers.stop("read_x_y");
}

}  // anon namespace


void train(
	std::string filename_input,
	std::string filename_output,
	float learning_rate,
	int nepoch,
	int input_dim,
	int hidden_dim,
	int output_dim,
	int time_steps,
	float decay
) {
  float prev_loss = 0.0f;

  int inputSize   = get_input_size(filename_input) - time_steps - 1;
  int limit       = inputSize / 50;
  float min_loss  = 99999999.0f;
  //std::cout << "Size of input file is : " << inputSize << std::endl;

  Model m;
  Model grad;
  Model cache;

  m.init(input_dim, hidden_dim, output_dim);

  grad.init_val(m.input_dim(), m.hidden_dim(), m.output_dim(), 0.0f, true);
  cache.init_val(m.input_dim(), m.hidden_dim(), m.output_dim(), 1.0f, false);

  load_x_y(filename_input, filename_output);

  for(int epoch = 0; epoch < nepoch; epoch++) {
    warn << "train loop epoch: " << epoch << ", limit: " << limit;
		if (epoch > 1) return;

    float loss = 0;
    for(int i = 0; i < limit; i++) {
      if (true) {
        if (i >= 10) break; // DEBUG
        warn << "train loop i: " << i;
      } else {
        if (i % 400 == 0) {
          warn << "train loop i: " << i;
        }
      }

      timers.start("train limit loop");

      State state;
      state.init(time_steps, hidden_dim, output_dim);
			//warn << "state init: " << state.dump();

      MMatrix currX(time_steps, input_dim);
      MMatrix currY(time_steps, output_dim);

      read_x_y(currX, currY, i);
      //warn << "currX: " << currX.dump();
      //warn << "currY: " << currY.dump();
  
      state.eval();

      forward_propagation(m, currX, currY, state, time_steps, input_dim, hidden_dim, output_dim);
			//warn << "state forward: " << state.dump();
			//warn << "state.O forward: " << state.O.dump();

      loss += (calculate_cost(state.E, time_steps) / (float) limit);
      //warn << "loss: " << loss;

      back_propagation(m, grad, state, currX, currY, input_dim, hidden_dim, output_dim, time_steps);

      // gradient_descent(m, grad, learning_rate);

      rms_prop(m, grad, cache, learning_rate, input_dim, hidden_dim, output_dim);

      timers.stop("train limit loop");
    }

    std::cout << "Loss: " << loss << ", Epoch: "<< epoch << std::endl;

    if (loss > prev_loss && prev_loss != 0) {
      learning_rate = learning_rate * 1;
      std::cout << "Adjusting learning rate to " << learning_rate << std::endl;
    }
    prev_loss = loss;
    learning_rate *= 1 / (1 + (decay * (float) epoch));

    if (loss < min_loss){
      min_loss = loss;
    }
  }
}

namespace {

MAYBE_UNUSED void unit_test() {
  if (false) {  
    warn << "=== Testing matrix     ===";
    qpu::matrix lhs(3, 16);
    lhs.set(1.0f);
    qpu::matrix rhs(7, 16);
    rhs.set(3.0f);

    auto ret = rhs.mul_matrix_t(lhs);
    warn << "lhs: " << lhs.dump();
    warn << "rhs: " << rhs.dump();
    warn << "ret: " << ret.dump();

    warn << "=== End testing matrix ===";
  }

/*
  // Work as expected

  //
  // Test matrix multiplication
  //
  MMatrix m(10, 16, 1.0f);
  warn << "unit_test *= pre: " << m.dump();

  auto m2 = m * 20.0f;
  warn << "unit_test * m2 post: " << m2.dump();

  m /= 20.0f;
  warn << "unit_test *= post: " << m.dump();

  //
  // Test outer()
  //
  MMatrix r1(16,  1, 1.5f);
  MMatrix r2(16,  1, 2.0f);  // Will be transposed in outer()
  
  MMatrix expected(16, 16, 3.0f);
  auto res = r1.outer(r2);
  //warn << "unit_test res: " << res.dump();
  assert(res.same(expected));

  MMatrix expected2(16, 16, 4.0f);
  MMatrix res2(16, 16, 1.0f);
  res2.outer_add(r1, r2);
  warn << "unit_test outer_add: " << res2.dump();
  assert(res2.same(expected2));

  MMatrix r3(16, 16, 1.0f);
  MMatrix r4(16, 16, 2.0f);
  MMatrix expected3(16, 16, 35.0f);
  MMatrix res3(16, 16, 3.0f);
  res3.outer_add_rows(r3, r4);
  warn << "unit_test outer_add_rows: " << res3.dump();
  assert(res3.same(expected3));
*/  

  //
  // Test max dimensions in train
  //
  // Works as expected.
  // Adding this triggered heap overflow in 'big' step.
  //
  if (false) {
    const int Rows = 20;
    const int Mul  = 8;

    MMatrix r5(Rows, 16*Mul, 1.0f);
    MMatrix r6(Rows, 16*Mul, 2.0f);
    MMatrix expected4(16*Mul, 16*Mul, Rows*2.0f + 3.0f);
    MMatrix res4(16*Mul, 16*Mul, 3.0f);

    res4.outer_add_rows(r5, r6);
    // warn << "unit_test max outer_add_rows: " << res4.dump();
    assert(res4.same(expected4));
  }


  //
  // Test big matrices
  //
  // For big enough matrices, the calculation fails. Unclear why, the values are well within Int range.
  // Num QPU's also has an effect.
  //
  // This is not critical right now, the values for GRU train are well within the limits (see
  // previous 'max' test). But when GRU is upscaled to bigger NN's, this will definitely be an issue.
  //
  // ------------------------------------------------
  // Results for v3d
  // ===============
  //
  // Default heap size: 8MB
  //
  //             Mul
  //             --------------------
  // QPU  Rows   Good   Bad  Overflow  Comment
  // ===  =====  =====  ===  ========  =======
  //  1   16* 1  <= 63       64
  //  1   16* 8  <= 32  33 
  //  1   16*16  <= 23  24             Failed sometimes on initial calls
  //  1   16*32  <= 16
  //  2   16*16  <= 32
  //  2   16*32  <= 22
  //  4   16*32  <= 30
  // 16   16* 1  <= 72       96         heap size 16MB
  // 16   16* 8  <= 60       61
  // 16   "      <= 48                  heap size 16MB
  // 16   16*16  <= 56       57
  //
  if (false) {
    const int Rows = 16*16;
    const int Mul  = 23;

    MMatrix r5(Rows, 16*Mul, 1.0f);
    warn << "r5: " << r5.dump_dim();

    MMatrix r6(Rows, 16*Mul, 2.0f);
    MMatrix expected4(16*Mul, 16*Mul, Rows*2.0f + 3.0f);
    MMatrix res4(16*Mul, 16*Mul, 3.0f);
    warn << "res4: " << res4.dump_dim();

    res4.outer_add_rows(r5, r6);
    warn << "unit_test big outer_add_rows: " << res4.dump();
    assert(res4.same(expected4));
  }
}

} // anon namespace


void train_main() {
  gru_kernel::init();
  qpu::init();

  //unit_test();
  //return;

  //warn << "Called train_main()";
  std::string base = "Examples/NN/GRU/Tools/GRU/Inputs";

  // Dimensions must match with input file
  int input_dim       = 64;
  int hidden_dim      = 128;
  int output_dim      = 64;
  float learning_rate = 0.0005f;
  int nepochs         = 1; //1000;
  int time_steps      = 20;
  float decay         = 0.000f;

  train(
    base + "/donald_trump_input.txt",
    base + "/donald_trump_output.txt",
    learning_rate,
    nepochs,
    input_dim,
    hidden_dim,
    output_dim,
    time_steps,
    decay
  );
}
