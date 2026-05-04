# LSTM from Scratch in C++

## Overview
This repository contains an implementation of a Long Short-Term Memory (LSTM) neural network written from scratch in C++, as described in my blog post ["I Built an LSTM from Scratch in C++. Here's What I Learned."](https://medium.com/@anudeepadi/i-built-an-lstm-from-scratch-in-c-heres-what-i-learned-defed7b90949)

This project was created to understand the inner workings of LSTM networks at a fundamental level, without relying on machine learning frameworks or libraries.

## Features
- Complete LSTM cell implementation with forget, input, and output gates
- Backpropagation through time (BPTT) with gradient clipping
- Simple network structure with configurable hyperparameters
- Example usage with sine wave prediction

## Requirements
- C++11 or higher
- Standard library only (no external dependencies)

## Implementation Details

### Data Structure
```cpp
struct DataSample {
    vector<double> features;
    double target;
};
```

### LSTM Cell
The LSTM cell implements the core functionality:
- Forward pass with all gates (forget, input, output)
- Backward pass with proper gradient computation
- Weight updates using gradient descent

### Network Architecture
- Configurable input size, hidden size, and output size
- Hyperparameters tunable via constructor (learning rate, epochs)
- Simple prediction interface

## Usage

```cpp
// Create and configure the LSTM network
int input_size = 1;
int hidden_size = 32;
int output_size = 1;
double learning_rate = 0.01;
int epochs = 50;

LSTMNetwork lstm(input_size, hidden_size, output_size, learning_rate, epochs);

// Create training data
vector<DataSample> training_data;
/* Populate with your data */

// Train the network
double final_loss = lstm.train(training_data);

// Make predictions
double prediction = lstm.predict(input_features);
```

## Compilation Notes

When compiling the code, make sure to include the `<tuple>` header:

```cpp
#include <vector>
#include <cmath>
#include <random>
#include <iostream>
#include <algorithm>
#include <tuple>  // Required for std::tie and tuple returns
```

## Key Lessons Learned

1. **Proper Weight Initialization**: Small random values are crucial to prevent vanishing/exploding gradients.

2. **Gradient Clipping**: Essential for stable training, implemented as:
   ```cpp
   double clip(double value, double min_value, double max_value) {
       return max(min_value, min(value, max_value));
   }
   ```

3. **Hyperparameter Tuning**: After experimentation, these values worked well:
   - Learning rate: 0.01
   - Hidden size: 32
   - Number of epochs: 50

4. **Memory Management**: Careful tracking of intermediate values for backpropagation.

## Future Improvements

- Bidirectional LSTM support
- Different optimization algorithms
- Dropout layers
- Batch normalization
- Mini-batch training

## License
MIT

## Contact
- Website: [anudeepadi.me](https://anudeepadi.me)
- LinkedIn: [adirajuadi](https://linkedin.com/in/adirajuadi)
- GitHub: [anudeepadi](https://github.com/anudeepadi)
