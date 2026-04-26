/**
 * Reference project for implementing LSTM
 *
 * Source: https://github.com/anudeepadi/lstmcpp
 */
#include "LSTMNetwork.h"
#include <iostream>


/**
 * @brief Main function to demonstrate the LSTM network
 */
int main() {
  // Example parameters based on the blog post
  int input_size      = 1;
  int hidden_size     = 32;    // As mentioned in the blog
  int output_size     = 1;
  float learning_rate = 0.01f; // As mentioned in the blog
  int epochs          = 50;    // As mentioned in the blog
    
  DataSamples training_data;
	training_data.init();
    
  // Create and train the LSTM network
  LSTMNetwork lstm(input_size, hidden_size, output_size, learning_rate, epochs);
  float final_loss = lstm.train(training_data);
    
  std::cout << "Training complete. Final loss: " << final_loss << std::endl;
    
  // Test predictions
  std::cout << "Testing predictions:" << std::endl;
  for (int i = 0; i < 10; i++) {
    float x = (float) i * 0.1f;
    vector vec;
    vec.push_back(x);
    float actual = (float) sin(x);
    float predicted = lstm.predict(vec);

    std::cout << "Input: " << x << ", Actual: " << actual << ", "
              << "Predicted: " << predicted << std::endl;
  }

	warn << "QPU Call Count: " << BaseKernel::qpu_call_count();
    
  return 0;
}
