#include "global.h"
#include "forward.h"
#include "kernel.h"           // gru_kernel::init()

using namespace Log;

namespace {

MAYBE_UNUSED bool same(qpu::vector const &lhs, MatrixXf const &rhs) {
  assert(rhs.rows() == 1);

  for (int i = 0; i < (int) rhs.cols(); ++i) {
    if (!qpu::check_precision(lhs[i], rhs(0, i))) {
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


void LoopState::update_gradient_rows(Model &grad) const {
  //warn << "grad.U_h: " << grad.U_h.dump_dim();
  grad.U_h.outer_add_rows(temp_X, dreluInput_h);

  MMatrix tmp = m_temp.S.mul_e(m_temp.r);
  grad.W_h.outer_add_rows(tmp, dreluInput_h);

  grad.U_r.outer_add_rows(temp_X, dreluInput_r);
  grad.W_r.outer_add_rows(m_temp.S, dreluInput_r);

  grad.U_z.outer_add_rows(temp_X, dreluInput_z);
  grad.W_z.outer_add_rows(m_temp.S, dreluInput_z);
}


void gradient_delta(LoopState &ls, Model &grad, MMatrix &delta_y_x, int time_steps) {
  // Currently, relevant inputs are zero, with the calculation resulting in zero.
  // Skip this function until the inputs are non-zero.
  if (grad.V.is_zero() && ls.temp().S.is_zero()) return;
  warn << "gradient_delta() inputs non-zero! Yay! We can continue." << thrw;      

  MMatrix grad_V_x = grad.V;
  //warn << "delta grad_V_x: " << grad_V_x.dump();
  assert(grad_V_x.same(grad.V));

  timers.start("delta set");
  warn << "ls.temp().S: " << ls.temp().S.dump();

  for(int time_step = time_steps - 1; time_step >= 0; time_step--) {
    grad.V.set(grad.V.Xf() + ls.temp().S.Xf().transpose().eval() * delta_y_x.Xf().row(time_step));
  }

  assert(grad.V.is_zero());
  timers.stop("delta set");

  //
  // TODO: continue with this if values become non-zero
  //
  timers.start("delta set qpu");

  for(int time_step = time_steps - 1; time_step >= 0; time_step--) {
    //MMatrix lhs = ls.temp().S.transpose();
    //MMatrix rhs = delta_y_x.row(time_step);
    //warn << "delta lhs: " << lhs.dump_dim();
    //warn << "delta rhs: " << rhs.dump_dim();

    //grad.V.set(grad.V + ls.temp().S.outer(delta_y_x.row(time_step)));
    grad_V_x.set(grad_V_x * 1.001f + ls.temp().S.outer(delta_y_x.row(time_step)));
  }
  warn << "delta grad.V: " << grad.V.dump_dim();
  warn << "delta grad_V_x: " << grad_V_x.dump_dim();

  //assert(delta_y_x.same(delta_y));
  warn << "Hey!";
  assert(grad_V_x.same(grad.V));
  timers.stop("delta set qpu");
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
  Model &m,
  Model &grad,
  State &state,
  MMatrix const &X,
  MMatrix const &Y,
  int input_dim,
  int hidden_dim,
  int output_dim,
  int time_steps
) {
  warn << "state.S: " << state.S.dump_dim();

  timers.start("back_propagation");

  /* gradients = dLdV, dLdU0, dLdU1, dLdU2, dLdW0, dLdW1, dLdW2 */
  grad.init_val(m.input_dim(), m.hidden_dim(), m.output_dim(), 0.0f, false);

  LoopState ls(1, input_dim, hidden_dim);

  MMatrix delta_y_x = state.O - Y;

  State x_state = state;
  x_state.S = remove_last_rows(1, x_state.S);

  gradient_delta(ls, grad, delta_y_x, time_steps);

  // Difference in calculations between Xf and MMatrix larger than expected
  // All other mul_t() calls work fine.
  // TODO: examine further later
  MMatrix ds_single = m.V.mul_t(delta_y_x /* , true */);  // Enabling true does Xf calculation
  //assert(ds_single.same(ds_single));

  LoopState x_ls(time_steps, input_dim, hidden_dim);

  MMatrix x_ds_cur; x_ds_cur.set(ds_single);

  Model x_grad = grad;

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
    x_ls.update_gradient_rows(x_grad);  // Bulk of the time here
    timers.stop("x_step update_gradient_rows");
  }

  timers.stop("x_step");

  grad.grad_div_steps((float) time_steps);
  //warn << "delta grad.V: " << grad.V.dump();

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


void train(std::string filename_input, std::string filename_output, float learning_rate, int nepoch, int input_dim, int hidden_dim, int output_dim, int time_steps, float decay) {
/*  
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
*/
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
  gru_kernel::init();
  qpu::init();

  for(int epoch = 0; epoch < nepoch; epoch++) {
    warn << "train loop epoch: " << epoch << ", limit: " << limit;
    if (epoch >= 1) break; // DEBUG

    float loss = 0;
    for(int i = 0; i < limit; i++) {
      if (i >= 10) break; // DEBUG
			//if (i % 200 == 0) {
	      warn << "train loop i: " << i;
			//}

      timers.start("train limit loop");

      State state;
      state.init(time_steps, hidden_dim, output_dim);

      MMatrix currX(time_steps, input_dim);
      MMatrix currY(time_steps, output_dim);

      read_x_y(currX, currY, i);
  
      state.eval();

      forward_propagation(m, currX, currY, state, time_steps, input_dim, hidden_dim, output_dim);
      loss += (calculate_cost(state.E, time_steps) / (float) limit);

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


void train_main() {
  //warn << "Called train_main()";
  std::string base = "Examples/NN/GRU/Tools/GRU/Inputs";

  // Dim's must match with input file
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
