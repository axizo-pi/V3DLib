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
	Float::Ptr center_x, Float::Ptr center_y, Float::Ptr center_z,
	Float::Ptr radius,
	Float::Ptr ret_x, Float::Ptr ret_y, Float::Ptr ret_z,
	Float::Ptr ret_f
) {
	For (Int i = 0, i < N_spheres, i++)
  	// vec3 oc = m_center - r.origin();
		Float oc_x = *center_x - origin_x;                       comment("vec3 oc");
		Float oc_y = *center_y - origin_y;
		Float oc_z = *center_z - origin_z;

		*ret_x = oc_x;
		*ret_y = oc_y;
		*ret_z = oc_z;

    //auto a = r.direction().length_squared();
		Float dir_x = direction_x;                               comment("auto a");
		Float dir_y = direction_y;
		Float dir_z = direction_z;

		Float a = dir_x*dir_x + dir_y*dir_y + dir_z*dir_z;       comment("Float a");
		//*ret_f = a;

  	//auto h = dot(r.direction(), oc);
		Float h = dir_x*oc_x + dir_y*oc_y + dir_z*oc_z;          comment("Float h");
		//*ret_f = h;

  	//auto c = oc.length_squared() - m_radius*m_radius;
		Float rad = *radius;                                     comment("Float rad");
		Float c = (oc_x*oc_x + oc_y*oc_y + oc_z*oc_z) - rad*rad; comment("Float c");
		//*ret_f = c;

  	//auto discriminant = h*h - a*c;
  	Float discriminant = h*h - a*c;                          comment("Float discriminant");
		//*ret_f = discriminant;

  	// if (discriminant < 0) return false;
		Int valid = 1;
		Where (discriminant < 0.0f)  // <= leads to differences
			valid = 0;
		End

  	// auto sqrtd = std::sqrt(discriminant);
		Float sqrtd  = 0.0f;
		Float root   = 0.0f;
		Float root_2 = 0.0f;

		Float ray_t_min = 0.01f;
		Float ray_t_max = toFloat(0x7f800000); // Value for infinity; is a parameter in reference app

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
		*ret_f = root;


		center_x.inc();
		center_y.inc();
		center_z.inc();
		radius.inc();
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
	//to_file("sphere_hit_kernel.txt", s_sphere_hit->dump());
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
