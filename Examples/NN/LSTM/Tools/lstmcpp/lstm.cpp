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

using namespace std;

// Structure for data samples as mentioned in the blog
struct DataSample {
    vector<double> features;
    double target;
};

// Helper functions
double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

vector<double> sigmoid(const vector<double>& v) {
    vector<double> result(v.size());
    for (size_t i = 0; i < v.size(); i++) {
        result[i] = sigmoid(v[i]);
    }
    return result;
}

double tanh_custom(double x) {
    return tanh(x);
}

vector<double> tanh_custom(const vector<double>& v) {
    vector<double> result(v.size());
    for (size_t i = 0; i < v.size(); i++) {
        result[i] = tanh(v[i]);
    }
    return result;
}

double dsigmoid(double y) {
    return y * (1.0 - y);
}

vector<double> dsigmoid(const vector<double>& v) {
    vector<double> result(v.size());
    for (size_t i = 0; i < v.size(); i++) {
        result[i] = v[i] * (1.0 - v[i]);
    }
    return result;
}

double dtanh(double y) {
    return 1.0 - y * y;
}

vector<double> dtanh(const vector<double>& v) {
    vector<double> result(v.size());
    for (size_t i = 0; i < v.size(); i++) {
        result[i] = 1.0 - v[i] * v[i];
    }
    return result;
}

// Vector operations
vector<double> operator+(const vector<double>& a, const vector<double>& b) {
    vector<double> result(a.size());
    for (size_t i = 0; i < a.size(); i++) {
        result[i] = a[i] + b[i];
    }
    return result;
}

vector<double> operator*(const vector<double>& a, const vector<double>& b) {
    vector<double> result(a.size());
    for (size_t i = 0; i < a.size(); i++) {
        result[i] = a[i] * b[i];
    }
    return result;
}

vector<double> operator*(double scalar, const vector<double>& v) {
    vector<double> result(v.size());
    for (size_t i = 0; i < v.size(); i++) {
        result[i] = scalar * v[i];
    }
    return result;
}

// Matrix-vector multiplication
vector<double> matmul(const vector<vector<double>>& m, const vector<double>& v) {
    vector<double> result(m.size(), 0.0);
    for (size_t i = 0; i < m.size(); i++) {
        for (size_t j = 0; j < v.size(); j++) {
            result[i] += m[i][j] * v[j];
        }
    }
    return result;
}

// Vector concatenation
vector<double> concat(const vector<double>& a, const vector<double>& b) {
    vector<double> result = a;
    result.insert(result.end(), b.begin(), b.end());
    return result;
}

// Gradient clipping as mentioned in the blog
double clip(double value, double min_value, double max_value) {
    return max(min_value, min(value, max_value));
}

