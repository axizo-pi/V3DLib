#include "kernel.h"
#include "defaults.h"

namespace {

/**
 * Pre: num_entities multiple of 16
 *
 * Using ref's doesn't increase performance.
 */
void kernel_calc_acc(
	Float::Ptr &in_x, Float::Ptr &in_y, Float::Ptr &in_z,
 	Float::Ptr &in_mass,
 	Float::Ptr &out_acc_x, Float::Ptr &out_acc_y, Float::Ptr &out_acc_z,
	Int &num_entities
) {
	//
	// These conversion factors are here to prevent Inf values.
	// Distance and mass values are quite huge and calculations
	// go over the max float value without breaking a sweat.
	//
	Float DIST_FACTOR  = 1e-12f;  comment("Init DIST_FACTOR");
	Float MASS_FACTOR  = 1e-18f;  comment("Init MASS_FACTOR");

	Float ACC_CONSTANT = ((float) BIG_G) * DIST_FACTOR / MASS_FACTOR * DIST_FACTOR;

	For (Int cur_index = 0, cur_index < num_entities, cur_index++)
		Int ptr_offset = cur_index - index();

		Float::Ptr px0    = in_x    + ptr_offset; comment("Start loop kernel_calc_acc");
		Float::Ptr py0    = in_y    + ptr_offset; 
		Float::Ptr pz0    = in_z    + ptr_offset; 
		Float::Ptr pmass0 = in_mass + ptr_offset;

		Float x0    = *px0    * DIST_FACTOR;
		Float y0    = *py0    * DIST_FACTOR;
		Float z0    = *pz0    * DIST_FACTOR;
		Float mass0 = *pmass0 * MASS_FACTOR;

		// Accumulated acceleration vector
		Float x0_acc = 0;
		Float y0_acc = 0;
		Float z0_acc = 0;

		Float::Ptr px    = in_x;   comment("Local entity coordinate ptrs and mass");
		Float::Ptr py    = in_y;
		Float::Ptr pz    = in_z;
		Float::Ptr pmass = in_mass;

		For (Int cur_block = 0, cur_block < (num_entities >> 4), cur_block++)
			Float x    = *px    * DIST_FACTOR; comment("Get entity coordinates");
			Float y    = *py    * DIST_FACTOR;
			Float z    = *pz    * DIST_FACTOR;
			Float mass = *pmass * MASS_FACTOR;

			// Acceleration vector
			Float nx1 = x0 - x;          comment("Calc acceleration vector");
			Float ny1 = y0 - y; 
			Float nz1 = z0 - z; 

			// Gravity/Acceleration components
			Float dx1 = nx1 * nx1;
			Float dy1 = ny1 * ny1;
			Float dz1 = nz1 * nz1;

			Float d1 = dx1 + dy1 + dz1;  comment("Calc distance denominator");
			//d1 = d1 * 1e12f;
			//d1 = 3.6e12f;
			

			// Set the force/acceleration to zero for current entity,
			// calculate the rest.
			Float denom = 0;
			Float acceleration = 0;

			Where (((cur_block << 4) + index()) != cur_index)
				denom = recipsqrt(d1);
				acceleration = -1 * mass * recip(d1);
			End

			acceleration *= ACC_CONSTANT;
			comment("Calc force factor");

			// Normalize the acceleration vector
			nx1 *= denom;  comment("Normalize the acc vector"); 
			ny1 *= denom; 
			nz1 *= denom; 

			// Set the force component
			nx1 *= acceleration; 
			ny1 *= acceleration; 
			nz1 *= acceleration; 
			

			// Add to total
			x0_acc += nx1;
			y0_acc += ny1;
			z0_acc += nz1;

			// Do next iteration
			px.inc();
			py.inc();
			pz.inc();
			pmass.inc();
		End

		// Sum up and return the acceleration
		rotate_sum(x0_acc, x0_acc);
		Float::Ptr pacc_x = out_acc_x + ptr_offset; // - index() + cur_index;
		*pacc_x = x0_acc;

		rotate_sum(y0_acc, y0_acc);
		Float::Ptr pacc_y = out_acc_y + ptr_offset; //- index() + cur_index;
		*pacc_y = y0_acc;

		rotate_sum(z0_acc, z0_acc);
		Float::Ptr pacc_z = out_acc_z + ptr_offset; //- index() + cur_index;
		*pacc_z = z0_acc;
	End
}


/**
 * Pre: num_entities multiple of 16
 *
 * Using ref's doesn't increase performance.
 */
void kernel_step(
	Float::Ptr &x, Float::Ptr &y, Float::Ptr &z,
	Float::Ptr &in_v_x, Float::Ptr &in_v_y, Float::Ptr &in_v_z,
 	Float::Ptr &acc_x, Float::Ptr &acc_y, Float::Ptr &acc_z,
	Int &num_entities
) {
	Float delta_t = (float) dt; comment("Start kernel_step");

	//
	// Update speed
	//
	header("Start kernel_step");

	Float::Ptr v_x = in_v_x;
	Float::Ptr v_y = in_v_y;
	Float::Ptr v_z = in_v_z;

	For (Int cur_block = 0, cur_block < (num_entities >> 4), cur_block++)
		Float tmp_x = *v_x + *acc_x * delta_t;
		*v_x = tmp_x;

		Float tmp_y = *v_y + *acc_y * delta_t;
		*v_y = tmp_y;

		Float tmp_z = *v_z + *acc_z * delta_t;
		*v_z = tmp_z;

		v_x.inc();  comment("kernel_step do pointer increments");
		v_y.inc();
		v_z.inc();
		acc_x.inc();
		acc_y.inc();
		acc_z.inc();
	End


	//
	// Update position
	//
	v_x = in_v_x;
	v_y = in_v_y;
	v_z = in_v_z;

	For (Int cur_block = 0, cur_block < (num_entities >> 4), cur_block++)
		Float tmp_x = *x + *v_x * delta_t;
		*x = tmp_x;

		Float tmp_y = *y + *v_y * delta_t;
		*y = tmp_y;

		Float tmp_z = *z + *v_z * delta_t;
		*z = tmp_z;

		x.inc();
		y.inc();
		z.inc();
		v_x.inc();
		v_y.inc();
		v_z.inc();
	End
}

} // anon namespace


/**
 * Pre: num_entities multiple of 16
 */
void kernel_gravity(
	Float::Ptr in_x, Float::Ptr in_y, Float::Ptr in_z,
	Float::Ptr in_v_x, Float::Ptr in_v_y, Float::Ptr in_v_z,
 	Float::Ptr in_mass,
 	Float::Ptr in_acc_x, Float::Ptr in_acc_y, Float::Ptr in_acc_z,
	Int num_entities
) {
	Int Count = BATCH_STEPS;

	For (Int i = 0, i < Count, i++)

		kernel_calc_acc(
			in_x, in_y, in_z,
		 	in_mass,
		 	in_acc_x, in_acc_y, in_acc_z,
			num_entities
		);

		// kernel_step() adjusts pointers, reset to start before calling	
		Float::Ptr x = in_x;
		Float::Ptr y = in_y;
		Float::Ptr z = in_z;
		Float::Ptr v_x = in_v_x;
		Float::Ptr v_y = in_v_y;
		Float::Ptr v_z = in_v_z;
		Float::Ptr acc_x = in_acc_x;
		Float::Ptr acc_y = in_acc_y;
		Float::Ptr acc_z = in_acc_z;

		kernel_step(
			x, y, z,
			v_x, v_y, v_z,
		 	acc_x, acc_y, acc_z,
			num_entities
		);
	End
}
