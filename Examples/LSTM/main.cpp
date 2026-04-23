/**
 * Reference project for implementing LSTM
 *
 * Source: https://github.com/anudeepadi/lstmcpp
 */
#include <vector>
#include <cmath>
#include <random>
#include <iostream>
#include <algorithm>
#include <tuple>

//using namespace std;

class vector : public std::vector<float> {
	using Parent = std::vector<float>;

public:
  vector() = default;
  vector(vector const &val) = default;
  vector(long unsigned size) : Parent(size) {}
  vector(long unsigned size, float val) : Parent(size, val) {}
};

// Structure for data samples as mentioned in the blog
struct DataSample {
  //DataSample() = default;

  vector features;
  float target;
};

// Helper functions
float sigmoid(float x) {
    return 1.0f / (1.0f + (float) exp(-x));
}

vector sigmoid(const vector & v) {
    vector result(v.size());
    for (size_t i = 0; i < v.size(); i++) {
        result[i] = sigmoid(v[i]);
    }
    return result;
}

float tanh_custom(float x) {
    return (float) tanh(x);
}

vector tanh_custom(const vector & v) {
    vector result(v.size());
    for (size_t i = 0; i < v.size(); i++) {
        result[i] = (float) tanh(v[i]);
    }
    return result;
}

float dsigmoid(float y) {
    return y * (1.0f - y);
}

vector dsigmoid(const vector & v) {
    vector result(v.size());
    for (size_t i = 0; i < v.size(); i++) {
        result[i] = v[i] * (1.0f - v[i]);
    }
    return result;
}

float dtanh(float y) {
    return 1.0f - y * y;
}

vector dtanh(const vector & v) {
    vector result(v.size());
    for (size_t i = 0; i < v.size(); i++) {
        result[i] = 1.0f - v[i] * v[i];
    }
    return result;
}

// Vector operations
vector operator+(const vector & a, const vector & b) {
    vector result(a.size());
    for (size_t i = 0; i < a.size(); i++) {
        result[i] = a[i] + b[i];
    }
    return result;
}

vector operator*(const vector & a, const vector & b) {
    vector result(a.size());
    for (size_t i = 0; i < a.size(); i++) {
        result[i] = a[i] * b[i];
    }
    return result;
}

vector operator*(float scalar, const vector & v) {
    vector result(v.size());
    for (size_t i = 0; i < v.size(); i++) {
        result[i] = scalar * v[i];
    }
    return result;
}

// Matrix-vector multiplication
vector matmul(const std::vector<vector>& m, const vector & v) {
    vector result(m.size(), 0.0);
    for (size_t i = 0; i < m.size(); i++) {
        for (size_t j = 0; j < v.size(); j++) {
            result[i] += m[i][j] * v[j];
        }
    }
    return result;
}

// Vector concatenation
vector concat(const vector & a, const vector & b) {
    vector result = a;
    result.insert(result.end(), b.begin(), b.end());
    return result;
}

// Gradient clipping as mentioned in the blog
float clip(float value, float min_value, float max_value) {
    return std::max(min_value, std::min(value, max_value));
}

vector clip(const vector & v, float min_value, float max_value) {
    vector result(v.size());
    for (size_t i = 0; i < v.size(); i++) {
        result[i] = clip(v[i], min_value, max_value);
    }
    return result;
}

// LSTM Cell implementation as described in the blog
class LSTMCell {
private:
    int input_size;
    int hidden_size;
    
    // Weights and biases
    std::vector<vector> Wf; // Forget gate weights
    vector bf;         // Forget gate bias
    std::vector<vector> Wi; // Input gate weights
    vector bi;         // Input gate bias
    std::vector<vector> Wc; // Cell state candidate weights
    vector bc;         // Cell state candidate bias
    std::vector<vector> Wo; // Output gate weights
    vector bo;         // Output gate bias
    
    // For backpropagation
    vector x_t;        // Input at time t
    vector h_prev;     // Previous hidden state
    vector c_prev;     // Previous cell state
    vector f_t;        // Forget gate activation
    vector i_t;        // Input gate activation
    vector c_tilde;    // Cell state candidate
    vector c_t;        // Current cell state
    vector o_t;        // Output gate activation
    vector h_t;        // Current hidden state
    
    // Random number generator for weight initialization
		std::random_device rd;
		std::mt19937 gen;
		std::normal_distribution<float> dist;

public:
    LSTMCell(int input_size, int hidden_size) : 
        input_size(input_size), 
        hidden_size(hidden_size),
        gen(rd()),
        dist(0.0f, 0.1f) {
        
        // Initialize weights with small random values as mentioned in the blog
        //float scale = 0.1;
        
        Wf = std::vector<vector>(hidden_size, vector(input_size + hidden_size));
        bf = vector(hidden_size, 0.0f);
        
        Wi = std::vector<vector>(hidden_size, vector(input_size + hidden_size));
        bi = vector(hidden_size, 0.0f);
        
        Wc = std::vector<vector>(hidden_size, vector(input_size + hidden_size));
        bc = vector(hidden_size, 0.0f);
        
        Wo = std::vector<vector>(hidden_size, vector(input_size + hidden_size));
        bo = vector(hidden_size, 0.0f);
        
        // Initialize with random values
        for (int i = 0; i < hidden_size; i++) {
            for (int j = 0; j < input_size + hidden_size; j++) {
                Wf[i][j] = dist(gen);
                Wi[i][j] = dist(gen);
                Wc[i][j] = dist(gen);
                Wo[i][j] = dist(gen);
            }
            // Initialize biases for the forget gate to 1.0 (common practice)
            bf[i] = 1.0;
        }
    }
    
