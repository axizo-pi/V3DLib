/* ============================================================================
 *
 *	TODO: Discrepancy between qpu and scalar output. Examine and fix.
 *
 *  Input matrices verified to be the same.
 *  Outputs of qpu and interpreter _do_ check out.
 *
 * ============================================================================ */
#include <V3DLib.h>
#include "Support/Settings.h"
#include "Support/Timer.h"
#include "Support/Helpers.h"   // random_float()
#include "Kernels/Matrix.h"
#include <iostream>

using namespace V3DLib;
using namespace kernels;


// ============================================================================
// Command line handling
// ============================================================================

std::vector<const char *> const kernel_id = { "qpu", "cpu" };  // First is default

CmdParameters params = {
  "Matrix Multiplication\n\n"
  "Calculates the multiplication of two square matrices\n",
  {{
    "Kernel",
    "-k=",
    kernel_id,
    "Select the kernel to use\n"
  },{
    "Matrix dimension",
    { "-d=","-dimension="},
    ParamType::POSITIVE_INTEGER,
    "The number of matrix elements in a row/column. "
    "Must be a multiple of 16",
    48
  },{
    "Number of repeats",
    { "-p=","-repeat="},
    ParamType::POSITIVE_INTEGER,
    "The number times to execute the matrix multiplication",
    1
  },{
    "Output results",
    { "-output","-o"},
    ParamType::NONE,
    "Show the output of the matrix calculation",
  }}
};


struct MatrixSettings : public Settings {
  int kernel;
  int dimension;
  int repeats;
  bool do_output;

  int size() const { return dimension*dimension; }

  MatrixSettings() : Settings(&params, true) {}

  bool init_params() override {
    auto const &p = parameters();

    kernel      = p["Kernel"           ]->get_int_value();
    dimension   = p["Matrix dimension" ]->get_int_value();
    repeats     = p["Number of repeats"]->get_int_value();
    do_output   = p["Output results"   ]->get_bool_value();
    return true;
  }

} settings;


// ============================================================================
// Local functions
// ============================================================================

std::string arr_dump(float *arr, int dim) {
	std::string buf;

  for (int r = 0; r < dim; ++r) {
    buf << "(";
  	for (int c = 0; c < dim; ++c) {
      buf << arr[r*dim + c] << ", ";
   	}
    buf << ")\n";
  }

	return buf;
}


// ============================================================================
// Kernel Calls
// ============================================================================

void run_scalar_kernel() {
  if (settings.compile_only) return;
 
  // Allocate and initialise
  float *a      = new float [settings.size()];
  float *b      = new float [settings.size()];
  float *result = new float [settings.size()];

  int dim = settings.dimension;

  for (int r = 0; r < dim; r++) {
    for (int c = 0; c < dim; c++) {
      a[r*dim + c] = random_float();
      b[r*dim + c] = random_float();
    }
  }

	{
    Timer timer("matrix_mult_scalar", !settings.silent);
    for (int i = 0; i < settings.repeats; ++i) {
      kernels::matrix_mult_scalar(settings.dimension, result, a, b);
    }
  }

	if (settings.do_output && !settings.silent) {
		std::cout << arr_dump(result, dim);
  }

  delete [] a;
  delete [] b;
  delete [] result;
}


void run_qpu_kernel() {
  auto k = compile(kernels::matrix_mult_decorator(settings.dimension), settings);  // Construct kernel
  k.setNumQPUs(settings.num_qpus);

  int dim = settings.dimension;

  // Allocate and initialise arrays shared between ARM and GPU
  Shared2DArray<float> a(dim);
  Shared2DArray<float> b(dim);
  Shared2DArray<float> result(dim);

  for (int r = 0; r < dim; r++) {
    for (int c = 0; c < dim; c++) {
      a[r][c] = random_float();
      b[r][c] = random_float();
    }
  }

	{
    Timer timer("matrix qpu_kernel", !settings.silent);
    for (int i = 0; i < settings.repeats; ++i) {
      k.load(&result, &a, &b).run();
    }
  }

	if (settings.do_output && !settings.silent) {
	  std::cout << result.dump();
  }
}


// ============================================================================
// Main
// ============================================================================

int main(int argc, const char *argv[]) {
  settings.init(argc, argv);

  // Run a kernel as specified by the passed kernel index
  switch (settings.kernel) {
    case 0: run_qpu_kernel();     break;  
    case 1: run_scalar_kernel();  break;
    default: assert(false);       break;
  }

  if (!settings.silent) {
    auto name = kernel_id[settings.kernel];
    printf("Ran kernel '%s' %d time(s) with matrix size %d and %d QPU's.\n",
           name, settings.repeats, settings.dimension, settings.num_qpus);
  }

  return 0;
}
