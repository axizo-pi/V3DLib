#include "model.h"
#include "scalar.h"
#include "kernel.h"
#include "Support/Timer.h"
#include "Support/Settings.h"

using namespace Log;

V3DLib::Settings settings;


///////////////////////////////////////////////////////////////
// Main 
///////////////////////////////////////////////////////////////

int main(int argc, const char *argv[]) {
  settings.init(argc, argv);

	Image img(512, 512);
	img.set_conversion_factor(IMG_CONVERSION_FACTOR);

	//
	// Run scalar version
	//
	// if (false)
	{
		Timer timer("Scalar timer", true);

		init_orbital_entities();
		scalar_run(img);

		img.save("gravity.bmp");
	}


	//
	// Run kernel version
	//
	init_orbital_entities();
	Model m;
	m.init();
	m.plot();
	//warn << m.dump_pos();

	auto k = compile(kernel_gravity, settings);
	k.load(
		&m.x, &m.y, &m.z,
		&m.v_x, &m.v_y, &m.v_z,
		&m.mass,
		&m.acc_x, &m.acc_y, &m.acc_z,
		NUM
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
		m.save_img();
	}

  return 0;
}
