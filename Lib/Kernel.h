#ifndef _V3DLIB_KERNEL_H_
#define _V3DLIB_KERNEL_H_
#include <tuple>
//#include <algorithm>  // std::move
#include "BaseKernel.h"
#include "Source/Complex.h"

namespace V3DLib {

// ============================================================================
// Parameter passing
// ============================================================================

template <typename... ts> inline void nothing(ts... args) {}


template <typename T, typename t> inline bool passParam(IntList &uniforms, t x) {
  return T::passParam(uniforms, x);
}


/**
 * Grumbl still need special override for 2D shared array.
 * Sort of patched this, will sort it out another time.
 *
 * You can not possibly have any idea how long it took me to implement and use this correctly.
 * Even so, I'm probably doing it wrong.
 */
template <>
inline bool passParam< Float::Ptr, Float::Array2D * > (IntList &uniforms, Float::Array2D *p) {
  return Float::Ptr::passParam(uniforms, &((BaseSharedArray const &) p->get_parent()));
}


template <>
inline bool passParam< Complex::Ptr, Complex::Array2D * > (IntList &uniforms, Complex::Array2D *p) {
  passParam< Float::Ptr, Float::Array2D * > (uniforms, &p->re());
  passParam< Float::Ptr, Float::Array2D * > (uniforms, &p->im());
  return true;
}


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
 *
 * 2. Another way to apply the arguments.
 *
 *    Following allows for custom handling in mkArg.
 *    A consequence is that uniforms are copied to new variables in the source lang generation.
 *    This happens at line 21 in assign.h:
 *
 *            f(std::get<N>(std::forward<Tuple>(t))...);
 *
 *    It's not much of an issue, just ugly.
 *    No need for calling using `apply()` right now, this is for reference.
 * 
 *      // Construct the AST for vc4
 *      auto args = std::make_tuple(mkArg<ts>()...);
 *      apply(f, args);
 */
template <typename... ts> struct Kernel : public BaseKernel {
  using KernelFunction = void (*)(ts... params);

  // Construct an argument of QPU type 't'.
  template <typename T> inline T mkArg() { return T::mkArg(); }
public:
  Kernel(Kernel const &k) = delete;
  Kernel(Kernel &&k) = default;

  /**
   * Construct kernel out of C++ function
   */
  Kernel(KernelFunction f, BaseSettings const &settings) : BaseKernel(settings) {
		bool prev = Platform::compiling_for_vc4();
#ifdef QPU_MODE
  	if (Platform::use_main_memory() && m_settings.run_type == 0) {
			Log::cdebug << "Main memory selected in QPU mode, running on emulator instead of QPU.";
	    m_settings.run_type = 1;
			Platform::compiling_for_vc4(true);
	  }


// Enable following when relevant - for now, we're good
// #ifdef ARM32
// 		Log::warn << "ARM 32-bits not supported any more. If it works, it works. You're on your own.";
// #endif

#endif

  	if (m_settings.run_type != 0) {
    	if (!Platform::compiling_for_vc4()) { 
				Log::cdebug << "Kernel(): interpret or emulate selected, switching compile to vc4.";
				Platform::compiling_for_vc4(true);
			}
	  }

    if (m_settings.run_type != 0 || Platform::run_vc4()) {   // Compile vc4
      compile_init(true);
    } else {                                                 // Compile v3d
      compile_init(false);

    }

    driver().compile([this, f] () {
      f(mkArg<ts>()...);  // Construct the AST
    });

		Platform::compiling_for_vc4(prev);
 }


  /**
   * Load uniform values.
   */
  template <typename... us>
  Kernel &load(us... args) {
    uniforms.clear();

    // Check arguments types of param us against types of ts
    nothing(passParam<ts, us>(uniforms, args)...);

    return *this;
  }
};


template <typename... ts>
Kernel<ts...> compile(void (*f)(ts... params), BaseSettings const &settings) {
  return Kernel<ts...>(f, settings);
}


/**
 * Compile with the default settings
 */
template <typename... ts>
Kernel<ts...> compile(void (*f)(ts... params)) {
	BaseSettings settings;
  return Kernel<ts...>(f, settings);
}

}  // namespace V3DLib

#endif  // _V3DLIB_KERNEL_H_
