#include "model.h"
#include "global/log.h"
#include "Support/basics.h"
#include "../Lib/matrix.h"
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <climits>
#include <cmath>
#include <math.h>
#include <vector>

using namespace Log;

std::string dump(MatrixXf const &m) {
		std::string buf;
		buf << "(rows,cols) = (" << m.rows() << ", " << m.cols() << ") [\n";

		for (int i = 0; i < m.rows(); ++i) {
			for (int j = 0; j < m.cols(); ++j) {
				buf << "  " << m(i,j) << ", ";
			}
			buf << "\n";
		}
		buf << "]";

	return buf;
}


qpu::vector copy(MatrixXf const &rhs) {
	assert(rhs.rows() == 1);
	assert(rhs.cols() % 16 == 0);

  //int height = resize(rhs.size());
  //auto width = resize(rhs[0].size());
  int width = (int) rhs.cols();

  qpu::vector ret(width);
  ret.set(0.0f);

  for (int i = 0; i < rhs.cols(); i++) {
  	ret.at(0, i) = rhs(0, i);
  }

  return ret;
}


bool same(qpu::vector const &lhs, MatrixXf const &rhs, float precision = 0.0f) {
	assert(rhs.rows() == 1);

  for (int i = 0; i < (int) rhs.cols(); ++i) {
    if (!qpu::check_precision(lhs[i], rhs(0, i), precision)) {
      warn << "Fail same(qpu::vector, MatrixXf), index: " << i;
      return false;
    }      
  }

  return true;
}


MatrixXf loss_softmax_grad(MatrixXf& y, MatrixXf& o) {
    return o - y;
}

void dropout(MatrixXf& m, float p) {
    auto tmp = (float) m.rows() * (float) m.cols() * p;
		int limit = (int) tmp;
    int r, c;
    for (int i = 0; i < limit; ++i) {
        srand((unsigned) clock());
        r = rand() % (int) m.rows();
        c = rand() % (int) m.cols();
        m(r, c) = 0;
    }
}

float div_temp(float x) {
    float temp = 0.5f;
    if(x <= 0)
        x = 0;
    else
        x = log(x) / temp;
    return x;
}


float calculate_cost(MatrixXf& E, int time_steps) {
    /* Possibly - Move to forward propagation or Train */
    return (E.sum() / (float) time_steps);
}

