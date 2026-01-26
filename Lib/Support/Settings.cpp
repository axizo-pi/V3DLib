//
// Basic command line handling for the example programs.
//
// This contains common functionality for all examples.
//
///////////////////////////////////////////////////////////////////////////////
#include "Settings.h"
#include <cassert>
#include <memory>
#include <iostream>
#include "Platform.h"
#include "LibSettings.h"
#include "Support/basics.h"
#include "global/log.h"


namespace {

/**
 * Get the base filename, without the extension, from a path
 *
 * Source: https://stackoverflow.com/a/8520815
 *
 * In C++17, you can use `stem()`: https://en.cppreference.com/w/cpp/filesystem/path/stem
 */
std::string stem(const char *input) {
  std::string filename(input);

  // Remove directory if present.
  // Do this before extension removal incase directory has a period character.
  const size_t last_slash_idx = filename.find_last_of("\\/");
  if (std::string::npos != last_slash_idx) {
    filename.erase(0, last_slash_idx + 1);
  }

  // Remove extension if present.
  const size_t period_idx = filename.rfind('.');
  if (std::string::npos != period_idx) {
    filename.erase(period_idx);
  }

  return filename;
}


// ============================================================================
// Settings 
// ============================================================================

const char *blurb =
  "Example Program\n"      // TODO Perhaps fill this in dynamically
#ifdef QPU_MODE
  "\nRunning in QPU mode.\n"
#else
  "\nRunning in emulation mode.\n"
#endif
;


CmdParameters base_params = {
  blurb,
  {{
    "Output Generated Code",
    "-f",
    ParamType::NONE,     // Prefix needed to disambiguate
    "Write representations of the generated code to file"
  }, { 
    "Compile Only",
    "-c",
    ParamType::NONE,
    "Compile the kernel but do not run it"
  }, {
    "Select run type",
    "-r=",
    {"default", "emulator", "interpreter"},
    "Run the kernel on the QPU, emulator or on the interpreter"
  }, {
    "Disable logging",
    {"-s", "-silent"},
    ParamType::NONE,
    "Do not show the logging output on standard output"
#ifdef QPU_MODE
  }, {
    "Performance Counters",
    "-pc",
    ParamType::NONE,
    "Show the values of the performance counters"
#endif  // QPU_MODE
  }, {
    "QPU timeout",
    { "-t=", "-timeout="},
    ParamType::POSITIVE_INTEGER,
    "Time in seconds to wait for a result to come back from the QPUs",
    10
  }, {
    "Shared Memory Size",
    { "-m=", "-mem="},
    ParamType::POSITIVE_INTEGER,
    "Size in MB of the shared memory to use",
    V3DLib::LibSettings::heap_size() >> 20 
  }}
};


CmdParameters numqpu_params = {
  "",
  {{
    "Num QPU's",
    "-n=",
    INTEGER,
    "Number of QPU's to use. The values depends on the platform being run on:\n"
    "  - vc4 (Pi3+ and earlier), emulator: 1..12 (inclusive)\n"
    "  - vc6 (Pi4)                       : 1 or 8\n"
    "  - vc7 (Pi5)                       : 1..16 (inclusive)\n",
    1
  }, {
    "Max QPU's",
    "-nmax",
    ParamType::NONE,
		"Use the maximum available number of QPU's on the current platform.\n"
		"The maximum number for the VideoCore versions are: "
		"vc4: 12, vc6: 8, vc7: 16\n"
		"This parameter overrides any value set by parameter '-n'"
  }}
};


std::unique_ptr<CmdParameters> params;

CmdParameters &base_params_instance(bool use_numqpus = false) {
  if (!params) {
    CmdParameters *p = new CmdParameters(base_params);

    if (use_numqpus) {
      p->add(numqpu_params);
    }

    params.reset(p);
  }

  return *params;
}

}  // anon namespace


