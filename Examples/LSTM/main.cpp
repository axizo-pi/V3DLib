/**
 * Reference project for implementing LSTM
 *
 * Source: https://github.com/anudeepadi/lstmcpp
 */
#include "scalar.h"
#include "LSTMCell.h"
#include <iostream>
#include <algorithm>


// Structure for data samples as mentioned in the blog
struct DataSample {
  vector features;
  float target;
};

class DataSamples : public std::vector<DataSample> {};


/**
 * @brief The LSTM Network class
 */
class LSTMNetwork {
private:
  int input_size;
  int hidden_size;
  int output_size;
  LSTMCell lstm_cell;
    
  // Output layer weights and biases
  matrix Wy;
  vector by;
    
  // Learning parameters
  float learning_rate;
  int epochs;
    
  // For backpropagation
  vector y_pred;

public:
    LSTMNetwork(int input_size, int hidden_size, int output_size, 
               float learning_rate = 0.01f, int epochs = 50) :
        input_size(input_size), 
        hidden_size(hidden_size),
        output_size(output_size),
        lstm_cell(input_size, hidden_size),
        learning_rate(learning_rate),
        epochs(epochs) {
        
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
    
    float train(DataSamples &training_data) {
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
    
    float predict(const vector & features) {
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
};

// Main function to demonstrate the LSTM network
int main() {
    // Example parameters based on the blog post
    int input_size      = 1;
    int hidden_size     = 32;    // As mentioned in the blog
    int output_size     = 1;
    float learning_rate = 0.01f; // As mentioned in the blog
    int epochs          = 50;    // As mentioned in the blog
    
    // Create a simple sine wave dataset for demonstration
    DataSamples training_data;
    for (int i = 0; i < 100; i++) {
        float x = (float) i * 0.1f;
        DataSample sample;
        vector vec;
        vec.push_back(x);
        sample.features = vec;
        sample.target = (float) sin(x);
        training_data.push_back(sample);
    }
    
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

        std::cout << "Input: " << x << ", Actual: " << actual
                  << ", Predicted: " << predicted << std::endl;
    }
    
    return 0;
}
