#include "device_info.h"
#include "broadcom/common/v3d_device_info.h"
#include <sstream>
#include "global/log.h"
#include <sys/ioctl.h>

// imports from v3d.cpp
namespace v3d {
  extern int get_fd();
  extern bool open();
}


using namespace Log;

namespace {

bool did_devinfo = false;
struct v3d_device_info s_devinfo = { .ver = 0 /* 0 signals no v3d */ };


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

/**
 * Call ioctl, restarting if it is interrupted
 *
 * Source: mesa/src/drm/xf86drm.c
 */
int drmIoctl(int fd, unsigned long request, void *arg) {
    int ret;

    do {
        ret = ioctl(fd, request, arg);
    } while (ret == -1 && (errno == EINTR || errno == EAGAIN));

    return ret;
}

#pragma GCC diagnostic pop


/**
 * Retrieve the device info for the current v3d device.
 *
 * @return true if call succeeded, false otherwise
 */
bool _v3d_device_info() {
  //printf("Called _v3d_device_info\n");
  if (did_devinfo) return true;  // run this exactly one
  did_devinfo = true;

  bool fd_is_open = (::v3d::get_fd() != 0);

  if (!fd_is_open) {
    // open fd if not already open
    //printf("_v3d_device_info opening fd\n");

    if (!v3d::open()) {
      cerr << "_v3d_device_info: Could not open fd.";
      return false;
    }
  }

  bool ret =  v3d_get_device_info(v3d::get_fd(), &s_devinfo, drmIoctl);

  if (!ret) {
    warn << "Could not get device_info";
  }

  return ret;
}

}  // anon namespace


#ifdef QPU_MODE
uint8_t devinfo_ver() {
  return devinfo()->ver;
}
#else


/**
 * @brief Empty call to satisfy linking in non-QPU mode.
 */
uint8_t devinfo_ver() { return 0; }

#endif // QPU_MODE

#ifdef __cplusplus
// Used in v3d/instr/v3d_api.c, hence the 'extern' brackets
extern "C" {
#endif

#ifdef QPU_MODE
struct v3d_device_info const *devinfo() {
  if(!_v3d_device_info()) {
    cerr << "devinfo: Call to _v3d_device_info() failed";
    return nullptr;
  }

  return &s_devinfo;
}
#else

/**
 * @brief Empty call to satisfy linking in non-QPU mode.
 * 
 * v3d should never be used anyway in that case.
 */
struct v3d_device_info const *devinfo() { return NULL; }

#endif

#ifdef __cplusplus
}
#endif


std::string v3d_device_info() {
  std::stringstream buf;

  if(!_v3d_device_info()) {
    buf << "Call to _v3d_device_info() failed\n";
    return buf.str();
  }


  auto &devinfo = s_devinfo;

  buf << "v3d devinfo" << "\n"
      << "===========" << "\n"
      << "Version                   : " <<  (int) devinfo.ver                    << "\n"
      << "Revision number           : " <<  (int) devinfo.rev                    << "\n"
      << "Compatibility rev number  : " <<  (int) devinfo.compat_rev             << "\n"
      << "Max # performance counters: " <<  (int) devinfo.max_perfcnt            << "\n"
      << "VPM size                  : " <<  (int) devinfo.vpm_size  << " bytes"  << "\n"

       // NSLC*QUPS from the core's IDENT registers. 
      << "Num QPU's                 : " <<  (int) devinfo.qpu_count              << "\n"

      << "Has accumulators          : " <<  (devinfo.has_accumulators?  "yes":"no")  << "\n"
      << "Has reset counter         : " <<  (devinfo.has_reset_counter? "yes":"no")  << "\n"

      // Granularity for the Clipper XY Scaling 
      << "Granularity               : " <<  devinfo.clipper_xy_granularity << "\n"

      // WRI: Explanatory gobbledygook:
      /** The Control List Executor (CLE) pre-fetches V3D_CLE_READAHEAD
       *  bytes from the Control List buffer. The usage of these last bytes
       *  should be avoided or the CLE would pre-fetch the data after the
       *  end of the CL buffer, reporting the kernel "MMU error from client
       *  CLE".
       */
      << "CLE readahead             : " <<  (int) devinfo.cle_readahead << "\n"

      // Minimum size for a buffer storing the Control List Executor (CLE)
      << "CLE buffer min size       : " <<  (int) devinfo.cle_buffer_min_size  << "\n"
  ;

  return buf.str();
}
