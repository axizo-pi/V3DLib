#ifndef _V3D_KERNEL_PARAMS_H_
#define _V3D_KERNEL_PARAMS_H_

#include "Source/Complex.h"
#include "Source/Float.h"

namespace V3DLib {

inline std::size_t var_type(Float::Array2D *p) {
	return typeid(Float::Ptr).hash_code();
}

inline uint32_t param_value(Float::Array2D *p) {
	return p->getAddress();
}

inline std::size_t var_type(Float::Array *p) {
	return typeid(Float::Ptr).hash_code();
}

inline uint32_t param_value(Float::Array *p) {
	return p->getAddress();
}

inline std::size_t var_type(Int::Array *p) {
	return typeid(Int::Ptr).hash_code();
}

inline uint32_t param_value(Int::Array *p) {
	return p->getAddress();
}

inline std::size_t var_type(int p) {
	return typeid(Int::Ptr).hash_code();
}

inline uint32_t param_value(int p) {
	return (uint32_t) p;
}

inline uint32_t param_value(float p) {
  int32_t* bits = (int32_t*) &p;
	return *bits;
}

inline std::size_t var_type(float p) {
	return typeid(Float::Ptr).hash_code();
}

inline bool ppassParam(IntList &uniforms, std::vector<std::size_t> &typelist, int index) {
	if (index != (int) typelist.size()) {
		Log::cerr << "Kernel load(): incorrect number of parameters. "
						  << "expected: " << (int) typelist.size() << ", got: " << index;
		return false;
	}

	return true;
}

template <typename T> bool append(
	IntList &uniforms,
	std::vector<std::size_t> &typelist,
	int &index,
	T val
) {
	bool ret = true;
	//Log::debug << "Here t2 " << index << ": " << (unsigned) var_type(val) << "\n";

	if (var_type(val) != typelist[index]) {
		Log::cerr << "ERROR incorrect type for param at index " << index << "\n";
		ret = false;
	} else {
		//Log::debug << "Param " << index << ": checks out.";
	}

  uniforms.append(param_value(val));
	++index;
	return ret;
}


template <typename T, typename ...t> bool ppassParam(
	IntList &uniforms,
	std::vector<std::size_t> &typelist,
	int index,
	T val, t... x
) {
	bool ret = append(uniforms, typelist, index, val);

	return ret & ppassParam(uniforms, typelist, index, x...);
}

template <typename ...t> bool ppassParam(
	IntList &uniforms,
	std::vector<std::size_t> &typelist,
	int index,
	Complex::Array *val, t... x
) {
	bool ret  = append(uniforms, typelist, index, &val->re());
	     ret &= append(uniforms, typelist, index, &val->im());

	return ret & ppassParam(uniforms, typelist, index, x...);
}


template <typename ...t> bool ppassParam(
	IntList &uniforms,
	std::vector<std::size_t> &typelist,
	int index,
	Complex::Array2D *val, t... x
) {
	bool ret  = append(uniforms, typelist, index, &val->re());
	     ret &= append(uniforms, typelist, index, &val->im());

	return ret & ppassParam(uniforms, typelist, index, x...);
}

}  // namespace V3DLib

#endif // _V3D_KERNEL_PARAMS_H_


