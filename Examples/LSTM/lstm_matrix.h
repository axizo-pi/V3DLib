#ifndef _LSTM_LSTM_VECTOR_H
#define _LSTM_LSTM_VECTOR_H
#include <vector>


namespace lstm {

class vector : public std::vector<float> {
  using Parent = std::vector<float>;

public:
  vector() = default;
  vector(vector const &val) = default;
  vector(std::size_t size) : Parent(size) {}
  vector(std::size_t size, float val) : Parent(size, val) {}

  vector operator+(const vector & b) const;
  vector operator*(const vector & b) const;
};


vector operator*(float scalar, const vector &v);

class matrix: public std::vector<lstm::vector> {
  using Parent = std::vector<lstm::vector>;
    
public:    
  matrix() = default;
  matrix(std::size_t height, std::size_t width) : Parent(height, lstm::vector(width)) {}

  lstm::vector operator*(const lstm::vector &v);
};

} // namespace lstm


#endif // _LSTM_LSTM_VECTOR_H