vector<double> clip(const vector<double>& v, double min_value, double max_value) {
    vector<double> result(v.size());
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
    vector<vector<double>> Wf; // Forget gate weights
    vector<double> bf;         // Forget gate bias
    vector<vector<double>> Wi; // Input gate weights
    vector<double> bi;         // Input gate bias
    vector<vector<double>> Wc; // Cell state candidate weights
    vector<double> bc;         // Cell state candidate bias
    vector<vector<double>> Wo; // Output gate weights
    vector<double> bo;         // Output gate bias
    
    // For backpropagation
    vector<double> x_t;        // Input at time t
    vector<double> h_prev;     // Previous hidden state
    vector<double> c_prev;     // Previous cell state
    vector<double> f_t;        // Forget gate activation
    vector<double> i_t;        // Input gate activation
    vector<double> c_tilde;    // Cell state candidate
    vector<double> c_t;        // Current cell state
    vector<double> o_t;        // Output gate activation
    vector<double> h_t;        // Current hidden state
    
    // Random number generator for weight initialization
    random_device rd;
    mt19937 gen;
    normal_distribution<double> dist;

public:
    LSTMCell(int input_size, int hidden_size) : 
        input_size(input_size), 
        hidden_size(hidden_size),
        gen(rd()),
        dist(0.0, 0.1) {
        
        // Initialize weights with small random values as mentioned in the blog
        double scale = 0.1;
        
        Wf = vector<vector<double>>(hidden_size, vector<double>(input_size + hidden_size));
        bf = vector<double>(hidden_size, 0.0);
        
        Wi = vector<vector<double>>(hidden_size, vector<double>(input_size + hidden_size));
        bi = vector<double>(hidden_size, 0.0);
        
        Wc = vector<vector<double>>(hidden_size, vector<double>(input_size + hidden_size));
        bc = vector<double>(hidden_size, 0.0);
        
        Wo = vector<vector<double>>(hidden_size, vector<double>(input_size + hidden_size));
        bo = vector<double>(hidden_size, 0.0);
        
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
    pair<vector<double>, vector<double>> forward(const vector<double>& x, 
                                                const vector<double>& h_prev, 
                                                const vector<double>& c_prev) {
        // Store for backpropagation
        this->x_t = x;
        this->h_prev = h_prev;
        this->c_prev = c_prev;
        
        // Concatenate input and previous hidden state
        vector<double> x_h = concat(x, h_prev);
        
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
    tuple<vector<double>, vector<double>, vector<double>> backward(
        const vector<double>& dh_next, 
        const vector<double>& dc_next, 
        double learning_rate, 
        double clip_value = 5.0) {
        
        // Backpropagation through time (BPTT)
        
        // Gradient of the output gate
        vector<double> do_t = dh_next * tanh_custom(c_t) * dsigmoid(o_t);
        
        // Gradient of the cell state
        vector<double> dc_t = dc_next + dh_next * o_t * dtanh(tanh_custom(c_t));
        
        // Gradient of the input gate
        vector<double> di_t = dc_t * c_tilde * dsigmoid(i_t);
        
        // Gradient of the cell state candidate
        vector<double> dc_tilde = dc_t * i_t * dtanh(c_tilde);
        
        // Gradient of the forget gate
        vector<double> df_t = dc_t * c_prev * dsigmoid(f_t);
        
        // Concatenate x and h_prev for weight gradient computation
        vector<double> x_h = concat(x_t, h_prev);
        
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
        vector<double> dx_t(input_size, 0.0);
        vector<double> dh_prev(hidden_size, 0.0);
        vector<double> dc_prev = dc_t * f_t; // Gradient of previous cell state
        
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
    vector<vector<double>> Wy;
    vector<double> by;
    
    // Learning parameters
    double learning_rate;
    int epochs;
    
    // For backpropagation
    vector<double> y_pred;

public:
    LSTMNetwork(int input_size, int hidden_size, int output_size, 
               double learning_rate = 0.01, int epochs = 50) :
        input_size(input_size), 
        hidden_size(hidden_size),
        output_size(output_size),
        lstm_cell(input_size, hidden_size),
        learning_rate(learning_rate),
        epochs(epochs) {
        
        // Initialize output layer
        random_device rd;
        mt19937 gen(rd());
        normal_distribution<double> dist(0.0, 0.1);
        
        Wy = vector<vector<double>>(output_size, vector<double>(hidden_size));
        by = vector<double>(output_size, 0.0);
        
        for (int i = 0; i < output_size; i++) {
            for (int j = 0; j < hidden_size; j++) {
                Wy[i][j] = dist(gen);
            }
        }
    }
    
    double train(const vector<DataSample>& training_data) {
        double total_loss = 0.0;
        
        for (int epoch = 0; epoch < epochs; epoch++) {
            double epoch_loss = 0.0;
            
            for (const auto& sample : training_data) {
                // Initialize hidden state and cell state
                vector<double> h(hidden_size, 0.0);
                vector<double> c(hidden_size, 0.0);
                
                // Forward pass through the LSTM cell
                tie(h, c) = lstm_cell.forward(sample.features, h, c);
                
                // Output layer
                y_pred = matmul(Wy, h);
                for (int i = 0; i < output_size; i++) {
                    y_pred[i] += by[i];
                }
                
                // Compute loss (Mean squared error for simplicity)
                double loss = 0.0;
                vector<double> dy(output_size, 0.0);
                for (int i = 0; i < output_size; i++) {
                    double error = y_pred[i] - sample.target;
                    loss += error * error;
                    dy[i] = 2.0 * error; // Derivative of MSE
                }
                loss /= output_size;
                epoch_loss += loss;
                
                // Backpropagation
                
                // Output layer gradients
                vector<double> dh(hidden_size, 0.0);
                for (int i = 0; i < output_size; i++) {
                    for (int j = 0; j < hidden_size; j++) {
                        Wy[i][j] -= learning_rate * dy[i] * h[j];
                        dh[j] += Wy[i][j] * dy[i];
                    }
                    by[i] -= learning_rate * dy[i];
                }
                
                // LSTM cell backpropagation
                vector<double> dc(hidden_size, 0.0);
                lstm_cell.backward(dh, dc, learning_rate);
            }
            
            epoch_loss /= training_data.size();
            total_loss = epoch_loss;
            
            // Print progress
            if ((epoch + 1) % 10 == 0 || epoch == 0) {
                cout << "Epoch " << (epoch + 1) << "/" << epochs 
                     << ", Loss: " << epoch_loss << endl;
            }
        }
        
        return total_loss;
    }
    
    double predict(const vector<double>& features) {
        // Initialize hidden state and cell state
        vector<double> h(hidden_size, 0.0);
        vector<double> c(hidden_size, 0.0);
        
        // Forward pass
        tie(h, c) = lstm_cell.forward(features, h, c);
        
        // Output layer
        vector<double> output = matmul(Wy, h);
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
    int input_size = 1;
    int hidden_size = 32; // As mentioned in the blog
    int output_size = 1;
    double learning_rate = 0.01; // As mentioned in the blog
    int epochs = 50; // As mentioned in the blog
    
    // Create a simple sine wave dataset for demonstration
    vector<DataSample> training_data;
    for (int i = 0; i < 100; i++) {
        double x = i * 0.1;
        DataSample sample;
        sample.features = {x};
        sample.target = sin(x);
        training_data.push_back(sample);
    }
    
    // Create and train the LSTM network
    LSTMNetwork lstm(input_size, hidden_size, output_size, learning_rate, epochs);
    double final_loss = lstm.train(training_data);
    
    cout << "Training complete. Final loss: " << final_loss << endl;
    
    // Test predictions
    cout << "Testing predictions:" << endl;
    for (int i = 0; i < 10; i++) {
        double x = i * 0.1;
        double actual = sin(x);
        double predicted = lstm.predict({x});
        cout << "Input: " << x << ", Actual: " << actual
             << ", Predicted: " << predicted << endl;
    }
    
    return 0;
}
