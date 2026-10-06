#include "Platform.h"
#include "basics.h"
#include <string.h>  // strstr()
#include <fstream>
#include <memory>

namespace V3DLib {
namespace Platform {
namespace {

/**
 * @brief read the entire contents of a file into a string
 *
 * @param filename name of file to read
 * @param out_str  output parameter; place to store the file contents
 *
 * @return true if all went well, false if file could not be read.
 */
bool loadFileInString(const char *filename, std::string & out_str) {
  std::ifstream t(filename);
  if (!t.is_open()) {
    return false;
  }

  std::string str((std::istreambuf_iterator<char>(t)),
                   std::istreambuf_iterator<char>());

  if (str.empty()) {
    return false;
  }

  out_str = str;
  return true;
}


/**
 * @brief Detect Pi platform for newer Pi versions.
 *
 * @param content - output; store platform string here if found
 *
 * @return true if string describing platform found,
 *         false otherwise.
 */
bool get_platform_string(std::string &content) {
  // Alternative: cat /proc/device-tree/model
  const char *filename = "/sys/firmware/devicetree/base/model";

  bool success = loadFileInString(filename, content);
  if (!success) {
    content = "";
  }

  return success;
}


/**
 * Source: https://www.raspberrypi.org/documentation/hardware/raspberrypi/revision-codes/README.md
 */
std::string decode_revision(std::string const &revision) {
  assert(!revision.empty());

  // Convert string hex representation into number
  uint32_t hex;   
  std::stringstream ss;
  ss << std::hex << revision;
  ss >> hex;

  if (hex <= 15) { // pre-Pi2 have consecutive, non-encoded revisions
    return "BCM2835";
  }

  switch ((hex >> 12) & 0xf) {
    case 1: return "BCM2836"; break;
    case 2: return "BCM2837"; break;
    case 3: return "BCM2711"; break;

    case 0:
    default:
      return "BCM2835";
      break;
  }
}


/**
 * @brief Retrieve the VideoCore chip number.
 *
 * Detects if this is a VideoCore. This should also be sufficient for detecting
 * Pis, since it's the only thing to date(!) using this particular chip version.
 *
 * @param model     write parameter; receives chip model number
 * @param revision  write parameter; receives revision number
 *
 * @return true if Pi detected, false otherwise
 *
 * --------------------------------------------------------------------------
 * ## NOTES
 *
 * * `cat /proc/procinfo` is unreliable for kernels >= 4.9, will always
 *   return 'BCM2835`, although this seems to be corrected for 5.4.
 *   For this reason, the revision is decoded instead for the model number.
 *
 * * 'BCM2835' is the model number for the oldest Pis.
 *
 * * `BCM2837B0` also exists, but is not explicitly represented.
 */

bool get_chip_version(std::string &model, std::string &revision) {
  const char *filename = "/proc/cpuinfo";

  model.clear();
  revision.clear();

  std::ifstream t(filename);
  if (!t.is_open()) return false;

  auto read_field = [] (std::string const &line, std::string label) -> std::string {
    std::string ret;

    if (strstr(line.c_str(), label.c_str()) == nullptr) return ret;

    size_t pos = line.find(": ");
    assert(pos != line.npos);
    if (pos == line.npos) return ret;

    ret = line.substr(pos + 2);
    return ret;
  };

  std::string line;
  std::string field;

  while (getline(t, line)) {
    field = read_field(line, "Revision");
    if (!field.empty()) {
      revision = field;
      model = decode_revision(revision);  // Override previous assigned value of 'model' intentional
    }
  }

  return !model.empty();
}


///////////////////////////////////////////////////////////////////////////////
// Class PlatformInfo
///////////////////////////////////////////////////////////////////////////////

class PlatformInfo {
public:
  PlatformInfo();

  std::string model_number;
  std::string revision;
  VCType      vc_type         = UNKNOWN;
  VCType      m_compiling_for = UNKNOWN;
  VCType      emulating_for   = UNKNOWN;
  std::string platform_id; 

  bool is_pi_platform;
  bool m_use_main_memory   = false;
  bool m_running_emulator  = false;

