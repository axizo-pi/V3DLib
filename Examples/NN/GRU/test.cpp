#include "model.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <climits>
#include <cmath>
#include <math.h>
#include <vector>
#include <assert.h>
#include <map>


int get_max_index(MatrixXf& O, std::vector<int> predictions, int time_steps) {
  int max_index;
  int index = time_steps - 1;
  O.row(index).maxCoeff(&max_index);

  return max_index;
}


void predict(Model &m, int input_dim, int output_dim, int hidden_dim, int time_steps) {
  MatrixXf X, Y, U_z_grad, U_r_grad, U_h_grad, W_z_grad, W_r_grad, W_h_grad, V_grad;

  State state;
  state.init(time_steps, hidden_dim, output_dim);

  X = MatrixXf::Zero(time_steps, input_dim);
  Y = MatrixXf::Zero(1, output_dim);

  state.eval();

  int max_index;
  int start_count;
/*    
  char mapping_57[] = {' ', '!', '"', '&', '^', '(', ')', '+', ',', '.', '/', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '?',
      'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '*', '-', '_', '`', '~', '@', '#'};
  char mapping_27[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', ' '};
  char mapping_39[] = { '\n', ' ', '!', '$', '&', '"', ',', '-', '.', '3', ':', ';', '?', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z' };
*/    
  char mapping_64[] = {'\n', ' ', '!', '"', '#', '$', '%', '&', '`', '(', ')', '+', ',', '-', '.', '/', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '=', '?', '@', '[', ']', '_', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~'};

  std::map<char, int> char_to_int;

  for (int i = 0; i < 63; ++i) {
    char_to_int.insert(std::pair<char, int>(mapping_64[i], i));
  }

  std::string input = "i am the best";
  std::cout << "Input length = " << input.length() << std::endl;
  assert((int) input.length() == time_steps);

  int i = 0;
  for(auto c : input) {
    X(i, char_to_int.find(c)->second) = 1;
    i++;
  }
  std::cout << std::endl;

  std::vector<int> predictions;

  int count = 250;
  int s;

  X.eval();

  MatrixXf unused = MatrixXf::Zero(time_steps, hidden_dim);

  while (count --) {
    forward_propagation(m, X, unused, state, time_steps, input_dim, hidden_dim, output_dim, true);
    max_index = get_max_index(state.O, predictions, time_steps);
    predictions.push_back(max_index);
    X  = MatrixXf::Zero(time_steps, input_dim);
    X.eval();
    s  = (int) predictions.size();
    start_count = time_steps - 1;

    for (int i = time_steps - 1; i >= 0; i--) {
      if (s) {
        X(i, predictions[s - 1]) = 1;
        s--;
      } else {
        X(i, char_to_int.find(input[start_count])->second) = 1;
        start_count--;
      }
    }
  }
  std::cout << "Predictions starting with : \n" << input << std::endl;

  for(auto p : predictions){
    std::cout << mapping_64[p];
  }
  std::cout << std::endl;
}


void test_main(std::string const &epoch, std::string const &loss) {
  int time_steps = 13;

  Model m;
  m.read(epoch, loss);

  int input_dim  = (int) m.U_z.rows();
  int hidden_dim = (int) m.U_z.cols();
  int output_dim = (int) m.V.cols();
  //std::cout << input_dim << " " << hidden_dim << " " << output_dim << std::endl;

  predict(m, input_dim, output_dim, hidden_dim, time_steps);
}