void back_propagation(MatrixXf& V, MatrixXf& W_z, MatrixXf& W_r, MatrixXf& W_h, MatrixXf& dU_z, MatrixXf& dU_r, MatrixXf& dU_h, MatrixXf& dW_z, MatrixXf& dW_r, MatrixXf& dW_h, MatrixXf& dV, MatrixXf& z, MatrixXf& r, MatrixXf& h, MatrixXf& O, MatrixXf& S, MatrixXf& X, MatrixXf& Y, int input_dim, int hidden_dim, int output_dim, int time_steps) {
	timers.start("back_propagation");

    /* gradients = dLdV, dLdU0, dLdU1, dLdU2, dLdW0, dLdW1, dLdW2 */
    int time_step, curr_step;
    dU_z = MatrixXf::Zero(input_dim, hidden_dim);
    dU_r = MatrixXf::Zero(input_dim, hidden_dim);
    dU_h = MatrixXf::Zero(input_dim, hidden_dim);
    dW_z = MatrixXf::Zero(hidden_dim, hidden_dim);
    dW_r = MatrixXf::Zero(hidden_dim, hidden_dim);
    dW_h = MatrixXf::Zero(hidden_dim, hidden_dim);

    MatrixXf ds_0, dsr, ds_single, ds_cur, ds_cur_bk, dz, delta_y, db_V, dreluInput_z, dreluInput_r, dreluInput_h, temp_r, temp_z, temp_h, temp_S, temp_X, temp_W, temp_U, temp_V;
    dsr = MatrixXf::Zero(1, hidden_dim);
    temp_S = MatrixXf::Zero(1, hidden_dim);
    temp_z = MatrixXf::Zero(1, hidden_dim);
    temp_r = MatrixXf::Zero(1, hidden_dim);
    temp_h = MatrixXf::Zero(1, hidden_dim);
    temp_X = MatrixXf::Zero(1, input_dim);
    ds_cur_bk = MatrixXf::Zero(1, hidden_dim);

    delta_y = O - Y;

    dV = MatrixXf::Zero(hidden_dim, output_dim);
    for(time_step = time_steps - 1; time_step >= 0; time_step--) {
        temp_S = S.row(time_step + 1);
        dV += temp_S.transpose().eval() * delta_y.row(time_step);
    }

    ds_single = delta_y * V.transpose().eval();


		auto ones = MatrixXf::Ones(1, hidden_dim);
		auto q_ones = copy(ones);
		//warn << "ones  : " << dump(ones);
		//warn << "q_ones: " << q_ones.dump();

    for(curr_step = time_steps - 1; curr_step >= 0; curr_step--) {
        ds_cur = ds_single.row(curr_step);
        for(time_step = curr_step; time_step >= 0; time_step--) {
						timers.start("back_propagation for inner");

            ds_cur_bk = ds_cur;
            temp_S = S.row(time_step);
            temp_r = r.row(time_step);
            temp_z = z.row(time_step);
            temp_h = h.row(time_step);
            temp_X = X.row(time_step);


						auto q_temp_z = copy(temp_z);
						auto q_temp_h = copy(temp_h);
						auto q_ds_cur = copy(ds_cur);
						auto q_tmp2 = q_ones - q_temp_z;

						timers.start("back_propagation dreluInput_h");
            dreluInput_h = ds_cur.cwiseProduct(ones - temp_z).cwiseProduct(temp_h.unaryExpr(&tanh_grad));//.cwiseProduct(temp_S.unaryExpr(&tanh_grad));
						timers.stop("back_propagation dreluInput_h");

						timers.start("back_propagation q_dreluInput_h");
						auto q_dreluInput_h = q_ds_cur.mul(q_ones - q_temp_z).mul(q_temp_h.dtanh());
						timers.stop("back_propagation q_dreluInput_h");

						//warn << "dreluInput_h: " << dump(dreluInput_h);
						//warn << "q_dreluInput_h: " << q_dreluInput_h.dump();
						assert(same(q_dreluInput_h, dreluInput_h));

            temp_U = (temp_X.transpose().eval() * dreluInput_h);
            dU_h = dU_h + temp_U;

            temp_W = ((temp_S.cwiseProduct(temp_r)).transpose().eval() * dreluInput_h);
            dW_h = dW_h + temp_W;


            dsr = dreluInput_h * W_h.transpose().eval();
            ds_cur = dsr.cwiseProduct(temp_r);
            dreluInput_r = dsr.cwiseProduct(temp_S).cwiseProduct(temp_r.unaryExpr(&sigmoid_grad));

            temp_U = (temp_X.transpose().eval() * dreluInput_r);
            dU_r = dU_r + temp_U;

            temp_W = (temp_S.transpose().eval() * dreluInput_r);
            dW_r = dW_r + temp_W;

            ds_cur = ds_cur + (dreluInput_r * W_r.transpose().eval());
            ds_cur = ds_cur + ds_cur_bk.cwiseProduct(temp_z);
            dz = ds_cur_bk.cwiseProduct(temp_S - temp_h);
            dreluInput_z = dz.cwiseProduct(temp_z.unaryExpr(&sigmoid_grad));

            temp_U = (temp_X.transpose().eval() * dreluInput_z);
            dU_z = dU_z + temp_U;

            temp_W = (temp_S.transpose().eval() * dreluInput_z);
            dW_z = dW_z + temp_W;

            ds_cur = ds_cur + (dreluInput_z * W_z.transpose().eval());

						timers.stop("back_propagation for inner");
        }
    }


		float steps = (float) time_steps;

    dU_z /= steps;
    dU_r /= steps;
    dU_h /= steps;
    dW_z /= steps;
    dW_r /= steps;
    dW_h /= steps;
    dV   /= steps;

    dU_z.eval();
    dU_r.eval();
    dU_h.eval();
    dW_z.eval();
    dW_r.eval();
    dW_h.eval();
    dV.eval();

	timers.stop("back_propagation");
}

