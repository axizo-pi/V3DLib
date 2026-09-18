#include "kernel.h"
#include "global.h"  // SINGLE_RAY
#include "Source/GlobalConstants.h"
#include "Support/Helpers.h"
#include "Support/Timer.h"

namespace kernel {
namespace {

//////////////////////////////////////////////////////////////////////
// Convenience classes to reduce clutter in the kernel and partials.
//
// The goal is to make further changes easier.
//////////////////////////////////////////////////////////////////////

struct Vector {
	Float x;
	Float y;
	Float z;

	void inner(Float &dst, Vector &rhs) {
  	dst = x*rhs.x + y*rhs.y + z*rhs.z;
	}

	void sub(Vector &dst, Vector &rhs) {
    dst.x = x - rhs.x;
    dst.y = y - rhs.y;
    dst.z = z - rhs.z;
	}

	void length_squared(Float &dst) {
    dst = x*x + y*y + z*z;
	}

	void assign(Vector &rhs) {
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
	}
};


struct RayPtr {
	Float::Ptr origin_x;
	Float::Ptr origin_y;
	Float::Ptr origin_z;
	Float::Ptr direction_x;
	Float::Ptr direction_y;
	Float::Ptr direction_z;

	void init(
  	Float::Ptr &p_origin_x,    Float::Ptr &p_origin_y,    Float::Ptr &p_origin_z,
  	Float::Ptr &p_direction_x, Float::Ptr &p_direction_y, Float::Ptr &p_direction_z
	) {
    origin_x    = p_origin_x;
    origin_y    = p_origin_y;
    origin_z    = p_origin_z;
    direction_x = p_direction_x;
    direction_y = p_direction_y;
    direction_z = p_direction_z;
	}

	void inc() {
    origin_x.inc();
    origin_y.inc();
    origin_z.inc();
    direction_x.inc();
    direction_y.inc();
    direction_z.inc();
	}

	void offset(Int &offset) {
    origin_x.offset(   offset);  comment("p_origin_x");
    origin_y.offset(   offset);  comment("p_origin_y");
    origin_z.offset(   offset);
    direction_x.offset(offset);
    direction_y.offset(offset);
    direction_z.offset(offset);  comment("End adjust point pointers");
	}

	// Param required for postfix operator
	RayPtr &operator++(int) noexcept {
    origin_x++;
    origin_y++;
    origin_z++;
    direction_x++;
    direction_y++;
    direction_z++;

		return *this;
	}
};


struct Ray {
	Vector origin;
	Vector direction;

  void load(RayPtr &ray_ptr) {
		nop(1);  sub_header("load ray_ptr");

  	origin.x    = *ray_ptr.origin_x;
	  origin.y    = *ray_ptr.origin_y;
	  origin.z    = *ray_ptr.origin_z;
	  direction.x = *ray_ptr.direction_x;
	  direction.y = *ray_ptr.direction_y;
	  direction.z = *ray_ptr.direction_z;
	}


  void load(RayPtr &ray_ptr, Int &ray_index) {
		nop(1);  sub_header("load ray_ptr index");
  	origin.x    = *(ray_ptr.origin_x + ray_index);
	  origin.y    = *(ray_ptr.origin_y + ray_index);
	  origin.z    = *(ray_ptr.origin_z + ray_index);
	  direction.x = *(ray_ptr.direction_x + ray_index);
	  direction.y = *(ray_ptr.direction_y + ray_index);
	  direction.z = *(ray_ptr.direction_z + ray_index);
	}		

	void set_at(Int &n, Ray &rhs) {
		nop(1);  sub_header("set_at");

    element_at(rhs.origin.x,    n, origin.x);      comment("element origin_x");
    element_at(rhs.origin.y,    n, origin.y);      comment("element origin_y");
    element_at(rhs.origin.z,    n, origin.z);
    element_at(rhs.direction.x, n, direction.x);
    element_at(rhs.direction.y, n, direction.y);
    element_at(rhs.direction.z, n, direction.z);
	}		

