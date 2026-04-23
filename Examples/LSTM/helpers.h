#ifndef _LSTM_HELPERS_H
#define _LSTM_HELPERS_H
#include <random>

class normal_rand {
public:
	normal_rand() : gen(123 /*rd()*/), dist(0.0f, 0.1f) { }

	float rand() { return dist(gen); }

private:	
  std::random_device rd;
  std::mt19937 gen;
  std::normal_distribution<float> dist;
};

#endif // _LSTM_HELPERS_H
