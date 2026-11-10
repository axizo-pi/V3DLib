#include "global.h"


namespace settings {
namespace {

const std::string base_dir  = "Examples/NN/GRU";

}  // anon namespace

const std::string input_dir   = base_dir + "/Tools/GRU/Inputs";
const std::string weights_dir = base_dir + "/Weights";

/**
 * @brief Number of epochs ro run.
 *
 * In the original, this is 1000.
 */
const int         num_epochs = 4; //1; //1000;

/**
 * @brief Limit the number of iterations per epoch to set value.
 *
 * If -1, run to max limit (with  current data, 6642)
 */
const int         max_limit  = 100; // -1;

}  // namespace settings
