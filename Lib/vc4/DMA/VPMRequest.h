#ifndef _V3DLIB_VC4_DMA_VPMREQUEST_H_
#define _V3DLIB_VC4_DMA_VPMREQUEST_H_
#include <string>
#include "Support/basics.h"

namespace V3DLib {

/**
 * @brief VPM load request
 */
struct VPMLoadReq {
  VPMLoadReq() = default;
  VPMLoadReq(int setup);
  VPMLoadReq(int in_numVecs, int in_hor, int in_stride);

  void set(int in_numVecs, int in_hor, int in_stride);
  int code() const;
  std::string dump() const;

  int  numVecs;   // Number of vectors to load
  bool hor;       // Horizontal or vertical access?
  int  addr = -1; // Address in VPM to load from
  int  stride;    // Added to address after every vector read
};


/**
 * @brief VPM store request
 */
struct VPMStoreReq {
  VPMStoreReq() = default;
  VPMStoreReq(int setup);
  VPMStoreReq(int in_addr, bool in_hor, int in_stride);

  void set(int in_hor, int in_stride);
  int code() const;
  std::string dump() const;

  int  addr = -1; // Address in VPM to load from
  bool hor;       // Horizontal or vertical access?
  int  stride;    // Added to address after every vector written
};

}  // namespace V3DLib

#endif  // _V3DLIB_VC4_DMA_VPMREQUEST_H_