    // Forward pass implementation as described in the blog
		std::pair<vector, vector> forward(const vector & x, 
                                 const vector & h_prev, 
                                 const vector & c_prev) {
        // Store for backpropagation
        this->x_t = x;
        this->h_prev = h_prev;
        this->c_prev = c_prev;
        
        // Concatenate input and previous hidden state
        vector x_h = concat(x, h_prev);
        
        // Forget gate
        f_t = sigmoid(matmul(Wf, x_h) + bf);
        
        // Input gate
        i_t = sigmoid(matmul(Wi, x_h) + bi);
        
        // Cell state candidate
        c_tilde = tanh_custom(matmul(Wc, x_h) + bc);
        
        // Cell state update
        c_t = f_t * c_prev + i_t * c_tilde;
        
        // Output gate
        o_t = sigmoid(matmul(Wo, x_h) + bo);
        
        // Hidden state
        h_t = o_t * tanh_custom(c_t);
        
        return {h_t, c_t};
    }
    
    // Backward pass implementation based on the blog description
		std::tuple<vector, vector, vector> backward(
        const vector & dh_next, 
        const vector & dc_next, 
        float learning_rate, 
        float clip_value = 5.0) {
        
        // Backpropagation through time (BPTT)
        
        // Gradient of the output gate
        vector do_t = dh_next * tanh_custom(c_t) * dsigmoid(o_t);
        
        // Gradient of the cell state
        vector dc_t = dc_next + dh_next * o_t * dtanh(tanh_custom(c_t));
        
        // Gradient of the input gate
        vector di_t = dc_t * c_tilde * dsigmoid(i_t);
        
        // Gradient of the cell state candidate
        vector dc_tilde = dc_t * i_t * dtanh(c_tilde);
        
        // Gradient of the forget gate
        vector df_t = dc_t * c_prev * dsigmoid(f_t);
        
        // Concatenate x and h_prev for weight gradient computation
        vector x_h = concat(x_t, h_prev);
        
        // Gradient clipping to prevent exploding gradients
        do_t = clip(do_t, -clip_value, clip_value);
        di_t = clip(di_t, -clip_value, clip_value);
        dc_tilde = clip(dc_tilde, -clip_value, clip_value);
        df_t = clip(df_t, -clip_value, clip_value);
        
        // Update weights using gradients
        // This is a simple implementation of gradient descent
        
        // Update forget gate weights and biases
        for (int i = 0; i < hidden_size; i++) {
            for (int j = 0; j < input_size + hidden_size; j++) {
                Wf[i][j] -= learning_rate * df_t[i] * x_h[j];
            }
            bf[i] -= learning_rate * df_t[i];
        }
        
        // Update input gate weights and biases
        for (int i = 0; i < hidden_size; i++) {
            for (int j = 0; j < input_size + hidden_size; j++) {
                Wi[i][j] -= learning_rate * di_t[i] * x_h[j];
            }
            bi[i] -= learning_rate * di_t[i];
        }
        
        // Update cell state candidate weights and biases
        for (int i = 0; i < hidden_size; i++) {
            for (int j = 0; j < input_size + hidden_size; j++) {
                Wc[i][j] -= learning_rate * dc_tilde[i] * x_h[j];
            }
            bc[i] -= learning_rate * dc_tilde[i];
        }
        
        // Update output gate weights and biases
        for (int i = 0; i < hidden_size; i++) {
            for (int j = 0; j < input_size + hidden_size; j++) {
                Wo[i][j] -= learning_rate * do_t[i] * x_h[j];
            }
            bo[i] -= learning_rate * do_t[i];
        }
        
        // Compute gradients with respect to inputs for backpropagation to earlier layers
        vector dx_t(input_size, 0.0);
        vector dh_prev(hidden_size, 0.0);
        vector dc_prev = dc_t * f_t; // Gradient of previous cell state
        
        // Gradient with respect to previous hidden state and input
        for (int i = 0; i < hidden_size; i++) {
            for (int j = 0; j < input_size; j++) {
                dx_t[j] += Wf[i][j] * df_t[i] + Wi[i][j] * di_t[i] + 
                           Wc[i][j] * dc_tilde[i] + Wo[i][j] * do_t[i];
            }
            for (int j = 0; j < hidden_size; j++) {
                dh_prev[j] += Wf[i][j + input_size] * df_t[i] + 
                             Wi[i][j + input_size] * di_t[i] + 
                             Wc[i][j + input_size] * dc_tilde[i] + 
                             Wo[i][j + input_size] * do_t[i];
            }
        }
        
        return {dx_t, dh_prev, dc_prev};
    }
};

// The LSTM Network class
class LSTMNetwork {
private:
    int input_size;
    int hidden_size;
    int output_size;
    LSTMCell lstm_cell;
    
    // Output layer weights and biases
    std::vector<vector> Wy;
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
        std::random_device rd;
				std::mt19937 gen(rd());
				std::normal_distribution<float> dist(0.0, 0.1f);
        
        Wy = std::vector<vector>(output_size, vector(hidden_size));
        by = vector(output_size, 0.0);
        
        for (int i = 0; i < output_size; i++) {
            for (int j = 0; j < hidden_size; j++) {
                Wy[i][j] = dist(gen);
            }
        }
    }
    
    float train(const std::vector<DataSample>& training_data) {
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
                y_pred = matmul(Wy, h);
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
        vector output = matmul(Wy, h);
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
    std::vector<DataSample> training_data;
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
