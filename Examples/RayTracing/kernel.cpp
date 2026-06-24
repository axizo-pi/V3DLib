#include "kernel.h"
#include "Support/Helpers.h"
#include <memory>

using namespace V3DLib;

namespace kernel {
namespace {

void sphere_hit_kernel(
  Float origin_x, Float origin_y, Float origin_z,
  Float direction_x, Float direction_y, Float direction_z,
  Int N_spheres, // Blocks of 16
  Float::Ptr in_center_x, Float::Ptr in_center_y, Float::Ptr in_center_z,
  Float::Ptr in_radius,
  Float::Ptr ret_x, Float::Ptr ret_y, Float::Ptr ret_z,
  Float::Ptr ret_f
) {

  Float Inf       = toFloat(0x7f800000);  comment("Bit-value for infinity");
  Float ray_t_min = 0.001f;
  Float ray_t_max = Inf;    // Is a parameter in reference app

  Float acc_rec_t        = Inf;
	Float acc_rec_p_x;
	Float acc_rec_p_y;
	Float acc_rec_p_z;
	Float acc_rec_normal_x;
	Float acc_rec_normal_y;
	Float acc_rec_normal_z;
	Float acc_rec_front_face;

  For (Int i = 1, i < N_spheres, i++)
		Float center_x = *in_center_x;
		Float center_y = *in_center_y;
		Float center_z = *in_center_z;
    Float radius   = *in_radius;

    // vec3 oc = m_center - r.origin();
    Float oc_x = center_x - origin_x;                              comment("vec3 oc");
    Float oc_y = center_y - origin_y;
    Float oc_z = center_z - origin_z;

    *ret_x = oc_x;
    *ret_y = oc_y;
    *ret_z = oc_z;

    //auto a = r.direction().length_squared();
    Float dir_x = direction_x;                                     comment("auto a");
    Float dir_y = direction_y;
    Float dir_z = direction_z;

    Float a = dir_x*dir_x + dir_y*dir_y + dir_z*dir_z;             comment("Float a");
    //*ret_f = a;

    //auto h = dot(r.direction(), oc);
    Float h = dir_x*oc_x + dir_y*oc_y + dir_z*oc_z;                comment("Float h");
    //*ret_f = h;

    //auto c = oc.length_squared() - m_radius*m_radius;
    Float c = (oc_x*oc_x + oc_y*oc_y + oc_z*oc_z) - radius*radius; comment("Float c");
    //*ret_f = c;

    //auto discriminant = h*h - a*c;
    Float discriminant = h*h - a*c;                                comment("Float discriminant");
    //*ret_f = discriminant;

    // if (discriminant < 0) return false;
    Int valid = 1;
    Where (discriminant < 0.0f)  // `<=` leads to differences
      valid = 0;
    End

    // auto sqrtd = std::sqrt(discriminant);
    Float sqrtd  = 0.0f;
    Float root   = 0.0f;
    Float root_2 = 0.0f;

    Where (valid == 1)
      sqrtd = sqrt_f(discriminant);

      // Find the nearest root that lies in the acceptable range.
      // auto root = (h - sqrtd) / a;
      root = (h - sqrtd) / a;

      // auto root = (h + sqrtd) / a;
      root_2 = (h + sqrtd) / a;

      // if (!ray_t.surrounds(root)) {
      Where (!(ray_t_min < root && root < ray_t_max))
        root = root_2;

        //if (!ray_t.surrounds(root)) return false;
        Where (!(ray_t_min < root && root < ray_t_max))
          valid = 0;
        End
      End
    End

    //*ret_f = sqrtd;
    //*ret_f = root;

		Float rec_p_x;           header("Update rec");
		Float rec_p_y;
		Float rec_p_z;
 		Float outward_normal_x = 1.0f; // Init values to test if values set
 		Float outward_normal_y = 2.0f;
 		Float outward_normal_z = 3.0f;

    Float rec_t;
		Float rec_normal_x;
		Float rec_normal_y;
		Float rec_normal_z;
		Float front_face;

    Where (valid == 1) // && rec_t > root)
      // rec.t = root;
      rec_t = root;

  		// rec.p = r.at(rec.t);
  		rec_p_x = origin_x + rec_t*direction_x;  comment("Start ray.at()");
  		rec_p_y = origin_y + rec_t*direction_y;
  		rec_p_z = origin_z + rec_t*direction_z;

  		//vec3 outward_normal = (rec.p - m_center) / m_radius;
			//
			// Results are zero vectors for many sphere indexes >= 483 (out of 486)
			// Not clear why, but it's indicative that it happens for the highest indexes.
			//
 			outward_normal_x = (rec_p_x - center_x) / radius;  comment("Calc outward_normal");
 			outward_normal_y = (rec_p_y - center_y) / radius;
 			outward_normal_z = (rec_p_z - center_z) / radius;

  		// rec.set_face_normal(r, outward_normal);
			//
			// This sets the sign for the normal vector and stores it in rec.normal.
			// 32% of vectors have flipped signs wrt reference; many sphere indexes repeat.
			//
			Float tmp = direction_x*outward_normal_x
				        + direction_y*outward_normal_y
				        + direction_z*outward_normal_z;

			front_face = 1.0f;
			Where (tmp < 0)
				front_face = -1.0f;
			End

			rec_normal_x = front_face*outward_normal_x;
			rec_normal_y = front_face*outward_normal_y;
			rec_normal_z = front_face*outward_normal_z;
    End

		// Collect absolute values for this loop (absolute meaning smalles rec_t)
    Where (valid == 1 && acc_rec_t > rec_t)
    	acc_rec_t          = rec_t;
			acc_rec_p_x        = rec_p_x;
			acc_rec_p_y        = rec_p_y;
			acc_rec_p_z        = rec_p_z;
			acc_rec_normal_x   = rec_normal_x;
			acc_rec_normal_y   = rec_normal_y;
			acc_rec_normal_z   = rec_normal_z;
			acc_rec_front_face = front_face;
		End

		// Debug output
		*ret_x = rec_normal_x;
		*ret_y = rec_normal_y;
		*ret_z = rec_normal_z;

    in_center_x.inc();    header("Start increment pointers");
    in_center_y.inc();
    in_center_z.inc();
    in_radius.inc();

		// Increment debug pointers
    ret_x.inc();
    ret_y.inc();
    ret_z.inc();
    ret_f.inc();
  End
}

std::unique_ptr<BaseKernel> s_sphere_hit;


} // anon namespace

void init() {
  if (s_sphere_hit != nullptr) return;

  s_sphere_hit.reset(new BaseKernel(compile(sphere_hit_kernel))); //, settings())));
  to_file("sphere_hit_kernel.txt", s_sphere_hit->dump());
}


void sphere_hit(
  ray const &r, int N_spheres,
  Float::Array &center_x, Float::Array &center_y, Float::Array &center_z,
  Float::Array &radius,
  Float::Array &ret_x, Float::Array &ret_y, Float::Array &ret_z,
  Float::Array &ret_f
) {
  //warn << "sphere_hit N_spheres: " << N_spheres;

  float o_x = (float) r.origin().x();
  float o_y = (float) r.origin().y();
  float o_z = (float) r.origin().z();

  float d_x = (float) r.direction().x();
  float d_y = (float) r.direction().y();
  float d_z = (float) r.direction().z();

  s_sphere_hit->load(
    o_x, o_y, o_z,
    d_x, d_y, d_z,
    (N_spheres >> 4),
    &center_x, &center_y, &center_z,
    &radius,
    &ret_x, &ret_y, &ret_z,
    &ret_f
  ).run();
}

} // namespace kernel
