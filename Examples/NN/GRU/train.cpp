#include "model.h"

using namespace Log;

namespace {

const float Precision = 1.0e-7f;	

MAYBE_UNUSED std::string dump(MatrixXf const &m) {
  std::string buf;
  buf << dump_dim(m) << " [\n";

  for (int i = 0; i < m.rows(); ++i) {
    for (int j = 0; j < m.cols(); ++j) {
      buf << "  " << m(i,j) << ", ";
    }
    buf << "\n";
  }
  buf << "]";

  return buf;
}


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


float calculate_cost(MatrixXf& E, int time_steps) {
    /* Possibly - Move to forward propagation or Train */
    return (E.sum() / (float) time_steps);
}

std::vector<int> x_input;
std::vector<int> x_output;

} // anon namespace


void back_propagation(Model &m, Model &grad, State &state, MatrixXf& X, MatrixXf& Y, int input_dim, int hidden_dim, int output_dim, int time_steps) {
	timers.start("back_propagation");

  /* gradients = dLdV, dLdU0, dLdU1, dLdU2, dLdW0, dLdW1, dLdW2 */
  int time_step, curr_step;
	grad.init_zeroes(m.input_dim(), m.hidden_dim(), m.output_dim());

	MatrixXf ds_0, ds_single;
 	MMatrix	ds_cur;
  MMatrix	ds_cur_bk;
  MatrixXf	dz, delta_y, db_V, dreluInput_z;
	MMatrix dreluInput_r;
	MMatrix dreluInput_h;
	MMatrix temp_X;
	MMatrix temp_W;
	MatrixXf temp_U, temp_V;

	State temp(true);
	temp.init(1, hidden_dim, -1);

	temp_X.init_zeroes(input_dim);

  ds_cur_bk.set(MatrixXf::Zero(1, hidden_dim));

  delta_y = state.O - Y;

  for(time_step = time_steps - 1; time_step >= 0; time_step--) {
    temp.S.set(state.S.row(time_step + 1));
    grad.V += temp.S.Xf().transpose().eval() * delta_y.row(time_step);
  }

  ds_single = delta_y * m.V.transpose().eval();

	auto ones = MatrixXf::Ones(1, hidden_dim);
	auto q_ones = copy(ones);
	//grad.q_U_r = copy_m(grad.U_r);

  for(curr_step = time_steps - 1; curr_step >= 0; curr_step--) {
    ds_cur.set(ds_single.row(curr_step));

    for(time_step = curr_step; time_step >= 0; time_step--) {
			timers.start("back_propagation for inner");

      ds_cur_bk = ds_cur;
			temp.set_step(time_step, state);
			temp_X.set(X.row(time_step));

			timers.start("back_propagation converted");

		  dreluInput_h.back_prop_1(ds_cur, temp);
      grad.U_h += temp_X.outer(dreluInput_h);      //OK assert(grad.U_h.same());

			temp_W.back_prop_2(temp, dreluInput_h);
      grad.W_h += temp_W;                          //OK assert(grad.W_h.same());

			auto dsr = dreluInput_h * m.W_h;
      ds_cur.mul_e(dsr, temp);
      dreluInput_r.back_prop_3(dsr, temp);         //OK assert(dreluInput_r.same(Precision));

      grad.U_r += temp_X.outer(dreluInput_r);      //OK assert(grad.U_r.same(Precision));

      temp_W = temp.S.outer(dreluInput_r);
      grad.W_r += temp_W;

      ds_cur += dreluInput_r * m.W_r;

      //Original: ds_cur.set(ds_cur.Xf() + ds_cur_bk.Xf().cwiseProduct(temp.z.Xf()));
      ds_cur += ds_cur_bk.mul_e(temp.z);

			timers.stop("back_propagation converted");

			///////////////////////////////////////

			timers.start("back_propagation rest");

			//assert(tmp_ds_cur.same(ds_cur, Precision));

      dz = ds_cur_bk.Xf().cwiseProduct(temp.S.Xf() - temp.h);
      dreluInput_z = dz.cwiseProduct(temp.z.Xf().unaryExpr(&sigmoid_grad));

      temp_U = (temp_X.Xf().transpose().eval() * dreluInput_z);
      grad.U_z = grad.U_z + temp_U;

      temp_W.Xf((temp.S.Xf().transpose().eval() * dreluInput_z));
      grad.W_z = grad.W_z + temp_W.Xf();

      ds_cur.set(ds_cur.Xf() + (dreluInput_z * m.W_z.transpose().eval()));

			timers.stop("back_propagation rest");
			timers.stop("back_propagation for inner");
    }
  }

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
	timers.start("read_x_y");
/*
	warn << "Doing read_x_y() input\n"
		   << "  time_steps: " << time_steps << "\n"
		   << "  pos       : " << pos
	;
*/
    int count = 0;


	if (x_input.empty()) {
		warn << "read_x_y() read input file";

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

	//warn << "Done read_x_y() input";

	if (x_output.empty()) {
		warn << "read_x_y() read output file";

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

	timers.stop("read_x_y");
}


void train(std::string filename_input, std::string filename_output, float learning_rate, int nepoch, int input_dim, int hidden_dim, int output_dim, int time_steps, float decay) {
    float prev_loss = 0.0f;

    int inputSize   = get_input_size(filename_input) - time_steps - 1;
    int limit       = inputSize / 50;
    float min_loss  = 99999999.0f;
    std::cout << "Size of input file is : " << inputSize << std::endl;

		Model m;
		Model grad_batch;  // Unused, only initialized
		Model grad;
		Model cache;

		m.init(input_dim, hidden_dim, output_dim);
		grad_batch.init_zeroes(m.input_dim(), m.hidden_dim(), m.output_dim());
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

		    grad_batch.init_zeroes(m.input_dim(), m.hidden_dim(), m.output_dim());
        std::cout << "Loss: " << loss << ", Epoch: "<< epoch << std::endl;

        if(loss > prev_loss && prev_loss != 0) {
            learning_rate = learning_rate * 1;
            std::cout << "Adjusting learning rate to " << learning_rate << std::endl;
        }
        prev_loss = loss;
        learning_rate *= 1 / (1 + (decay * (float) epoch));

        if(loss < min_loss){
            min_loss = loss;
        }
    }
}


void train_main() {
	warn << "Called train_main()";

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
