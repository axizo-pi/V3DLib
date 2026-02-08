#include "model.h"
#include "scalar.h"
#include "kernel.h"
#include "Support/Timer.h"
#include "settings.h"

using namespace Log;


///////////////////////////////////////////////////////////////
// Main 
///////////////////////////////////////////////////////////////

int main(int argc, const char *argv[]) {
  settings.init(argc, argv);

	warn << "Num qpu's: " << settings.num_qpus;

	Image img(512, 512);
	img.set_conversion_factor(IMG_CONVERSION_FACTOR);

	if (settings.kernel == 1) {
		//
		// Run scalar version
		//
		warn << "Run scalar kernel";
		Timer timer("Scalar timer", true);

		init_orbital_entities();
		scalar_run(img);

		// The plot image is always filled in,
		// output is optional.
		if (settings.output_orbits) {
			img.save("gravity.bmp");
		}
	} else {
		//
		// Run kernel version
		//
		warn << "Run GPU kernel";
		init_orbital_entities();
		Model m;
		m.init();
		m.plot();

    Int::Array signal;

		auto k = compile(kernel_gravity, settings);
	  k.setNumQPUs(settings.num_qpus);

		k.load(
			&m.x, &m.y, &m.z,
			&m.v_x, &m.v_y, &m.v_z,
			&m.mass,
			&m.acc_x, &m.acc_y, &m.acc_z,
			NUM,
      &signal
		);

	 	{
			Timer timer("QPU timer", true);

			double t = 0;
			while (t < t_end) {
		  	k.run();
				m.plot();
	
	  	  t += BATCH_STEPS*dt;
			}

			//warn << m.dump_acc();
			//warn << m.dump_pos();

			// The plot image is always filled in,
			// output is optional.
			if (settings.output_orbits) {
				m.save_img();
			}
		}
	}

  return 0;
}
