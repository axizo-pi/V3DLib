#ifndef _V3DLIB_KERNEL_H_
#define _V3DLIB_KERNEL_H_
#include "BaseKernel.h"
#include "Source/Complex.h"
#include "Support/Platform.h"
#include <tuple>
#include <iostream>
#include <typeinfo>

//
// Main Page for doxygen documentation.
// Put here because this file is the main entry point for the library.
//

/** \mainpage V3DLib Source Documentation 

This source-code documentation is by no means complete; I'm filling it in as I go.
And I'm filling it in for better understanding as I code, because 

<b>I DON'T KNOW WTF I WAS THINKING FOUR YEARS AGO</b>.

Eventually, this will benefit you too.

==========================
Doxygen Wisdom
--------------

- Entities that are members of classes are only documented if their class is documented.
- Entities declared at namespace scope are only documented if their namespace is documented.
- Entities declared at file scope are only documented if their file is documented.

==========================
C++ Notes
---------

Source: https://git-scm.com/book/en/v2/Git-Tools-Submodules

This looks quite useful for e.g. kernel functions, which can have intricate
templated types. E.g.:

    void mandelbrot_kernel(
      Float topLeftReal, Float topLeftIm,
      Float offsetX, Float offsetY,
      Int numStepsWidth, Int numStepsHeight,
      Int numIterations,
      Int::Ptr result,
      Int num_repeats
    );
    
    // Usually: `decltype(mandelbrot_kernel) KernelType;
    // `decltype((x))` will create the lvaluei ie const version
    using KernelType = decltype(mandelbrot_kernel);

    void run_qpu_kernel(KernelType &kernel) {
      ...
    }
*/

namespace V3DLib {
  using ::operator<<; // C++ weirdness


/**
 * API kernel definition.
 *
 * This serves to make the kernel arguments strictly typed.
 *
 * -------------------------
 * NOTES
 * =====
 *
 * 1. A kernel is parameterised by a list of QPU types 'ts' representing
 *    the types of the parameters that the kernel takes.
 *
 *    The kernel constructor takes a function with parameters of QPU
 *    types 'ts'.  It applies the function to constuct an AST.
 */
template <typename... ts>
struct Kernel : public BaseKernel {
  using KernelFunction = void (*)(ts... params);

public:
  Kernel(Kernel const &k) = delete;
  Kernel(Kernel &&k) = default;
  Kernel(BaseSettings const &settings) : BaseKernel(settings) {}

  Kernel(struct generated_kernels const &kernels, BaseSettings const &settings) : BaseKernel(settings) {
    load_kernel(kernels);
  }

  /**
   * @brief Construct kernel out of C++ function
   */
  Kernel(KernelFunction f, BaseSettings const &settings) : BaseKernel(settings) {
    if (!settings.precompiled_outfile.empty()) {
      Log::warn << "precompiling kernels to: '" << settings.precompiled_outfile << "'";

      BaseKernel::generate_kernels([this, f] () {
        f(mkArg<ts>(m_typelist)...);  // Construct the AST
      });

      to_file(settings.precompiled_outfile, generated_kernels());
    } else {
      init_compile_struct();

      compile().compile([this, f] () {
        f(mkArg<ts>(m_typelist)...);  // Construct the AST
      });

      Platform::compile::done();
    }
  }
};


/**
 * Compile and return a BaseKernel instance.
 *
 * This gets rid of the specific typing of the result
 * of a kernel template, and should be more flexible
 * in the calling code.
 */ 
template <typename... ts>
BaseKernel compile(void (*f)(ts... params)) {
  BaseSettings settings;
  return (BaseKernel) Kernel<ts...>(f, settings);
}


template <typename... ts>
BaseKernel load_kernel(struct generated_kernels const &kernels, BaseSettings const &settings) {
  return (BaseKernel) Kernel<ts...>(kernels, settings);
}


template <typename... ts>
BaseKernel compile(void (*f)(ts... params), BaseSettings const &settings) {
  return (BaseKernel) Kernel<ts...>(f, settings);
}

}  // namespace V3DLib

#endif  // _V3DLIB_KERNEL_H_
