#include "kernel.h"
#include "defaults.h"

namespace {

/**
 * Global constants and values for this kernel.
 *
 * The main goal of this struct is to define constants at the top-level of the kernel,
 * so that they get calculated only once.
 * This avoids recalculating them within loops.
 */
struct Context {
	Context(Int &in_num_entities) {
	  DIST_FACTOR   = 1e-12f;         comment("Init DIST_FACTOR");
  	MASS_FACTOR   = 1e-18f;         comment("Init MASS_FACTOR");

  	ACC_CONSTANT  = ((float) BIG_G) * DIST_FACTOR / MASS_FACTOR * DIST_FACTOR;
		comment("Init ACC_CONSTANT");

  	Count        = BATCH_STEPS;
		num_entities = in_num_entities; // Must be multiple of 16
  	delta_t      = (float) dt;      comment("init delta_t");
	}

  //
  // These conversion factors exist to prevent Inf values.
  // Distance and mass values are quite huge and calculations
  // go over the max float value without breaking a sweat.
  //
  Float DIST_FACTOR;
  Float MASS_FACTOR;
  Float ACC_CONSTANT;

  Int   Count;          // Number of times to do the complete calculation per kernel call
  Int   num_entities;
  Float delta_t;				// Time between full calculations. Default is 1 day
};


/**
 * Update speed and position.
 */
void kernel_update(
  Float &x    , Float &y    , Float &z,
  Float &v_x  , Float &v_y  , Float &v_z,
  Float &acc_x, Float &acc_y, Float &acc_z,
  Float &delta_t
) {
  header("Update speed and position");

  v_x = v_x + acc_x * delta_t;
  v_y = v_y + acc_y * delta_t;
  v_z = v_z + acc_z * delta_t;

  x = x + v_x * delta_t;
  y = y + v_y * delta_t;
  z = z + v_z * delta_t;
}


void vector_calc_acc(
  Float &x0, Float &y0, Float &z0,
  Float &accum_x, Float &accum_y, Float &accum_z,
  Int &entity_index,
  Float::Ptr &px, Float::Ptr &py, Float::Ptr &pz, Float::Ptr &pmass,
	Context &c
) {
  header("Start vector_calc_acc");

  x0    *= c.DIST_FACTOR;
  y0    *= c.DIST_FACTOR;
  z0    *= c.DIST_FACTOR;

  // Accumulated acceleration vector
  Float x0_acc = 0;
  Float y0_acc = 0;
  Float z0_acc = 0;

  For (Int cur_block = 0, cur_block < (c.num_entities >> 4), cur_block++)
    Float x    = *px    * c.DIST_FACTOR; comment("Get entity coordinates");
    Float y    = *py    * c.DIST_FACTOR;
    Float z    = *pz    * c.DIST_FACTOR;
    Float mass = *pmass * c.MASS_FACTOR;

    // Acceleration vector
    Float nx1 = x0 - x;                comment("Calc acceleration vector");
    Float ny1 = y0 - y; 
    Float nz1 = z0 - z; 

    // Gravity/Acceleration components
    Float dx1 = nx1 * nx1;
    Float dy1 = ny1 * ny1;
    Float dz1 = nz1 * nz1;

    Float d1 = dx1 + dy1 + dz1;        comment("Calc distance denominator");

    // Set the force/acceleration to zero for current entity,
    // calculate the rest.
    Float denom        = 0;
    Float acceleration = 0;

    Where (((cur_block << 4) + index()) != entity_index)
      denom = recipsqrt(d1);
      acceleration = -1 * mass * c.ACC_CONSTANT * recip(d1);
    End

    // Post vc4: denom nonzero, acceleration zero for all intents and purposes

    //acceleration *= c.ACC_CONSTANT;
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

  //
  // Sum up and store in acc accumulators
  //
  rotate_sum(x0_acc, accum_x);  // NB second param is the output (result) param
  rotate_sum(y0_acc, accum_y);
  rotate_sum(z0_acc, accum_z);
}


/**
 *
 * -------------------
 *
 * Notes
 * =====
 *
 * 1. For _Sum up and return the acceleration_:  
 *    I tried updating speed and position for entity 0 here, the result is completely distorted.  
 *    This is unfortunate; I was hoping to make using multiple QPU's easier.  
 *    An option might be to use double buffering for speed and position.
 *
 */
void kernel_calc_acc(
  Float::Ptr &in_x, Float::Ptr &in_y, Float::Ptr &in_z,
  Float::Ptr &out_acc_x, Float::Ptr &out_acc_y, Float::Ptr &out_acc_z,
  Float::Ptr &in_mass,
	Context &c
) {
  header("Start loop kernel_calc_acc");

  For (Int cur_index = me(), cur_index < c.num_entities, cur_index += numQPUs())
    Int ptr_offset = cur_index - index();

    Float::Ptr px0    = in_x    + ptr_offset;
    Float::Ptr py0    = in_y    + ptr_offset; 
    Float::Ptr pz0    = in_z    + ptr_offset; 
    Float::Ptr pmass0 = in_mass + ptr_offset;

    Float x0    = *px0;
    Float y0    = *py0;
    Float z0    = *pz0;

    Float::Ptr px    = in_x;   comment("Local entity coordinate ptrs and mass");
    Float::Ptr py    = in_y;
    Float::Ptr pz    = in_z;
    Float::Ptr pmass = in_mass;

    // Accumulated acceleration vector
    Float x0_acc = 0;
    Float y0_acc = 0;
    Float z0_acc = 0;

    vector_calc_acc(
      x0, y0, z0,
      x0_acc, y0_acc, z0_acc,
      cur_index,
      px, py, pz, pmass,
			c
    );

    //
    // Sum up and return the acceleration. See Note 1.
    //

		// Following works for TMU, not for VPM(DMA).
		// VPM can not deal with vector offsets
/*
    Float::Ptr pacc_x = out_acc_x + ptr_offset;
    *pacc_x = x0_acc;

    Float::Ptr pacc_y = out_acc_y + ptr_offset;
    *pacc_y = y0_acc;

    Float::Ptr pacc_z = out_acc_z + ptr_offset;
    *pacc_z = z0_acc;
*/

		{
			Int offset = (cur_index << 4);

	    Float::Ptr pacc_x = out_acc_x + offset;
	    *pacc_x = x0_acc;

	    Float::Ptr pacc_y = out_acc_y + offset;
	    *pacc_y = y0_acc;

	    Float::Ptr pacc_z = out_acc_z + offset;
	    *pacc_z = z0_acc;
		}
  End
}


/**
 *
 */
void kernel_step(
  Float::Ptr &p_x    , Float::Ptr &p_y    , Float::Ptr &p_z,
  Float::Ptr &p_v_x  , Float::Ptr &p_v_y  , Float::Ptr &p_v_z,
  Float::Ptr &p_acc_x, Float::Ptr &p_acc_y, Float::Ptr &p_acc_z,
	Context &c
) {

  header("Start kernel_step");

  Int step         = numQPUs() << 4;
  Int start_offset = me()*16;

  p_x     += start_offset;
  p_y     += start_offset;
  p_z     += start_offset;
  p_v_x   += start_offset;
  p_v_y   += start_offset;
  p_v_z   += start_offset;

  Int dma_offset = index()*16;
  Int dma_step   = 16*16;

  p_acc_x += dma_offset;
  p_acc_y += dma_offset;
  p_acc_z += dma_offset;

  For (Int cur_block2 = me(), cur_block2 < (c.num_entities >> 4), cur_block2 += numQPUs())
    Float x     = *p_x;
    Float y     = *p_y;
    Float z     = *p_z;
    Float v_x   = *p_v_x;
    Float v_y   = *p_v_y;
    Float v_z   = *p_v_z;

    Float acc_x = *(p_acc_x + cur_block2*dma_step);
    Float acc_y = *(p_acc_y + cur_block2*dma_step);
    Float acc_z = *(p_acc_z + cur_block2*dma_step);

    kernel_update(
      x    , y    , z,
      v_x  , v_y  , v_z,
      acc_x, acc_y, acc_z,
      c.delta_t
    );

    *p_v_x = v_x;
    *p_v_y = v_y;
    *p_v_z = v_z;

    *p_x = x;
    *p_y = y;
    *p_z = z;

    p_x     += step;   header("kernel_step do pointer increments");
    p_y     += step;
    p_z     += step;
    p_v_x   += step;
    p_v_y   += step;
    p_v_z   += step;
  End
}

} // anon namespace