void divide_matrix(MatrixXf& gradient_total, MatrixXf gradient, MatrixXf cache) {

    for (int i = 0; i < gradient_total.rows(); ++i)
    {
        for (int j = 0; j < gradient_total.cols(); ++j)
        {
            gradient_total(i, j) = ( gradient(i, j) / ( sqrt(cache(i, j)) + 0.00000001f) );
        }
    }
}

void rms_prop(MatrixXf& U_z, MatrixXf& U_r, MatrixXf& U_h, MatrixXf& W_z, MatrixXf& W_r, MatrixXf& W_h, MatrixXf& V, MatrixXf& U_z_grad, MatrixXf& U_r_grad, MatrixXf& U_h_grad, MatrixXf& W_z_grad, MatrixXf& W_r_grad, MatrixXf& W_h_grad, MatrixXf& V_grad, MatrixXf& cache_U_z, MatrixXf& cache_U_r, MatrixXf& cache_U_h, MatrixXf& cache_W_z, MatrixXf& cache_W_r, MatrixXf& cache_W_h, MatrixXf& cache_V, float learning_rate, int input_dim, int hidden_dim, int output_dim) {

    float decay = 0.9f;

    MatrixXf U_z_grad_total = MatrixXf::Zero(input_dim, hidden_dim);
    MatrixXf U_r_grad_total = MatrixXf::Zero(input_dim, hidden_dim);
    MatrixXf U_h_grad_total = MatrixXf::Zero(input_dim, hidden_dim);
    MatrixXf W_z_grad_total = MatrixXf::Zero(hidden_dim, hidden_dim);
    MatrixXf W_r_grad_total = MatrixXf::Zero(hidden_dim, hidden_dim);
    MatrixXf W_h_grad_total = MatrixXf::Zero(hidden_dim, hidden_dim);
    MatrixXf V_grad_total   = MatrixXf::Zero(hidden_dim, output_dim);

    cache_U_z = decay * cache_U_z + (1 - decay) * (U_z_grad.cwiseProduct(U_z_grad)).eval();
    cache_U_r = decay * cache_U_r + (1 - decay) * (U_r_grad.cwiseProduct(U_r_grad)).eval();
    cache_U_h = decay * cache_U_h + (1 - decay) * (U_h_grad.cwiseProduct(U_h_grad)).eval();
    cache_W_z = decay * cache_W_z + (1 - decay) * (W_z_grad.cwiseProduct(W_z_grad)).eval();
    cache_W_r = decay * cache_W_r + (1 - decay) * (W_r_grad.cwiseProduct(W_r_grad)).eval();
    cache_W_h = decay * cache_W_h + (1 - decay) * (W_h_grad.cwiseProduct(W_h_grad)).eval();
    cache_V   = decay * cache_V   + (1 - decay) * (V_grad.cwiseProduct(V_grad)).eval();

    cache_U_z.eval();
    cache_U_r.eval();
    cache_U_h.eval();
    cache_W_z.eval();
    cache_W_r.eval();
    cache_W_h.eval();
    cache_V.eval();

    divide_matrix(U_z_grad_total, U_z_grad, cache_U_z);
    divide_matrix(U_r_grad_total, U_r_grad, cache_U_r);
    divide_matrix(U_h_grad_total, U_h_grad, cache_U_h);
    divide_matrix(W_z_grad_total, W_z_grad, cache_W_z);
    divide_matrix(W_r_grad_total, W_r_grad, cache_W_r);
    divide_matrix(W_h_grad_total, W_h_grad, cache_W_h);
    divide_matrix(V_grad_total, V_grad, cache_V);

    U_z_grad_total.eval();
    U_r_grad_total.eval();
    U_h_grad_total.eval();
    W_z_grad_total.eval();
    W_r_grad_total.eval();
    W_h_grad_total.eval();
    V_grad_total.eval();

    U_z -= learning_rate * U_z_grad_total;
    U_r -= learning_rate * U_r_grad_total;
    U_h -= learning_rate * U_h_grad_total;
    W_z -= learning_rate * W_z_grad_total;
    W_r -= learning_rate * W_r_grad_total;
    W_h -= learning_rate * W_h_grad_total;
    V   -= learning_rate * V_grad_total;

    U_z_grad  = MatrixXf::Zero(input_dim, hidden_dim);
    U_r_grad  = MatrixXf::Zero(input_dim, hidden_dim);
    U_h_grad  = MatrixXf::Zero(input_dim, hidden_dim);
    W_z_grad  = MatrixXf::Zero(hidden_dim, hidden_dim);
    W_r_grad  = MatrixXf::Zero(hidden_dim, hidden_dim);
    W_h_grad  = MatrixXf::Zero(hidden_dim, hidden_dim);
    V_grad    = MatrixXf::Zero(hidden_dim, output_dim);

    U_z_grad.eval();
    U_r_grad.eval();
    U_h_grad.eval();
    W_z_grad.eval();
    W_r_grad.eval();
    W_h_grad.eval();
    V_grad.eval();

    U_z.eval();
    U_r.eval();
    U_h.eval();
    W_z.eval();
    W_r.eval();
    W_h.eval();
    V.eval();
}


