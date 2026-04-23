#ifndef _LSTM_LSTM_MATRIX_H
#define _LSTM_LSTM_MATRIX_H
#include <vector>

namespace lstm {

class vector : public std::vector<float> {
  using Parent = std::vector<float>;

public:
  vector() = default;
  vector(vector const &val) = default;
  vector(std::size_t size) : Parent(size) {}
  vector(std::size_t size, float val) : Parent(size, val) {}

  vector operator+(vector const &b) const;
  vector operator*(vector const &b) const;

	vector sigmoid(vector const &bias);
};


vector operator*(float scalar, vector const &v);

class matrix: public std::vector<lstm::vector> {
  using Parent = std::vector<lstm::vector>;
    
public:    
  matrix() = default;
  matrix(std::size_t height, std::size_t width) : Parent(height, lstm::vector(width)) {}

  lstm::vector operator*(lstm::vector const &v);
};

//
// Helper functions
//
vector tanh(vector const &v);
vector dsigmoid(vector const &v);
vector dtanh(vector const &v);

//
// Vector operations
//
vector concat(vector const &a, vector const &b);
vector clip(vector const &v, float min_value, float max_value);

} // namespace lstm

#endif //  _LSTM_LSTM_MATRIX_H
