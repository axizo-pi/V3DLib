#include "Platform.h"
#include <sstream>
#include <fstream>
#include <memory>
#include <string.h>  // strstr()
#include "defines.h"
#include "basics.h"

namespace V3DLib {
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
    content == "";
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
 /* 
    field = read_field(line, "Hardware");
    if (!field.empty()) { 
      model = field;
      continue;
    }
*/
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
	enum VideoCoreType {
		UNKNOWN,
		vc4,
		vc6,
		vc7
	};

  PlatformInfo();

  std::string model_number;
  std::string revision;
	VideoCoreType vc_type = UNKNOWN;
  std::string platform_id; 

  bool is_pi_platform;
  bool m_use_main_memory   = false;
  bool m_compiling_for_vc4 = true;

  bool run_vc4() const { return vc_type == vc4; }
  int size_regfile() const;
  std::string output() const;
	int max_qpus() const;
};


PlatformInfo::PlatformInfo() {
  is_pi_platform = get_platform_string(platform_id);
  if (get_chip_version(model_number, revision)) {
    is_pi_platform = true;
  }

  if (!platform_id.empty() && is_pi_platform) {
   vc_type = (platform_id.find("Pi 4") != platform_id.npos)? vc6:
   	(platform_id.find("Pi 5") != platform_id.npos)? vc7:
		vc4;
  }

#ifndef QPU_MODE
  // Allow only emulator and interpreter modes, no hardware
  m_use_main_memory = true;
  vc_type = vc4;             // run vc4 code only
#endif
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


///////////////////////////////////////////////////////////////////////////////
// Class Platform
///////////////////////////////////////////////////////////////////////////////


void Platform::use_main_memory(bool val) {
#ifdef QPU_MODE
  instance().m_use_main_memory = val;
#else
  assertq(instance().m_use_main_memory, "Should only use main memory for emulator and interpreter", true);

  if (!val) {
    warning("use_main_memory(): ignoring passed value 'false', because QPU mode is disabled");
  }
#endif
}


/**
 * Sets the target platform to compile to.
 *
 * This is distinct from the platform we are actually running on.
 * The compilation can occur on any platform, including non-pi.
 */
void Platform::compiling_for_vc4(bool val) { instance().m_compiling_for_vc4 = val; }

bool Platform::compiling_for_vc4() { return instance().m_compiling_for_vc4; }
bool Platform::use_main_memory()   { return instance().m_use_main_memory; }
std::string Platform::platform_info() { return instance().output(); }
bool Platform::is_pi_platform()    { return instance().is_pi_platform; }
bool Platform::run_vc4()           { return instance().run_vc4(); }


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
int Platform::size_regfile() {
  {
    static bool showed = false;
  	if (!showed) debug("Platform::size_regfile(): add vc7.");
    showed = true;
  }

  if (compiling_for_vc4()) {
    return 32;
  } else {
    return 64;
  }
}


int Platform::max_qpus() {
	return instance().max_qpus();
}



int Platform::gather_limit() {
  {
    static bool showed = false;
  	if (!showed) debug("Platform::gather_limit(): add vc7.");
    showed = true;
  }

  if (compiling_for_vc4()) {
    return 4;
  } else {
    return 8;
  }
}


/**
 * Return short string with main version of the current pi
 */
std::string Platform::pi_version() {
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
  if (version == 'M') {  // Pi1 has no explicit number in version string; this checks the 'M' in 'Raspberry Pi Model B Rev 2'
    version = '1';
  }
  ret = "pi";
  ret += version;

  assertq('1' <= version && version <= '4', "Unknown pi version number");

#ifdef ARM64
  ret += "-64";
#endif

  return ret;
}

}  // namespace V3DLib