	void at(Vector &dst, Float &t) {
  	dst.x = origin.x + (t*direction.x);    sub_header("Update rec"); comment("Start ray.at()");
  	dst.y = origin.y + (t*direction.y);
  	dst.z = origin.z + (t*direction.z);
	}
};


struct SpherePtr {
  Float::Ptr center_x;
 	Float::Ptr center_y;
 	Float::Ptr center_z;
  Float::Ptr radius;

	void load(
	  Float::Ptr &in_center_x, Float::Ptr &in_center_y, Float::Ptr &in_center_z,
  	Float::Ptr &in_radius
	) {
	  center_x = in_center_x;
		center_y = in_center_y;
		center_z = in_center_z;
  	radius   = in_radius;
	}

	void inc() {
	  center_x.inc();
		center_y.inc();
		center_z.inc();
  	radius.inc();
	}
};


struct Sphere {
	Vector center;
  Float radius;

	void load(SpherePtr &ptr) {
    center.x = *ptr.center_x;
    center.y = *ptr.center_y;
    center.z = *ptr.center_z;
    radius   = *ptr.radius;
	}

	void load(SpherePtr &ptr, Int &offset) {
  	center.x = *(ptr.center_x + offset);
  	center.y = *(ptr.center_y + offset);
  	center.z = *(ptr.center_z + offset);
  	radius   = *(ptr.radius   + offset);
	}