namespace V3DLib {

using ::operator<<;  // C++ weirdness

Settings::Settings(CmdParameters *derived_params, bool use_num_qpus) :
  m_derived_params(derived_params),
  m_use_num_qpus(use_num_qpus)
{}


void Settings::init(int argc, const char *argv[]) {
  // Store the app name
  name = stem(argv[0]);

  // Load the parameter definitions
  if (m_derived_params != nullptr) {
    m_all_params.add(*m_derived_params);
  }
  m_all_params.add(base_params_instance(m_use_num_qpus));

  check_params(m_all_params, argc, argv);

  set_loglevel(WARNING);
	// No need to set log level for Log
 	Log::set_log_dir("/home/wim/projects/V3DLib/log");
  Log::set_log_file("V3DLib.log");
}


void Settings::show_help() {
  std::string ret;

  ret << m_all_params.description() << "\n"
      << "General Parameters:\n"
      << base_params_instance(m_use_num_qpus).params_usage(true)
      << "\n";

  if (m_derived_params != nullptr) {
    ret << "Application Parameters:\n"
        << m_derived_params->params_usage(false)
        << "\n";
  }

  std::cout << ret;
}


/**
 * Parse the params from the commandline.
 * 
 * Also checks the params definition for correctness.
 *
 * Will exit locally if an error occured or help is displayed.
 * In other words, if it returns all is well.
 */
void Settings::check_params(CmdParameters &params, int argc, char const *argv[]) {
  if (params.has_errors()) {  // Prob not necessary. Keeping it in for now
    std::cout << params.get_errors();
    exit(CmdParameters::EXIT_ERROR);
  }

	Args args;
	for (int i = 1; i < argc; ++i) {  // Skip first param, which is the program name
		args.push_back(argv[i]);
	}

  int ret = CmdParameters::EXIT_ERROR;

  //
  // This mask the call to handle_help() in handle_commandline()
  //
  // This skips the init() and reset() calls in handle_commandline();
  // TODO check if this is OK
  //
  if (params.has_help(args)) {
    assert(!params.scan_action(args));  // No actions expected for V3DLib examples
    show_help();
    ret = CmdParameters::EXIT_NO_ERROR;
  } else {
    ret = params.handle_commandline(argc, argv, false);

    if (ret == CmdParameters::ALL_IS_WELL) {
      bool success = process() && init_params();
      if (!success) {
        ret = CmdParameters::EXIT_ERROR;
      }
    }
  }


  if (ret != CmdParameters::ALL_IS_WELL) exit(ret);
}


bool Settings::process() {
  auto const &p = m_all_params.parameters();

  output_code  = p["Output Generated Code"]->get_bool_value();
  compile_only = p["Compile Only"]->get_bool_value();
  silent       = p["Disable logging"]->get_bool_value();
  run_type     = p["Select run type"]->get_int_value();
#ifdef QPU_MODE
  show_perf_counters = p["Performance Counters"]->get_bool_value();
#endif  // QPU_MODE

  int qpu_timeout  = p["QPU timeout"]->get_int_value();
  LibSettings::qpu_timeout(qpu_timeout);

  int heap_mem     = p["Shared Memory Size"]->get_int_value();
  V3DLib::LibSettings::heap_size(heap_mem << 20);

  if (silent) {
    log_to_cout(false);      // Needs to be before debug that follows
    Log::log_to_cout(false);
  }

  if (m_use_num_qpus) {
    num_qpus    = p["Num QPU's"]->get_int_value();

    if (p["Max QPU's"]->get_bool_value()) {
    	if (run_type != 0 || Platform::run_vc4()) {
      	num_qpus = 12;
			}	else if (Platform::run_vc7()) {
      	num_qpus = 16;
      } else {  // vc6
      	num_qpus = 8;
      }
		}

    if (run_type != 0 || Platform::run_vc4()) {
      if (num_qpus < 0 || num_qpus > 12) {
        printf("ERROR: For vc4 and emulator, the number of QPU's selected must be between 1 and 12 inclusive.\n");
        return false;
      }
		}	else if (Platform::run_vc7()) {
      if (num_qpus < 0 || num_qpus > 16) {
        printf("ERROR: For vc7, the number of QPU's selected must be between 1 and 16 inclusive.\n");
        return false;
      }
    } else {
      if (num_qpus != 1 && num_qpus != 8) {
        printf("ERROR: For vc6, the number of QPU's selected must be 1 or 8.\n");
        return false;
      }
    }
  }

  if (run_type != 0) {
		//printf("Settings: using main memory.\n");
    Platform::use_main_memory(true);
    Platform::compiling_for_vc4(true);
  }

	if (compile_only) {
		Log::warn << "Only compiling, using main memory.";
 		Platform::use_main_memory(true);
	}

  return true;
}

}  // namespace V3DLib;
