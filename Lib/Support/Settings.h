#ifndef _V3DLIB_SUPPORT_SETTINGS_H
#define _V3DLIB_SUPPORT_SETTINGS_H
#include <CmdParameters.h>
#include "BaseSettings.h"

namespace V3DLib {

struct Settings : public BaseSettings {
  bool silent;

  Settings(CmdParameters *derived_params = nullptr, bool use_num_qpus = false);

  void init(int argc, const char *argv[]);
  virtual bool init_params() { return true; }
  TypedParameter::List const &parameters() const { return m_all_params.parameters(); }

private:
  CmdParameters * const m_derived_params;
  CmdParameters m_all_params;
  bool const m_use_num_qpus;

  void check_params(CmdParameters &params, int argc, char const *argv[]);
  bool process();
  void show_help();
};

}  // namespace V3DLib

#endif  // _V3DLIB_SUPPORT_SETTINGS_H
