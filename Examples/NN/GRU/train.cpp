#include "model.h"
#include "helpers.h"  // resize_16()

using namespace Log;

namespace {

const float Precision = 1.0e-7f;  

MAYBE_UNUSED bool same(qpu::vector const &lhs, MatrixXf const &rhs, float precision = 0.0f) {
  assert(rhs.rows() == 1);

  for (int i = 0; i < (int) rhs.cols(); ++i) {
    if (!qpu::check_precision(lhs[i], rhs(0, i), precision)) {
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
  return (E.Xf().sum() / (float) time_steps);
}


MAYBE_UNUSED MMatrix remove_last_rows(int num, MMatrix const &rhs) {
  if (num == 0) {
    // Nothing to do
    return rhs;
  }

  //warn << "Called remove_last_rows(" << num << ")";

  assert(num > 0 && num < rhs.rows());

  MMatrix ret(rhs.rows() - num, rhs.cols());

  for (int i = 0; i < rhs.rows() - num; ++i) {
    ret.row(i, rhs.row(i), false);
  }

  ret.sync_qpu();
/*
  warn << "remove_last_rows(" << num << "):\n"
       << "rhs: " << rhs.dump() << "\n"
       << "ret: " << ret.dump() << "\n";
*/
  return ret;
}


MAYBE_UNUSED MMatrix to_matrix(int rows, MMatrix const &vec) {
  assert(vec.rows() == 1);
  assert(vec.cols() > 1);

  MMatrix ret(rows, vec.cols());

  for (int i = 0; i < rows; ++i) {
    ret.row(i, vec, false);
  }

  ret.sync_qpu();
  return ret;
}


std::vector<int> x_input;
std::vector<int> x_output;


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

  State const &temp() const { return  m_temp; }

  void set_step(int time_step, State const &state, MatrixXf const &X);
  void x_set_step(int step, State state, MatrixXf const &X);

  void temp_init(int time_steps, State const &state);
  void init_drelu(MMatrix const &ds_cur, Model const &m);
  void update(MMatrix &ds_cur, Model const &m) const;
  void update_gradient(Model &grad) const;
  void update_gradient_rows(Model &grad) const;
};


LoopState::LoopState(int time_steps, int input_dim, int hidden_dim) :
  m_temp(true),
  temp_X(time_steps, input_dim),
  ds_cur_bk(time_steps, hidden_dim)
{
  m_temp.init(time_steps, hidden_dim, -1);
}


void LoopState::set_step(int time_step, State const &state, MatrixXf const &X) {
  m_temp.set_step(time_step, state);
  temp_X.set(X.row(time_step));
}


void LoopState::x_set_step(int step, State state, MatrixXf const &X) {
	state.S = remove_last_rows(1, state.S);
  m_temp.move_rows(step, state);

	//warn << "X: " << dump(X);
	MMatrix tmp;
	tmp.set(X);
  temp_X = move_rows(step, tmp);
	//warn << "temp_X: " << temp_X.dump();
}


/**
 * Prob not necessary, m_temp.S gets overwritten in set_step().
 * TODO check this
 *
 * Don't call this for x_ls.
 */
void LoopState::temp_init(int time_steps, State const &state) {
  for(int time_step = time_steps - 1; time_step >= 0; time_step--) {
		// NOTE: This sets S to a single row 20 times
    m_temp.S.set(state.S.row(time_step + 1));
  }

  //warn << "temp_init m_temp.S: "     << m_temp.S.dump_dim();
}


void LoopState::init_drelu(MMatrix const &ds_cur, Model const &m) {
  ds_cur_bk = ds_cur;

  dreluInput_h.back_prop_1(ds_cur, m_temp);
  dreluInput_h.sync_qpu();                                  // See Note 1

  //warn << "m.W_h: " << m.W_h.dump_dim();
  //warn << "ls.dreluInput_h: " << ls.dreluInput_h.dump_dim();
  dsr = m.W_h * dreluInput_h;
  //assert(dsr.same(Precision));                            // convergence Xf/qpu progressively worse
  dsr.sync_qpu();                                           // See Note 1

  dreluInput_r.back_prop_3(dsr, m_temp, Precision);
  dreluInput_r.sync_qpu();                                  // See Note 1
  //warn << "ls.dreluInput_r: " << ls.dreluInput_r.dump_dim();

  dreluInput_z.back_prop_4(ds_cur_bk, m_temp, 3*Precision);   // convergence Xf/qpu gets progressively worse
  dreluInput_z.sync_qpu();                                  // See Note 1
}


void LoopState::update(MMatrix &ds_cur, Model const &m) const {
  ds_cur  = dsr.mul_e(m_temp.r);
  ds_cur += m.W_r * dreluInput_r;
  ds_cur += ds_cur_bk.mul_e(m_temp.z);
  ds_cur += m.W_z * dreluInput_z;
}


void LoopState::update_gradient(Model &grad) const {
  grad.U_h.outer_add(temp_X, dreluInput_h);

  MMatrix temp_W;
  temp_W.back_prop_2(m_temp, dreluInput_h, Precision);
  grad.W_h += temp_W;                                       //OK assert(grad.W_h.same());

  grad.U_r.outer_add(temp_X, dreluInput_r);
  grad.W_r.outer_add(m_temp.S, dreluInput_r);

  grad.U_z.outer_add(temp_X, dreluInput_z);                 //OK assert(grad.U_z.same());
  grad.W_z.outer_add(m_temp.S, dreluInput_z);
}


MAYBE_UNUSED void LoopState::update_gradient_rows(Model &grad) const {
  grad.U_h.outer_add_rows(temp_X, dreluInput_h);

  //MMatrix temp_W;
  //temp_W.back_prop_2(m_temp, dreluInput_h, Precision);
  //grad.W_h += temp_W;                                       //OK assert(grad.W_h.same());

	MMatrix tmp = m_temp.S.mul_e(m_temp.r);
  grad.W_h.outer_add_rows(tmp, dreluInput_h);

  grad.U_r.outer_add_rows(temp_X, dreluInput_r);
  grad.W_r.outer_add_rows(m_temp.S, dreluInput_r);

  grad.U_z.outer_add_rows(temp_X, dreluInput_z);                 //OK assert(grad.U_z.same());
  grad.W_z.outer_add_rows(m_temp.S, dreluInput_z);
}

} // anon namespace


/**
 * =============================
 * Notes
 * -----
 *
 * 1. DEBUG: Xf/qpu values diverge slightly in the back_prop_x() calls, on the order of <= 1.0e-10.
 *    Due to error accumulation, assertions fail.
 *    For debugging/testing purposes, qpu is syned with Xf. Eventually, this will be removed.
 */
void back_propagation(Model &m, Model &grad, State &state, MatrixXf& X, MatrixXf& Y, int input_dim, int hidden_dim, int output_dim, int time_steps) {
  timers.start("back_propagation");

  /* gradients = dLdV, dLdU0, dLdU1, dLdU2, dLdW0, dLdW1, dLdW2 */
  grad.init_zeroes(m.input_dim(), m.hidden_dim(), m.output_dim());

  LoopState ls(1, input_dim, hidden_dim);
  ls.temp_init(time_steps, state);

  MatrixXf delta_y = state.O.Xf() - Y;

  for(int time_step = time_steps - 1; time_step >= 0; time_step--) {
    grad.V.set(grad.V.Xf() + ls.temp().S.Xf().transpose().eval() * delta_y.row(time_step));
  }

  MatrixXf ds_single = delta_y * m.V.Xf().transpose().eval();
  MMatrix  ds_cur;

  LoopState x_ls(time_steps, input_dim, hidden_dim);

  MMatrix x_ds_cur; x_ds_cur.set(ds_single);

	timers.start("x_step");

	int x_step = time_steps;
	for (int x = 0; x < x_step; ++x) {
		x_ls.x_set_step(x, state, X);
		x_ls.init_drelu(x_ds_cur, m);
		if (x < x_step - 1) {
	  	x_ls.update(x_ds_cur, m);
		}
	}

	timers.stop("x_step");

	Model x_grad = grad;
  x_ls.update_gradient_rows(x_grad);

  for (int cur_step = time_steps - 1; cur_step >= 0; cur_step--) {
    ds_cur.set(ds_single.row(cur_step));

    for (int time_step = cur_step; time_step >= 0; time_step--) {
      //warn << "cur_step, time_step: " << cur_step << ", " << time_step;
      timers.start("back_propagation for inner");             // All processing time here in this loop

      ls.set_step(time_step, state, X);
      ls.init_drelu(ds_cur, m);


      if (time_step == cur_step - (x_step - 1)) {
				timers.start("Loop assert");

				//warn << ds_cur.dump();
				//warn << x_ds_cur.row(cur_step).dump();
				assert(ds_cur.same(x_ds_cur.row(cur_step), Precision));

				//warn << ls.temp().S.dump();
				//warn << x_ls.temp().S.row(cur_step).dump();
				assert(ls.temp().S.same(x_ls.temp().S.row(cur_step)));

				timers.stop("Loop assert");
			}

      ls.update(ds_cur, m);
      ls.update_gradient(grad);


      if (time_step == cur_step - (x_step - 1)) {
				timers.start("Loop assert");

        assert(ls.dreluInput_h.same(x_ls.dreluInput_h.row(cur_step), Precision));
        assert(ls.dreluInput_r.same(x_ls.dreluInput_r.row(cur_step), Precision));
        assert(ls.dreluInput_z.same(x_ls.dreluInput_z.row(cur_step), Precision));

				timers.stop("Loop assert");
      }

      timers.stop("back_propagation for inner");
    }
  }

  // This is a cumulative sum. You can only compare if the entire double loop is done.
  //
  //MMatrix x_U_h(grad.U_h);
  // x_U_h.outer_add_rows(x_X, x_dreluInput_h);
  //warn << "x_U_h: " << x_U_h.dump_dim();
	//
	// Test new loop:
	//assert(x_grad.U_h.same(grad.U_h));

  grad.grad_div_steps((float) time_steps);

  timers.stop("back_propagation");
}




void rms_prop(Model &m, Model &grad, Model &cache, float learning_rate, int input_dim, int hidden_dim, int output_dim) {
    float decay = 0.9f;

    Model grad_total;
    grad_total.init_zeroes(m.input_dim(), m.hidden_dim(), m.output_dim());

    cache.cache_decay(decay, grad);
    grad_total.divide(grad, cache);
    m.adjust_learning_rate(learning_rate, grad_total);
    grad.init_zeroes(m.input_dim(), m.hidden_dim(), m.output_dim(), true);

    m.eval();
}


void gradient_descent(Model &m, Model &grad, float learning_rate) {
  m.adjust_learning_rate(learning_rate, grad);
  m.eval();
  grad.init_zeroes(m.input_dim(), m.hidden_dim(), m.output_dim(), true);
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

void read_x_y(MatrixXf& x, MatrixXf& y, std::string filename_input, std::string filename_output, int time_steps, int pos) {
  //timers.start("read_x_y");
/*
  warn << "Doing read_x_y() input\n"
       << "  time_steps: " << time_steps << "\n"
       << "  pos       : " << pos
  ;
*/
  int count = 0;

  if (x_input.empty()) {
    //warn << "read_x_y() read input file";

    filename_input.replace(filename_input.end() - 3, filename_input.end(), "bin");
    std::ifstream file_input(filename_input, std::ios::binary);
    assert(!file_input.fail());

    file_input.seekg(pos * sizeof(int), std::ios::beg);
    uint32_t a = 0;

    while(!file_input.eof()) {
        file_input.read((char*)&a, sizeof(uint32_t));
        x_input.push_back((int) a);
    }

    x.eval();

    file_input.close();
  }

    while((pos + count) < (int) x_input.size() && count < time_steps) {
      int index = pos + count;
        x(count, x_input[index]) = 1;
        count++;
    }

    x.eval();

  if (x_output.empty()) {
    //warn << "read_x_y() read output file";

    filename_output.replace(filename_output.end() - 3, filename_output.end(), "bin");
    std::ifstream file_output(filename_output, std::ios::binary);

    uint32_t a = 0;

    file_output.seekg(pos * sizeof(int), std::ios::beg);
    a = 0;

    while(!file_output.eof()) {
        count++;
        file_output.read((char*)&a, sizeof(uint32_t));
        x_output.push_back((int) a);
    }

    y.eval();

    file_output.close();
  }

  count = 0;

  while((pos + count) < (int) x_input.size() && count < time_steps) {
    int index = pos + count;
    y(count, x_output[index]) = 1;
    count++;
  }

  //timers.stop("read_x_y");
}


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
  grad.init_zeroes(m.input_dim(), m.hidden_dim(), m.output_dim(), true);
  cache.init_ones(m.input_dim(), m.hidden_dim(), m.output_dim());

  for(int epoch = 0; epoch < nepoch; epoch++) {
    warn << "train loop epoch: " << epoch << ", limit: " << limit;

    float loss = 0;
    for(int i = 0; i < limit; i++) {
      if (i >= 10) break; // DEBUG
      warn << "train loop i: " << i;

      timers.start("train limit loop");

      State state;
      state.init(time_steps, hidden_dim, output_dim);

      MatrixXf currX  = MatrixXf::Zero(time_steps, input_dim);
      MatrixXf currY  = MatrixXf::Zero(time_steps, output_dim);

      read_x_y(currX, currY, filename_input, filename_output, time_steps, i);

      state.eval();
      currX.eval();
      currY.eval();

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

  train(base + "/donald_trump_input.txt", base + "/donald_trump_output.txt", learning_rate, nepochs, input_dim, hidden_dim, output_dim, time_steps, decay);
}