void gradient_descent(MatrixXf& U_z, MatrixXf& U_r, MatrixXf& U_h, MatrixXf& W_z, MatrixXf& W_r, MatrixXf& W_h, MatrixXf& V,
            MatrixXf& U_z_grad, MatrixXf& U_r_grad, MatrixXf& U_h_grad, MatrixXf& W_z_grad, MatrixXf& W_r_grad, MatrixXf& W_h_grad, MatrixXf& V_grad,
            float learning_rate, int input_dim, int hidden_dim, int output_dim) {

    V   -= learning_rate * V_grad;
    U_z -= learning_rate * U_z_grad;
    U_r -= learning_rate * U_r_grad;
    U_h -= learning_rate * U_h_grad;
    W_z -= learning_rate * W_z_grad;
    W_r -= learning_rate * W_r_grad;
    W_h -= learning_rate * W_h_grad;

    U_z.eval();
    U_r.eval();
    U_h.eval();
    W_z.eval();
    W_r.eval();
    W_h.eval();
    V.eval();

    U_z_grad  = MatrixXf::Zero(input_dim, hidden_dim);
    U_r_grad  = MatrixXf::Zero(input_dim, hidden_dim);
    U_h_grad  = MatrixXf::Zero(input_dim, hidden_dim);
    W_z_grad  = MatrixXf::Zero(hidden_dim, hidden_dim);
    W_r_grad  = MatrixXf::Zero(hidden_dim, hidden_dim);
    W_h_grad  = MatrixXf::Zero(hidden_dim, hidden_dim);
    V_grad    = MatrixXf::Zero(hidden_dim, output_dim);

    U_z_grad.eval();
    U_r_grad.eval();
    U_h_grad.eval();
    W_z_grad.eval();
    W_r_grad.eval();
    W_h_grad.eval();
    V_grad.eval();
}

void init_matrix(MatrixXf& X, int dimension_row, int dimension_col) {
    float upperlimit =  1.0f * sqrt(1.0f / (float) dimension_row);
    float lowerlimit = -1.0f * sqrt(1.0f / (float) dimension_row);
    float range = upperlimit - lowerlimit;

    srand((unsigned) clock());
    X = MatrixXf::Random(dimension_row, dimension_col);
    X = (X + MatrixXf::Constant(dimension_row, dimension_col, 1.0)) * (range / 2.0);
    X = (X + MatrixXf::Constant(dimension_row, dimension_col, lowerlimit));
}

