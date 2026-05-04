#ifndef _V3DLIB_KERNEL_H_
#define _V3DLIB_KERNEL_H_
#include "BaseKernel.h"
#include "Source/Complex.h"
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

  /**
   * Construct kernel out of C++ function
   */
  Kernel(KernelFunction f, BaseSettings const &settings) : BaseKernel(settings) {
    bool prev = Platform::compiling_for_vc4();
    compile_init();

    driver().compile([this, f] () {
      f(mkArg<ts>(m_typelist)...);  // Construct the AST
    });

/*  
    // DEBUG: show the detected types for the param's (uniforms)
    std::string buf;
    buf << "Types: ";
    for (int i = 0; i < (int) m_typelist.size(); ++i) {
      buf << m_typelist[i] << ", ";
    }
    Log::warn << buf;
*/
    Platform::compiling_for_vc4(prev);
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
BaseKernel compile(void (*f)(ts... params), BaseSettings const &settings) {
  return (BaseKernel) Kernel<ts...>(f, settings);
}

}  // namespace V3DLib

#endif  // _V3DLIB_KERNEL_H_
