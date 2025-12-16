#ifndef _V3DLIB_KERNEL_H_
#define _V3DLIB_KERNEL_H_
#include <tuple>
#include "BaseKernel.h"
#include "Source/Complex.h"
#include <iostream>
#include <typeinfo>

namespace V3DLib {
	using ::operator<<; // C++ weirdness

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
  template <typename T> inline T mkArg( std::vector<std::size_t> &typelist) {
		auto t_hash = typeid(T).hash_code(); //both T and T::Ptr return the same value: N6V3DLib7Complex3PtrE
		//Log::warn << "mkArg t_hash: " << typeid(T).name();

	  auto c_hash = typeid(typename Complex::Ptr).hash_code();

	  if (t_hash == c_hash) {
			//Log::warn << "mkArg: Complex::Ptr detected";

			// Add two Float ptr's for Re and Im
			auto p_hash = typeid(typename Float::Ptr).hash_code();
			typelist.push_back(p_hash);
			typelist.push_back(p_hash);
		} else {
			auto hash = typeid(typename T::Ptr).hash_code();
			typelist.push_back(hash);
		}


		auto ret = T::mkArg();	
		return ret;
	}

public:
  Kernel(Kernel const &k) = delete;
  Kernel(Kernel &&k) = default;

  /**
   * Construct kernel out of C++ function
   */
  Kernel(KernelFunction f, BaseSettings const &settings) : BaseKernel(settings) {
		bool prev = Platform::compiling_for_vc4();
#ifdef QPU_MODE
 		if (!m_settings.compile_only) {
	  	if (Platform::use_main_memory() && m_settings.run_type == 0) {
				Log::info << "Main memory selected in QPU mode, running on emulator instead of QPU.";
	  	  m_settings.run_type = 1;
				Platform::compiling_for_vc4(true);
	  	}
		}
#endif

  	if (m_settings.run_type != 0) {
    	if (!Platform::compiling_for_vc4()) { 
				Log::info << "Kernel(): interpret or emulate selected, switching compile to vc4.";
				Platform::compiling_for_vc4(true);
			}
	  }

    if (m_settings.run_type != 0 || Platform::run_vc4()) {   // Compile vc4
			// Log::warn << "compile_init for vc4";
      compile_init(true);
    } else {                                                 // Compile v3d
      compile_init(false);
    }

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


  /**
   * Load uniform values.
	 *
	 * This version checks the args types at compile time.
	 * The version BaseKernel::load() checks types at run time.
   */
  template <typename... us>
  Kernel &load_k(us... args) {
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


/**
 * Compile and return a BaseKernel instance.
 *
 * This gets rid of the specific typing of the result
 * of a kernel template, and should be more flexible
 * in the calling code.
 */ 
template <typename... ts>
BaseKernel compile_b(void (*f)(ts... params), BaseSettings const &settings) {
  return (BaseKernel) Kernel<ts...>(f, settings);
}

}  // namespace V3DLib

#endif  // _V3DLIB_KERNEL_H_