	void normal(Vector &dst, Vector &p) {
  	dst.x = (p.x - center.x) / radius;
  	dst.y = (p.y - center.y) / radius;
  	dst.z = (p.z - center.z) / radius;
	}
};


///////////////////////////////////////////////////////////////////
// Partial Definitions
///////////////////////////////////////////////////////////////////

/**
 * @brief return the nearest sphere hit for the current ray.
 */
void hit_record_partial(
  Int   &ray_index,
  Int   &in_sphere_index,
  Float &in_t,
	Ray &r,
	SpherePtr &sphere_ptr,
  // Output parameters
  Float::Ptr &rec_p_x, Float::Ptr &rec_p_y, Float::Ptr &rec_p_z,
  Float::Ptr &rec_normal_x, Float::Ptr &rec_normal_y, Float::Ptr &rec_normal_z,
  Float::Ptr &rec_t,
  Float::Ptr &rec_front_face,
  Int::Ptr   &rec_sphere_index
) {
  nop(1);             sub_header("Start hit_record_partial");
  Float t = -1;

  // Get the best t of the values in the 16-vector `in_t`.
  // This should be the lowest value.
  Int min_index;
  rotate_min(in_t, t, min_index);
  Int sphere_index;
  element_at(in_sphere_index, min_index, sphere_index);

  // rec.p = r.at(rec.t);
	Vector p;
	r.at(p, t);                                sub_header("Update rec"); comment("Start ray.at()");

  //vec3 outward_normal = (rec.p - m_center) / m_radius;
  Int sphere_offset = sphere_index - index();
	Sphere sphere;
 	sphere.load(sphere_ptr, sphere_offset);

	Vector outward_normal;
	sphere.normal(outward_normal, p);                                     comment("Calc outward_normal");

  // rec.set_face_normal(r, outward_normal);
  //
  // This sets the sign for the normal vector and stores it in rec.normal.
  //
  Float tmp;
	r.direction.inner(tmp, outward_normal);

  // NOTE: minus sign is the other way around as I would expect; counter-intuitive but correct.
  Float front_face = -1.0f;
  Where (tmp < 0)
    front_face = 1.0f;
  End

  // `- index()` to save to a single location in main mem
  Int offset = ray_index - index();

  *(rec_sphere_index + offset) = sphere_index;
  *(rec_p_x        + offset) = p.x;
  *(rec_p_y        + offset) = p.y;
  *(rec_p_z        + offset) = p.z;
  *(rec_normal_x   + offset) = front_face*outward_normal.x;
  *(rec_normal_y   + offset) = front_face*outward_normal.y;
  *(rec_normal_z   + offset) = front_face*outward_normal.z;
  *(rec_t          + offset) = t;
  *(rec_front_face + offset) = front_face;
}


void sphere_hit_partial(
	Ray &r,
  Int &N_spheres,
	SpherePtr &sphere_ptr,
  // Internal variables
  Int &sphere_index,
  Float &ray_t_max
) {
   nop(1);                                                         sub_header("Start sphere_hit_partial");

  Float ray_t_min  = GlobalConst(0.001f);  // ray_t_min is an alias;
                                           // this is fine here because values doesn't change
  For (Int i = 0, i < N_spheres, i++)
    Int valid = 1;

		Sphere sphere;
		sphere.load(sphere_ptr);

    // Exclude items added to resize to multiple of 16 blocks
    Where (sphere.radius == 0.0f)
      valid = 0;
    End

    // vec3 oc = m_center - r.origin();
		Vector oc;
		sphere.center.sub(oc, r.origin);                               comment("vec3 oc");

    //auto a = r.direction().length_squared();
		Vector dir;
		dir.assign(r.direction);

    Float a;
		dir.length_squared(a);                                           comment("Float a");

    //auto h = f_dot(r.direction(), oc);
    Float h;
    dir.inner(h, oc);                                               comment("Float h");

    //auto c = oc.length_squared() - m_radius*m_radius;
    Float c;
		oc.length_squared(c);                                           comment("Float c");
    c -= sphere.radius*sphere.radius;

    //auto discriminant = h*h - a*c;
    Float discriminant = h*h - a*c;                                comment("Float discriminant");

    // if (discriminant < 0) return false;
    Where (discriminant < 0.0f)  // `<=` leads to differences
      valid = 0;
    End

    // auto  std::sqrt(discriminant);
    Float sqrtd  = 0.0f;
    Float root   = 0.0f;
    Float root_2 = 0.0f; sub_header("Start test root");

    Where (valid == 1)
      sqrtd = sqrt_f(discriminant);
      // Find the nearest root that lies in the acceptable range.
      // auto root = (h - sqrtd) / a;
      root = (h - sqrtd) / a;

      // auto root = (h + sqrtd) / a;
      root_2 = (h + sqrtd) / a;

      // if (!ray_t.surrounds(root)) {
      Where (!(ray_t_min < root && root < ray_t_max))

        //if (!ray_t.surrounds(root)) return false;
        Where (ray_t_min < root_2 && root_2 < ray_t_max)
          root = root_2;
        Else
          valid = 0;
        End
      End
    End

    Where (valid == 1)
      ray_t_max = root;                 comment("Setting ray_t_max");
      sphere_index = 16*i + index();
    End

		sphere_ptr.inc();                   comment("Start increment pointers");
  End
}


///////////////////////////////////////////////////////////////////
// Kernel Definition
///////////////////////////////////////////////////////////////////

/**
 * @brief Get the nearest hits for the given rays.
 *
 * All spheres are checked for a hit. The best hit, if any, is returned.
 *
 * A bad hit can be detected by checking the coordinates of `rec_p_*`; a failed
 * hit has Inf coordinates.
 */
void sphere_hit_kernel(
  // Input values
  Int ray_dummy,
  Int ray_num,
  Float::Ptr p_origin_x,    Float::Ptr p_origin_y,    Float::Ptr p_origin_z,
  Float::Ptr p_direction_x, Float::Ptr p_direction_y, Float::Ptr p_direction_z,
  Int N_spheres, // Blocks of 16
  Float::Ptr in_center_x, Float::Ptr in_center_y, Float::Ptr in_center_z,
  Float::Ptr in_radius,
  // Output values
  Float::Ptr rec_p_x, Float::Ptr rec_p_y, Float::Ptr rec_p_z,
  Float::Ptr rec_normal_x, Float::Ptr rec_normal_y, Float::Ptr rec_normal_z,
  Float::Ptr rec_t,
  Float::Ptr rec_front_face,
  Int::Ptr   rec_sphere_index
) {

  nop(1);                        sub_header("Init RayPtr");
	RayPtr ray_ptr;
	ray_ptr.init(
  	p_origin_x,    p_origin_y,    p_origin_z,
  	p_direction_x, p_direction_y, p_direction_z
	);

//#define BLOCK_READ

#if defined(SINGLE_RAY) || !defined(BLOCK_READ)
  nop(1);                        sub_header("Adjust point pointers");
  Int offset = index()*-4;
	ray_ptr.offset(offset);
#endif  

#ifdef SINGLE_RAY
  warn << "SINGLE_RAY defined kernel";

  Int ray_index = ray_dummy;
	Ray ray;
	ray.load(ray_ptr, ray_index);

	SpherePtr sphere_ptr;
	sphere_ptr.load(in_center_x, in_center_y, in_center_z, in_radius);
#else
  nop(1);                        sub_header("Start ray_index loop");

#ifdef BLOCK_READ
  const Int ray_blocks = ray_num >> 4;

  For (Int ray_block = 0, ray_block < ray_blocks, ray_block++)
		nop(1); sub_header("Init Ray");
		Ray in_ray;
		in_ray.load(ray_ptr);

    nop(1);                                      sub_header("Start index element loop");
    const Int n_max = 16;

    For (Int n = 0, n < n_max, n++)
      Int ray_index = n_max*ray_block + n;

			Ray ray;
			ray.set_at(n, in_ray);

			SpherePtr sphere_ptr;
		  sphere_ptr.load(in_center_x, in_center_y, in_center_z, in_radius);
#else  
  For (Int ray_index = 0, ray_index < ray_num, ray_index++)
		Ray ray;
		ray.load(ray_ptr);

		SpherePtr sphere_ptr;
	  sphere_ptr.load(in_center_x, in_center_y, in_center_z, in_radius);
#endif  
#endif  // SINGLE_RAY

    Int   sphere_index = -1;       comment("sphere_index"); // Used to store sphere indexes of best hits
    Float ray_t_max    = 1*Inf();  comment("ray_t_max"); // Is a parameter in reference app

    sphere_hit_partial(
			ray,
      N_spheres,
			sphere_ptr,
      sphere_index,
      ray_t_max
    );

		// Reload the sphere pointers for the next step
		sphere_ptr.load(in_center_x, in_center_y, in_center_z, in_radius);

    // Store best results
    hit_record_partial(
      ray_index,
      sphere_index,
      ray_t_max,
			ray,
			sphere_ptr,
      rec_p_x, rec_p_y, rec_p_z,
      rec_normal_x, rec_normal_y, rec_normal_z,
      rec_t,
      rec_front_face,
      rec_sphere_index
    );


#ifndef SINGLE_RAY
#ifdef BLOCK_READ
    End

    nop(1);                        sub_header("Update pointers ray_index block");
		ray_ptr.inc();
#else
    nop(1);                        sub_header("Update pointers ray_index loop");
		ray_ptr++;
#endif    

    nop(1);                        sub_header("End ray_index loop");
  End
#endif    

#undef BLOCK_READ
}

std::unique_ptr<BaseKernel> s_sphere_hit;


} // anon namespace

void init() {
  if (s_sphere_hit != nullptr) return;

  timers.start("kernel::init()");

  s_sphere_hit.reset(new BaseKernel(compile(sphere_hit_kernel)));
  to_file("sphere_hit_kernel.txt", s_sphere_hit->dump());

  timers.stop("kernel::init()");
}


void sphere_hit(
  int ray_index,
  int ray_num,
  Float::Array &in_origin_x,    Float::Array &in_origin_y,    Float::Array &in_origin_z,
  Float::Array &in_direction_x, Float::Array &in_direction_y, Float::Array &in_direction_z,
  int N_spheres,
  Float::Array &center_x, Float::Array &center_y, Float::Array &center_z,
  Float::Array &radius,
  Float::Array &rec_p_x, Float::Array &rec_p_y, Float::Array &rec_p_z,
  Float::Array &rec_normal_x, Float::Array &rec_normal_y, Float::Array &rec_normal_z,
  Float::Array &rec_t,
  Float::Array &rec_front_face,
  Int::Array   &rec_sphere_index
) {
  int sphere_blocks = resize_16(N_spheres) >> 4;

  s_sphere_hit->load(
    ray_index,
    ray_num,
    &in_origin_x,    &in_origin_y,    &in_origin_z,
    &in_direction_x, &in_direction_y, &in_direction_z,
    sphere_blocks,
    &center_x, &center_y, &center_z,
    &radius,
    &rec_p_x, &rec_p_y, &rec_p_z,
    &rec_normal_x, &rec_normal_y, &rec_normal_z,
    &rec_t,
    &rec_front_face,
    &rec_sphere_index
  ).run();
}

} // namespace kernel