/*
 //
 // Start of the kernel which will be more efficient on vc4,
 // to compensate for the DMA write.
 //
 // For small number of entities, this will perform less than current kernel,
 // because 16 entities are processed per QPU. Entities can thus not
 // be spread out over QPU's.
 //

/ **
 * Handle the entities in blocks of 16
 * /
void handle_vector(
  Int num_entities,
  Float::Ptr  in_x, Float::Ptr  in_y, Float::Ptr  in_z, Float::Ptr in_mass,
  Float::Ptr acc_x, Float::Ptr acc_y, Float::Ptr acc_z,
) {

  For (Int i = 0, i < (num_entities >> 4), i += 16)
    Float vec_x0 = *in_x;
    Float vec_y0 = *in_y;
    Float vec_z0 = *in_z;
    Float vec_mass0 = *in_mass;

    Float accum_x = 0;
    Float accum_y = 0;
    Float accum_z = 0;

    For (Int n = 0, i < 16, i++)
      // Prepare next entity to use
      Float x0 = 0;
      Float y0 = 0;
      Float z0 = 0;

      Where (index() == n)
        x0 = vec_x0;
        y0 = vec_y0;
        z0 = vec_z0;
      End

      rotate_sum(x0, x0);
      rotate_sum(y0, y0);
      rotate_sum(z0, z0);

      vector_calc_acc(
        x0, y0, z0,
        accum_x, accum_y, accum_z,
        n, ((i << 4) + n)
      );
    End

    *acc_x = accum_x;
    *acc_y = accum_y;
    *acc_z = accum_z;

    accum_x.inc();
    accum_y.inc();
    accum_z.inc();
    in_x.inc();
    in_y.inc();
    in_z.inc();
  End
}

 */


/**
 * Main entry point kernel
 */
void kernel_gravity(
  Float::Ptr in_x, Float::Ptr in_y, Float::Ptr in_z,
  Float::Ptr in_v_x, Float::Ptr in_v_y, Float::Ptr in_v_z,
  Float::Ptr in_acc_x, Float::Ptr in_acc_y, Float::Ptr in_acc_z,
  Float::Ptr in_mass,
  Int in_num_entities,
  Int::Ptr signal
) {
	Context c(in_num_entities);
  comment("Start Count loop");

  For (Int i = 0, i < c.Count, i++)

    kernel_calc_acc(
      in_x, in_y, in_z,
      in_acc_x, in_acc_y, in_acc_z,
      in_mass,
			c
    );

    barrier();
    //barrier(signal);

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
      c
    );

    barrier();
    //barrier(signal);
  End
}
