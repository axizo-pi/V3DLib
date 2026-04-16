#include "Debugger.h"
#include "State.h"
#include <iostream>              // std::cin
#include <algorithm>             // std::find

namespace V3DLib {

std::string Debugger::show_help() const {
  std::string ret;

  ret << "  ?     - show this help\n"
      << "  b     - show breakpoints\n"
      << "  b <l> - add breakpoint at line <l>\n"
      << "  B <l> - remove breakpoint at line <l>\n"
      << "  c     - continue running\n"
      << "  h <a> - add heap view at address <a>\n"
      << "  H <a> - Remove heap view at address <a>\n"
      << "  n     - (or blank) run next instruction\n"
      << "  o     - show overview of QPU states\n"
      << "  s     - show semaphores\n"
      << "  S     - hide semaphores\n"
      << "  v     - show VPM\n"
      << "  V     - hide VPM\n"
      << "  r     - run till end, ignoring breakpoints\n"
      << "  q     - Quit right away\n";

  return ret;
}


std::string Debugger::show_heapviews() const {
  std::string ret;

  for (int addr: m_heap_view) {
    ret << show_heap(addr);
  }

  return ret;
}


std::string Debugger::show_heap(int addr) const {
  std::string ret;

  ret << "  " << addr << ": ";

  for (int i = 0; i < 16; ++i) {
    ret << m_state.emuHeap[addr + i] << ", ";
  }

  ret << "\n";

  return ret;
}


bool Debugger::breakpoint_present(int line) const {
  return vector_present(m_breakpoint, line);
}


bool Debugger::vector_present(std::vector<int> const &v, int val) const {
  return (std::find(v.begin(), v.end(), val) != v.end());
}


void Debugger::add_breakpoint(int line) {
  if (!vector_add(m_breakpoint, line)) {
    warn << "add_breakpoint(): line " << line << " already present, not adding.";
  }
}


void Debugger::add_heapview(int addr) {
  warn << "Adding heap view at address " << addr;

  if (addr < 0 || (addr + 16) > (int) m_state.emuHeap.size()) {
    warn << "add_heapview(): address " << addr << " is outside of heap range, not adding.";
    return;
  }

  if (!vector_add(m_heap_view, addr)) {
    warn << "add_heapview(): address " << addr << " already present, not adding.";
  }
}


/**
 * @return true if add succeeded, false otherwise.
 */
bool Debugger::vector_add(std::vector<int> &v, int val) {
  if (vector_present(v, val)) {
    return false;
  }

  v.push_back(val);
  return true;
}


/**
 * @brief Remove given value from the vector
 */
void Debugger::vector_remove(std::vector<int> &v, int val, const char *label) {
   assert(label != nullptr);

  if (!vector_present(v, val)) {
    std::cerr << "Error: no " << label << " for " << val << "." << std::endl;
  } else {
    warn << "Removing " << label << " for " << val;
    v.erase(std::remove(v.begin(), v.end(), val), v.end());
  }
}


/**
 * @return integer value if read succeeded, -1 otherwise
 */
int Debugger::read_int(std::stringstream &ss, bool show_error) {
  int line;
  ss >> line;
  if (ss.fail()) {
    if (show_error) {
      std::cerr << "Error: no breakpoint at line " << line << "." << std::endl;
    }
    return -1;
  } else {
    return line;
  }
}


std::string Debugger::show_breakpoints() const {
  std::string ret;

  ret << "Breakpoints: ";
  for (int l: m_breakpoint) {
    ret << l << ", ";
  }
  ret << "\n";

  return ret;
}


void Debugger::step(int qpu_num, int numQPUs) {
  QPUState *s = &m_state.qpu[qpu_num];

  int cur_pc = s->pc;
  if (breakpoint_present(cur_pc)) do_step = true;

  if (!do_step) return;

  int first_pc = cur_pc - 2;
  if (first_pc < 0) first_pc = 0;
  int last_pc  = cur_pc + 2;
  if (last_pc > m_instrs.size()) last_pc = m_instrs.size();

  for (int i = first_pc; i <= last_pc; ++i) {
    if (i == cur_pc) {
      std::cout << " > ";
    } else {
      std::cout << "   ";
    }
    std::cout << i << ": " << m_instrs[i].dump() << "\n";
  }

  std::cout << s->dump(qpu_num);

  if (!m_heap_view.empty()) {
    std::cout << "Heap Views (heap size " << (int) m_state.emuHeap.size() << "):\n" 
              << show_heapviews();
  }

  if (disp_sema) std::cout << m_state.dump_sema();
  if (disp_vpm)  std::cout << m_state.dump_vpm();

  std::cout << "=============================================\n";

  bool do_loop = true;
  while (do_loop) {
    std::string input_line;
    std::cout << "> ";
    getline(std::cin, input_line);

    if (input_line.empty()) break;

    std::stringstream ss(input_line);
    char command;
    ss >> command;
            
    switch (command) {
      case 'b': {
        int line = read_int(ss, false);
        if (line == -1) {
          std::cout << show_breakpoints();
        } else {
          warn << "Adding breakpoint at line " << line;
          add_breakpoint(line);
        }
      }
      break;

      case 'B': {
        int line = read_int(ss);
        if (line != -1) {
          vector_remove(m_breakpoint, line, "breakpoint");
        }
      }
      break;

      case 'h': {
        int addr = read_int(ss);
        if (addr != -1) {
          add_heapview(addr);
        }
      }
      break;

      case 'H': {
        int addr = read_int(ss);
        if (addr != -1) {
          vector_remove(m_heap_view, addr, "heap view");
        }
      }
      break;

      case 'c':
        do_step = false;
        do_loop = false;
      break;

      case 'n':
        do_loop = false;
      break;

      case 'o': {
        std::cout << "QPU overview:\n";

        for (int i = 0; i < numQPUs; ++i) {
          auto const &qpu = m_state.qpu[i];

          std::cout << "  QPU " << i << ": " 
                    << "PC : "     << qpu.pc << ", "
                    << "running: " << qpu.dump_runstate()
                    << "\n";
        }

      }
      break;

      case 'r':
        m_breakpoint.clear();
        do_step = false;
        do_loop = false;
        break;
      case 's':
        disp_sema = true;
        std::cout << "Semaphores display enabled\n"; 
        break;
      case 'S':
        std::cout << "Disp semaphores disabled\n"; 
        disp_sema = false;
        break;
      case 'v':
        disp_vpm = true;
        std::cout << "Disp VPM enabled\n"; 
        break;
      case 'V':
        std::cout << "Disp VPM disabled\n"; 
        disp_vpm = false;
        break;
      case 'q':
        info << "Quitting program" << thrw;  // Throw in the hope of cleanup
        break;
      case '?': 
        std::cout << show_help();
        break;
      default:
        std::cout << "Unknown command. Enter '?' for overview.\n";
    }
  }
}

} // namespace V3DLib
