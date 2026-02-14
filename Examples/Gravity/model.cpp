#include "model.h"
#include "scalar.h"

/**
 * 
 * Take the values from the scalar simulation, so that results can 
 * be compared.
 *
 * The main consideration here is that scalar uses doubles, which
 * do not exist on the QPU. QPU will use 4-byte Floats.
 * Differences in output are inevitable.
 */
void Model::init() {
  assert(NUM % 16 == 0);  // Num entities must be a multiple of 16

  x.alloc(NUM);
  y.alloc(NUM);
  z.alloc(NUM);
  v_x.alloc(NUM);
  v_y.alloc(NUM);
  v_z.alloc(NUM);
  mass.alloc(NUM);

	// Stride 16 for DMA
  acc_x.alloc(16*NUM);
  acc_y.alloc(16*NUM);
  acc_z.alloc(16*NUM);

  dummy.alloc(16);

  auto &o = orbital_entities;

  for (int i = 0; i < NUM; i++) {
    x[i]    = (float) o[i].x();
    y[i]    = (float) o[i].y();
    z[i]    = (float) o[i].z();
    v_x[i]  = (float) o[i].e[3];
    v_y[i]  = (float) o[i].e[4];
    v_z[i]  = (float) o[i].e[5];
    mass[i] = (float) o[i].e[6];
  }

  img.set_conversion_factor(IMG_CONVERSION_FACTOR);

  acc_x.fill(0);
  acc_y.fill(0);
  acc_z.fill(0);
}


void Model::plot() {
  for (int i = 0; i < (int) x.size(); ++i) {
    img.plot(x[i], y[i]);
  }
}


void Model::save_img() {
  img.save("gravity_qpu.bmp");
}


std::string Model::dump_pos() const {
  std::string ret;

  ret << "\n"
      << "x: " << x.dump() << "\n"
      << "y: " << y.dump() << "\n"
      << "z: " << z.dump() << "\n";

  return ret;
}


std::string Model::dump_acc() const {
  std::string ret;

	auto dump_with_stride = [] (Float::Array const &vec) -> std::string {
		std::string ret;

		for (int i = 0; i < 16; ++i) {
			ret << vec[i*16] << ", ";
		}

		return ret;
	};

  ret << "\n"
      << "acc_x: " << dump_with_stride(acc_x) << "\n"
      << "acc_y: " << dump_with_stride(acc_y) << "\n"
      << "acc_z: " << dump_with_stride(acc_z) << "\n"
	;

  return ret;
}
