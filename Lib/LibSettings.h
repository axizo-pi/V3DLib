#ifndef _V3DLIB_LIBSETTINGS_H_
#define _V3DLIB_LIBSETTINGS_H_

namespace V3DLib {

/**
 * Global settings for V3DLib
 */
namespace LibSettings {

int  qpu_timeout();
void qpu_timeout(int val);

int  heap_size();
void heap_size(int val);

bool use_tmu_for_load();
void use_tmu_for_load(bool val);

bool use_high_precision_sincos();
void use_high_precision_sincos(bool val);

bool dump_line_numbers();
void dump_line_numbers(bool val);

void L2Cache_enable(bool enable);

} // namespace LibSettings
} // namespace V3DLib

#endif  // _V3DLIB_LIBSETTINGS_H_
