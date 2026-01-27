#include "tools.h"
#include "global/log.h"
#include <cmath>
#include <random>

using namespace std;

namespace {

/**
 * Tried implementing random numberi generation withn Gaussian distribution
 * (https://stackoverflow.com/a/9266488/1223531).
 *
 * No luck. Sticking to normal.
 *
 */

// Source: https://stackoverflow.com/a/33389729/1223531
std::default_random_engine generator;
std::uniform_real_distribution<double> distribution(-1,1); //doubles from -1 to 1

} // anon namespace


/**
 * Return random double value in range -1.0..-1.0
 */
double frand() { return distribution(generator); }
