#include "LSTMNetwork.h"
#include <iostream>


LSTMNetwork::LSTMNetwork(int input_size, int hidden_size, int output_size, 
                         float learning_rate, int epochs) :
  input_size(input_size), 
  hidden_size(hidden_size),
  output_size(output_size),
  lstm_cell(input_size, hidden_size),
  learning_rate(learning_rate),
  epochs(epochs) 
{
  // Initialize output layer
	normal_rand gen;
        
  Wy = matrix(output_size, hidden_size);
  by = vector(output_size, 0.0);
        
  for (int i = 0; i < output_size; i++) {
    for (int j = 0; j < hidden_size; j++) {
      Wy[i][j] = gen.rand();
    }
  }
}


float LSTMNetwork::train(DataSamples &training_data) {
  float total_loss = 0.0;
        
  for (int epoch = 0; epoch < epochs; epoch++) {
    float epoch_loss = 0.0;
            
    for (const auto& sample : training_data) {
      // Initialize hidden state and cell state
      vector h(hidden_size, 0.0);
      vector c(hidden_size, 0.0);
                
      // Forward pass through the LSTM cell
      tie(h, c) = lstm_cell.forward(sample.features, h, c);
                
      // Output layer
      y_pred = Wy*h;
      for (int i = 0; i < output_size; i++) {
        y_pred[i] += by[i];
      }
                
      // Compute loss (Mean squared error for simplicity)
      float loss = 0.0;
      vector dy(output_size, 0.0);
      for (int i = 0; i < output_size; i++) {
        float error = y_pred[i] - sample.target;
        loss += error * error;
        dy[i] = 2.0f * error; // Derivative of MSE
      }
      loss /= (float) output_size;
      epoch_loss += loss;
                
      // Backpropagation
                
      // Output layer gradients
      vector dh(hidden_size, 0.0f);
      for (int i = 0; i < output_size; i++) {
        for (int j = 0; j < hidden_size; j++) {
          Wy[i][j] -= learning_rate * dy[i] * h[j];
          dh[j] += Wy[i][j] * dy[i];
        }
        by[i] -= learning_rate * dy[i];
      }
                
      // LSTM cell backpropagation
      vector dc(hidden_size, 0.0);
      lstm_cell.backward(dh, dc, learning_rate);
    }
            
    epoch_loss /= (float) training_data.size();
    total_loss = epoch_loss;
            
    // Print progress
    if ((epoch + 1) % 10 == 0 || epoch == 0) {
      std::cout << "Epoch " << (epoch + 1) << "/" << epochs 
                << ", Loss: " << epoch_loss << std::endl;
    }
  }
        
  return total_loss;
}


float LSTMNetwork::predict(const vector & features) {
  // Initialize hidden state and cell state
  vector h(hidden_size, 0.0);
  vector c(hidden_size, 0.0);
        
  // Forward pass
  tie(h, c) = lstm_cell.forward(features, h, c);
        
  // Output layer
  vector output = Wy*h;
  for (int i = 0; i < output_size; i++) {
    output[i] += by[i];
  }
        
  // For simplicity, return the first output (assuming single output)
  return output[0];
}