  int size_regfile() const;
  std::string output() const;
  int max_qpus() const;
};


PlatformInfo::PlatformInfo() {
  is_pi_platform = get_platform_string(platform_id);
  if (get_chip_version(model_number, revision)) {
    is_pi_platform = true;
  }

  auto platform_contains = [&] (std::string str) -> bool {
    return (platform_id.find(str) != platform_id.npos);
  };

  if (!platform_id.empty() && is_pi_platform) {
   vc_type =
     platform_contains("Pi 4")? vc6:
     platform_contains("Pi Compute Module 4")? vc6:
     platform_contains("Pi 5")? vc7:
    vc4;
  }
/*
  // As default, select compiling for the platform you are on.
  // If you want to compile to vc4, you need to explicitly set this.
  m_compiling_for_vc4 = (vc_type == vc4);
*/
}


std::string PlatformInfo::output() const {
  std::string ret;

  if (!platform_id.empty()) {
    ret << "Platform    : " << platform_id.c_str() << "\n";
  } else {
    ret << "Platform    : " << "Unknown" << "\n";
  }

  ret << "Model Number: " << model_number.c_str() << "\n";
  ret << "Revision    : " << revision.c_str() << "\n";


  if (!is_pi_platform) {
    ret << "This is NOT a pi platform!\n";
  } else {
    ret << "This is a pi platform.\n";
    ret << "GPU: ";

    switch (vc_type) {
      case UNKNOWN: ret << "Unknown";             break;
      case vc4:     ret << "vc4 (VideoCore IV)";  break;
      case vc6:     ret << "v3d (VideoCore VI)";  break;
      case vc7:     ret << "v3d (VideoCore VII)"; break;
      default:      ret << "No clue!";
    }

    ret << "\n";
  }

  return ret;
}


int PlatformInfo::max_qpus() const {
  switch (vc_type) {
    case vc4: return 12;
    case vc6: return 8; 
    case vc7: return 16;
    default:  return -1;
  }
}


// Defined like this to delay the creation of the instance after program init,
// So that other globals get the chance to use it on program init.
std::unique_ptr<PlatformInfo> local_instance;


PlatformInfo &instance() {
  if (!local_instance) {
    local_instance.reset(new PlatformInfo);
  }

  return *local_instance;
}

}  // anon namespace


void use_main_memory(bool val) {
  instance().m_use_main_memory = val;
}

bool use_main_memory() { return instance().m_use_main_memory; }

/**
 * Compilation is only enabled if an actual compile is taking place.
 */
namespace compile {

/**
 * @brief Sets the target platform to compile to.
 *
 * This is distinct from the platform we are actually running on.
 * The compilation can occur on any platform, including non-pi.
 */
void start(VCType in_type) {
  assert(in_type != UNKNOWN);
  instance().m_compiling_for = in_type;
}


void compiling(bool do_vc4) { 
  if (do_vc4) {
    Log::warn << "Compiling forcing vc4";
  }

  if (do_vc4) {
    instance().m_compiling_for = vc4;
  } else {
    instance().m_compiling_for = instance().vc_type;
  }
}



bool running() { return instance().m_compiling_for != UNKNOWN; }

void done() {
  assertq(instance().m_compiling_for != UNKNOWN, "Stopping compiling for Unknown");
  instance().m_compiling_for = UNKNOWN;
}



bool for_vc4(bool do_break) {
  assert(!running_emulator());

  if (do_break) {
    if (instance().m_compiling_for == UNKNOWN) {
       warn << "compiling_for_vc4 compiling for Unknown";
       breakpoint;
    }
  }

  return instance().m_compiling_for == vc4;
}


/**
 * Determine if the compilation is done for vc7.
 *
 * Device (hardware) is accessed to determine what platform we are compiling on.
 *
 * Strictly speaking, it should not be necessary to determine the platform
 * we are running on. It is possible to compile for any platform on any
 * platform.
 */
bool for_vc7() {
  assert(!running_emulator());
/*
  // This overrides any device selection, due to emulator and interpreter
  if (instance().m_compiling_for_vc4) return false;
  return (instance().vc_type == vc7);  // This option is way easier
*/
  if (instance().m_compiling_for == UNKNOWN) {
     warn << "compiling_for_vc7 compiling for Unknown";
    breakpoint;
  }

  return (instance().m_compiling_for == vc7);
}


bool for_vc6() {
  assert(!running_emulator());
  return !for_vc4() && (instance().vc_type == vc6);
}

} // namespace compile


std::string platform_info() { return instance().output(); }
bool is_pi_platform()       { return instance().is_pi_platform; }

bool run_vc4() {
  //assert(!running_emulator());
  return instance().vc_type == vc4;
}


bool run_vc7() {
  //assert(!running_emulator());
  return instance().vc_type == vc7;
}


/**
 * @brief Return an indicator for the current platform
 */
Tag tag() {
  auto tmp = pi_version();
  //warn << "pi_version: '" << tmp << "'";

  Tag tag = not_pi;
       if (tmp == "pi1")    { tag = pi1;     }
  else if (tmp == "pi2")    { tag = pi2;     }
  else if (tmp == "pi3")    { tag = pi3;     }
  else if (tmp == "piZ")    { tag = pi_zero; }
  else if (tmp == "pi4-64") { tag = pi4;     }
  else if (tmp == "pi4")    { tag = pi4;     }
  else if (tmp == "pi5-64") { tag = pi5;     }
  else if (tmp == "pi5")    { tag = pi5;     }
  else {
    warn << "Unknown pi_version: '" << tmp << "'" << thrw;
  }


  return tag;
}


/**
 * Returns the number of available registers in a register file for the current
 * target platform
 *
 * For `vc4`, which has two register files 'A' and 'B' per QPU, returns the size
 * of each register file.
 * `v3d` has one single dual-port register file 'A' per QPU.
 *
 * Current implementation assumes no multi-threading has been enabled on the
 * QPU's. If that ever happens (not likely in this project), the size becomes
 * `size/num_threads`.
 *
 * However, according to internet hearsay, the default and minimum number of
 * threads for `v3d` is actually 2 per QPU. Still, 64 for `v3d` appears to be the
 * correct return value in this case.
 *
 * This all goes to show that something that appears to be exceedingly simple in
 * concept can actually be convoluted as f*** underwater.
 */
int size_regfile() {
  assert(!running_emulator()); // Warn me
  if (run_vc4()) return 32;
  return 64;  // v3d
}


int max_qpus() {
  return instance().max_qpus();
}


int gather_limit() {
  {
    static bool showed = false;
    if (!showed) cdebug << "Platform::gather_limit(): add vc7.";
    showed = true;
  }

  assert(!running_emulator());  // Warn me
  if (run_vc4()) {
    return 4;
  } else {
    return 8;
  }
}


/**
 * Return short string with main version of the current pi
 */
std::string pi_version() {
  std::string ret = "Not Pi";
  std::string val;

  if (!get_platform_string(val)) {
    return ret;
  }

  std::string const prefix = "Raspberry Pi ";

  if (val.find(prefix) != 0) {
    ret = "Not RPi";
    return ret;
  }

  char version = val[prefix.length()];
  // Pi1 and Zero have no explicit version numbers
  if (version == 'M') {
    version = '1';   // 'M' in 'Raspberry Pi Model B Rev 2'
  } else if (version == 'Z') {
    // OK; 'Z' in 'Raspberry Pi Zero W Rev 1.1'
  }
  ret = "pi";
  ret += version;

  assertq(('1' <= version && version <= '5') || (version == 'Z'),"Unknown pi version number");

#ifdef ARM64
  ret += "-64";
#endif

  return ret;
}


void run_emulator(VCType in_type) {
  assert(in_type != UNKNOWN);
  instance().emulating_for = in_type;
}

VCType emulating_for() {
  return instance().emulating_for;
}


/**
 * @brief Check if emulator or interpreter is running.
 *
 * This is only set if the emulator/interpreter is actually running.
 *
 * @return true if emulator or interpreter is running, false otherwise.
 */
bool running_emulator() { return instance().emulating_for != UNKNOWN; }

void done_emulating() { instance().emulating_for = UNKNOWN; }


/**
 * @brief return the actual hardware VideoCore type.
 *
 * The `run_` and `compile_` calls are now confusing me,
 * better to be more explicit with the platform type.
 *
 * **TODO:** Clean up `run_` and `compile_` calls.
 */
VCType vc_type() {
  auto type = instance().vc_type;
  assert(type != UNKNOWN);
  return type;
}


std::string vc_type_str(VCType type) {
  switch(type) {
    case UNKNOWN: return "Unknown";
    case vc4: return "vc4";
    case vc6: return "vc6";
    case vc7: return "vc7";
    default:  assert(false); return "none";  // Should never occur
  }
}


std::string vc_type_str() {
  return vc_type_str(instance().vc_type);
}


main_mem::main_mem(bool val) {
  m_prev = use_main_memory();
  use_main_memory(val);
}


main_mem::~main_mem() {
  use_main_memory(m_prev);
}

}  // namespace Platform
}  // namespace V3DLib
