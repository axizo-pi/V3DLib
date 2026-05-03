/*

Reversal of uniforms was introduced to be able to compile on Intel platforms.
On this platform, the uniform parameters of the kernels were reversed during compilation.

Since QPU_MODE=0 has been removed from the project, this functionality is useless and
therefore removed. Retained in the scrap heap because I can envisage a timeline where
this is required again.

 */

//
// In Lib/Support/Helpers.h:
//

bool uniforms_reversed();


//
// In Lib/Support/Helpers.cpp:
//

/**
 * @brief Detect if the parameters (i.e. uniforms) are reversed on current platform.
 *
 * On `x86`, the kernel parameters are reversed.
 * Thus, last parameter in the kernel function call is initialized first.
 * This screws up initialization of the uniforms in the kernel.
 *
 * @return true if uniforms reversed, false otherwise
 *
 * =========================================
 *
 * Notes
 * -----
 *
 * The original hypothesis was  that the parameter reversal occured with gnu c++ v14.2.0.
 * This is not true; given compiler running on `Pi3B+` with `Debian 13 (Trixie) arm64` does not 
 * reverse.
 *
 * So, it must be the hardware platform, i.e. `x86`. This is the current hypothesis.
 */
bool uniforms_reversed() {
  static bool did_first = false;
  static platform_info ret;

  if (!did_first) {
    ret = get_platform_info();
    if (ret.is_x86()) {
      warn << "x86: Need to reverse the parameter indexes";
    } else {
      debug << "No need to reverse the parameter indexes";
    }

    did_first = true;
  }

  return ret.is_x86();
}


//
// In Lib/Params.h:
//

#include "Support/Helpers.h"        // uniforms_reversed()
...
template <typename T> bool append(
  IntList &uniforms,
  std::vector<std::size_t> &typelist,
  int &index,
  T val
) {
  bool ret = true;
  //Log::warn << "Here t2 " << index << ": " << (unsigned) var_type(val);

  int type_index = index;
  if (uniforms_reversed()) {
    type_index = (int) (typelist.size() - 1) - index;
  }
...
}


//
// In Lib/KernelDriver.h:
//

#include "Support/Helpers.h"        // uniforms_reversed()
...
/**
 * @brief Reverse the kernel parameter indexes if necessary.
 *
 * On Debian Trixie, gnu c++ v14.2.0, the initialization order of kernel parameters
 * is **reversed**. Thus, the last parameter in the kernel function call is initialized first.
 *
 * This screws up initialization of the uniforms in the kernel.
 * The goal of this function is to reverse the parameter indexes, so that the kernel functions
 * again as expected.
 */
MAYBE_UNUSED void reverse_uniforms(StmtStack &stack) {
  if (!uniforms_reversed()) return;
  //warn << "Doing reverse_uniforms()";

  Stmts uniforms;

  //
  // Retrieve the uniforms loading.
  //
  // Also checks that uniforms are be only in the start of the kernel code.
  //
  stack.each([&uniforms] (Stmts const &items) {
    bool check_start_uniforms = true;

    for (int i = 0; i < (int) items.size(); i++) {
      auto &item = *items[i];
      std::string tmp = item.dump();

      if (check_start_uniforms) {
        if (contains(tmp, "Uniform")) {
          //warn << "uniform! lhs: " << id;
          uniforms.push_back(items[i]);
        } else {
          check_start_uniforms = false;
        }
      } else {
        // Check the rest of the code for uniforms, not expecting any
        assert(!contains(tmp, "Uniform"));
      }
    }
  });

  warn << "uniforms:\n" << uniforms.dump();

  // Check first two parameters
  assert(contains(uniforms[0]->dump(true) , "QPU id"));
  assert(contains(uniforms[1]->dump(true) , "Num QPUs"));

  // Reverse the rest of the uniforms, which are the kernel parameters
  for (int i = 2; i < (int) uniforms.size(); ++i) {
    int j = (int) (uniforms.size() - 1) - (i - 2);
    if (j <= i) break;
    warn << "i,j: " << i << ", " << j;

    // Switch the values of the i and j items
    auto tmp_lhs = uniforms[i]->lhs();
    auto tmp_rhs = uniforms[i]->rhs();

    uniforms[i]->lhs(uniforms[j]->lhs());
    uniforms[i]->rhs(uniforms[j]->rhs());

    uniforms[j]->lhs(tmp_lhs);
    uniforms[j]->rhs(tmp_rhs);
  }

  warn << "uniforms post:\n" << uniforms.dump();
}
...

void KernelDriver::compile(std::function<void()> create_ast) {
  try {
    create_ast();
    reverse_uniforms(m_stmtStack);
...
}
