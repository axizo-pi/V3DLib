#ifndef _LSTM_LSTMNETWORK_H
#define _LSTM_LSTMNETWORK_H
#include "LSTMCell.h"


/**
 * @brief Structure for data samples as mentioned in the blog
 */
struct DataSample {
  vector features;
  float target;
};


class DataSamples : public std::vector<DataSample> {
public:	
	void init();
};


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
              float learning_rate = 0.01f, int epochs = 50);
    
	float train(DataSamples &training_data);
  float predict(const vector & features);
};

#endif // _LSTM_LSTMNETWORK_H
