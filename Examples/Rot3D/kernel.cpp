#include "kernel.h"
#include "settings.h"
#include "Support/Timer.h"
#include <V3DLib.h>
#include <math.h>            // sinf(), cosf()
#include "stlfile.h"

using namespace kernels;
using namespace V3DLib;

namespace {

struct KernelData : public StlData {
  Float::Array x;
  Float::Array y;
  Float::Array z;

  KernelData(int in_size);

  float xc(int index) const override { return x[index]; }
  float yc(int index) const override { return y[index]; }
  float zc(int index) const override { return z[index]; }

  void xc(int index, float rhs) override { x[index] = rhs; }
  void yc(int index, float rhs) override { y[index] = rhs; }
  void zc(int index, float rhs) override { z[index] = rhs; }
};


KernelData::KernelData(int in_size) : StlData(in_size) {
  // Allocate and initialise arrays shared between ARM and GPU
  // size will change if an stl file is loaded
  x.alloc(size());
  y.alloc(size());
  z.alloc(size());
}

} // anon namespace


void run_qpu_kernel(KernelType &kernel) {
  auto k = compile(kernel, settings);
  k.setNumQPUs(settings.num_qpus);

  KernelData data(settings.num_vertices);
  data.init();
	data.disp("Data pre");

  Timer timer;
  k.load(
    data.size(),
    settings.rot_x/2, settings.rot_y/2, settings.rot_z/2,
    &data.x, &data.y, &data.z
  ).run();
  timer.end(!settings.silent);

	data.disp("Data post");

  if (settings.save_stl) {
    stl_save_data(data, true);
  }
}