void init_weight_matrices(MatrixXf& U_z, MatrixXf& U_r, MatrixXf& U_h, MatrixXf& W_z, MatrixXf& W_r, MatrixXf& W_h, MatrixXf& V, int input_dim, int output_dim, int hidden_dim) {
    init_matrix(U_z, input_dim, hidden_dim);
    init_matrix(U_r, input_dim, hidden_dim);
    init_matrix(U_h, input_dim, hidden_dim);
    init_matrix(W_z, hidden_dim, hidden_dim);
    init_matrix(W_r, hidden_dim, hidden_dim);
    init_matrix(W_h, hidden_dim, hidden_dim);
    init_matrix(V, hidden_dim, output_dim);

    U_z.eval();
    U_r.eval();
    U_h.eval();
    W_z.eval();
    W_r.eval();
    W_h.eval();
    V.eval();
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

    MatrixXf U_z, U_r, U_h, W_z, W_r, W_h, V, U_z_grad_batch, U_r_grad_batch, U_h_grad_batch, W_r_grad_batch, W_h_grad_batch, W_z_grad_batch, V_grad_batch;

		warn << "Doing init_weight_matrices()\n"
			"  input_dim : " << input_dim  << "\n"
			"  hidden_dim: " << hidden_dim << "\n"
			"  output_dim: " << output_dim
		;

    init_weight_matrices(U_z, U_r, U_h, W_z, W_r, W_h, V, input_dim, output_dim, hidden_dim);

		warn << "Done init_weight_matrices()";

    U_z_grad_batch = MatrixXf::Zero(input_dim, hidden_dim);
    U_r_grad_batch = MatrixXf::Zero(input_dim, hidden_dim);
    U_h_grad_batch = MatrixXf::Zero(input_dim, hidden_dim);
    W_z_grad_batch = MatrixXf::Zero(hidden_dim, hidden_dim);
    W_r_grad_batch = MatrixXf::Zero(hidden_dim, hidden_dim);
    W_h_grad_batch = MatrixXf::Zero(hidden_dim, hidden_dim);
    V_grad_batch   = MatrixXf::Zero(hidden_dim, output_dim);

    MatrixXf U_z_grad = MatrixXf::Zero(input_dim, hidden_dim);
    MatrixXf U_r_grad = MatrixXf::Zero(input_dim, hidden_dim);
    MatrixXf U_h_grad = MatrixXf::Zero(input_dim, hidden_dim);
    MatrixXf W_z_grad = MatrixXf::Zero(hidden_dim, hidden_dim);
    MatrixXf W_r_grad = MatrixXf::Zero(hidden_dim, hidden_dim);
    MatrixXf W_h_grad = MatrixXf::Zero(hidden_dim, hidden_dim);
    MatrixXf V_grad   = MatrixXf::Zero(hidden_dim, output_dim);

    U_z_grad.eval();
    U_r_grad.eval();
    U_h_grad.eval();
    W_z_grad.eval();
    W_r_grad.eval();
    W_h_grad.eval();
    V_grad.eval();

    MatrixXf cache_U_z = MatrixXf::Ones(input_dim, hidden_dim);
    MatrixXf cache_U_r = MatrixXf::Ones(input_dim, hidden_dim);
    MatrixXf cache_U_h = MatrixXf::Ones(input_dim, hidden_dim);
    MatrixXf cache_W_z = MatrixXf::Ones(hidden_dim, hidden_dim);
    MatrixXf cache_W_r = MatrixXf::Ones(hidden_dim, hidden_dim);
    MatrixXf cache_W_h = MatrixXf::Ones(hidden_dim, hidden_dim);
    MatrixXf cache_V   = MatrixXf::Ones(hidden_dim, output_dim);

    cache_U_z.eval();
    cache_U_r.eval();
    cache_U_h.eval();
    cache_W_r.eval();
    cache_W_z.eval();
    cache_V.eval();

    for(int epoch = 0; epoch < nepoch; epoch++) {
				warn << "train loop epoch: " << epoch << ", limit: " << limit;

        float loss = 0;
        for(int i = 0; i < limit; i++) {
						if (i >= 30) break; // DEBUG
						warn << "train loop i: " << i;

						timers.start("train limit loop");

            // std::cout << i << std::endl;
            MatrixXf E      = MatrixXf::Zero(1, time_steps);
            MatrixXf z      = MatrixXf::Zero(time_steps, hidden_dim);
            MatrixXf r      = MatrixXf::Zero(time_steps, hidden_dim);
            MatrixXf h      = MatrixXf::Zero(time_steps, hidden_dim);
            MatrixXf O      = MatrixXf::Zero(time_steps, output_dim);
            MatrixXf S      = MatrixXf::Zero(time_steps + 1, hidden_dim);
            S(0, 0)         = static_cast <float> (((float) rand()) / (static_cast <float> (RAND_MAX / 2)) - 1);
            MatrixXf currX  = MatrixXf::Zero(time_steps, input_dim);
            MatrixXf currY  = MatrixXf::Zero(time_steps, output_dim);

            read_x_y(currX, currY, filename_input, filename_output, time_steps, i);

            E.eval();
            z.eval();
            r.eval();
            h.eval();
            O.eval();
            S.eval();
            currX.eval();
            currY.eval();

            forward_propagation(U_z, U_r, U_h, W_z, W_r, W_h, V, currX, currY, O, S, E, z, r, h, time_steps, input_dim, hidden_dim, output_dim);
            loss += (calculate_cost(E, time_steps) / (float) limit);

            back_propagation(V, W_z, W_r, W_h, U_z_grad, U_r_grad, U_h_grad, W_z_grad, W_r_grad, W_h_grad, V_grad, z, r, h, O, S, currX, currY, input_dim, hidden_dim, output_dim, time_steps);

            // gradient_descent(U_z, U_r, U_h, W_z, W_r, W_h, V, U_z_grad, U_r_grad, U_h_grad, W_z_grad, W_r_grad, W_h_grad, V_grad, learning_rate, input_dim, hidden_dim, output_dim);

            rms_prop(U_z, U_r, U_h, W_z, W_r, W_h, V, U_z_grad, U_r_grad, U_h_grad, W_z_grad, W_r_grad, W_h_grad, V_grad, cache_U_z, cache_U_r, cache_U_h, cache_W_z, cache_W_r, cache_W_h, cache_V, learning_rate, input_dim, hidden_dim, output_dim);

						timers.stop("train limit loop");
        }

        U_z_grad_batch = MatrixXf::Zero(input_dim, hidden_dim);
        U_r_grad_batch = MatrixXf::Zero(input_dim, hidden_dim);
        U_h_grad_batch = MatrixXf::Zero(input_dim, hidden_dim);
        W_z_grad_batch = MatrixXf::Zero(hidden_dim, hidden_dim);
        W_r_grad_batch = MatrixXf::Zero(hidden_dim, hidden_dim);
        W_h_grad_batch = MatrixXf::Zero(hidden_dim, hidden_dim);
        V_grad_batch   = MatrixXf::Zero(hidden_dim, output_dim);

        std::cout << "Loss: " << loss << ", Epoch: "<< epoch << std::endl;

        if(loss > prev_loss && prev_loss != 0) {
            learning_rate = learning_rate * 1;
            std::cout << "Adjusting learning rate to " << learning_rate << std::endl;
        }
        prev_loss = loss;
        learning_rate *= 1 / (1 + (decay * (float) epoch));

        if(loss < min_loss){
            min_loss = loss;
            std::cout << "Writing weights to file. " << std::endl;
            write_binary_matrix("Weights/Uz_epoch_" + std::to_string(epoch) + "_loss_" + std::to_string(loss) + ".bin", U_z);
            write_binary_matrix("Weights/Uh_epoch_" + std::to_string(epoch) + "_loss_" + std::to_string(loss) + ".bin", U_h);
            write_binary_matrix("Weights/Ur_epoch_" + std::to_string(epoch) + "_loss_" + std::to_string(loss) + ".bin", U_r);
            write_binary_matrix("Weights/Wz_epoch_" + std::to_string(epoch) + "_loss_" + std::to_string(loss) + ".bin", W_z);
            write_binary_matrix("Weights/Wh_epoch_" + std::to_string(epoch) + "_loss_" + std::to_string(loss) + ".bin", W_h);
            write_binary_matrix("Weights/Wr_epoch_" + std::to_string(epoch) + "_loss_" + std::to_string(loss) + ".bin", W_r);
            write_binary_matrix("Weights/V_epoch_"  + std::to_string(epoch) + "_loss_" + std::to_string(loss) + ".bin", V);
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
