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
   * Construct kernel out of C++ function
   */
  Kernel(KernelFunction f, BaseSettings const &settings, bool do_generate_kernels = false) : BaseKernel(settings) {
    if (do_generate_kernels) {
      BaseKernel::generate_kernels([this, f] () {
        f(mkArg<ts>(m_typelist)...);  // Construct the AST
      });
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
BaseKernel gen_kernel(void (*f)(ts... params)) {
  BaseSettings settings;
  return (BaseKernel) Kernel<ts...>(f, settings, InitAction::do_generate);
}


template <typename... ts>
BaseKernel load_kernel(struct generated_kernels const &kernels, BaseSettings const &settings) {
  return (BaseKernel) Kernel<ts...>(kernels, settings);
}


template <typename... ts>
BaseKernel compile(void (*f)(ts... params), BaseSettings const &settings, InitAction do_action = InitAction::do_compile) {
  return (BaseKernel) Kernel<ts...>(f, settings, do_action);
}

}  // namespace V3DLib

#endif  // _V3DLIB_KERNEL_H_
