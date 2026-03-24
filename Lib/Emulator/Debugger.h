#ifndef _V3DLIB_EMULATOR_DEBUGGER_H_
#define _V3DLIB_EMULATOR_DEBUGGER_H_
#include "Target/instr/Instr.h"
#include "Common/SharedArray.h"  // Data
#include "QPUState.h"
#include "EmuState.h"

namespace V3DLib {

class State;

class Debugger {
public:
  Debugger(State &state, Target::Instr::List const &instrs) : m_state(state), m_instrs(instrs) {}

  void step(int qpu_num);
  void add_breakpoint(int line);

private:
  State &m_state;
  Target::Instr::List const &m_instrs;

  bool do_step  = false;
  bool disp_vpm = false;

  std::vector<int> m_breakpoint;
  bool breakpoint_present(int line) const;
  std::string show_breakpoints() const;

  std::vector<int> m_heap_view;
  void add_heapview(int addr);
  std::string show_heapviews() const;
  std::string show_heap(int addr) const;

  std::string show_help() const;

  // Support routines
  bool vector_present(std::vector<int> const &v, int val) const;
  bool vector_add(std::vector<int> &v, int val);
  void vector_remove(std::vector<int> &v, int val, const char *label);
  int read_int(std::stringstream &ss, bool show_error = true);
};


/**
 * @brief State of the VideoCore
 */
struct State : public EmuState {
  QPUState qpu[MAX_QPUS];  // State of each QPU
  Data emuHeap;

  State(int in_num_qpus, IntList const &in_uniforms) : EmuState(in_num_qpus, in_uniforms, true) {}
};

} // namespace V3DLib

#endif // _V3DLIB_EMULATOR_DEBUGGER_H_
