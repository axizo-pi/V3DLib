#include "kernel.h"
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

  void init() {
    x = 0;
    y = 0;
    z = 0;
  }

  void inner(Float &dst, Vector &rhs) {
    nop(1); comment("Vector inner");
    dst = x*rhs.x + y*rhs.y + z*rhs.z;
  }

  void sub(Vector &dst, Vector &rhs) {
    dst.x = x - rhs.x;
    dst.y = y - rhs.y;
    dst.z = z - rhs.z;
  }

  void length_squared(Float &dst) {
    nop(1); comment("Vector length_squared");
    dst = x*x + y*y + z*z;
  }

  void assign(Vector &rhs) {
    x = rhs.x;
    y = rhs.y;
    z = rhs.z;
  }

  void mult(Float &k) {
    nop(1); comment("Vector mult");
    x = k*x;
    y = k*y;
    z = k*z;
  }

  void element_at(Int &n, Vector &rhs) {
    nop(1); comment("Vector element_at");
    V3DLib::element_at(rhs.x,    n, x);
    V3DLib::element_at(rhs.y,    n, y);
    V3DLib::element_at(rhs.z,    n, z);
  }

  void set_at(Int &n, Vector &rhs) {
    nop(1); comment("Vector set_at");
    V3DLib::set_at(x,    n, rhs.x);
    V3DLib::set_at(y,    n, rhs.y);
    V3DLib::set_at(z,    n, rhs.z);
  }
};


struct RayPtr {
  Float::Ptr &origin_x;
  Float::Ptr &origin_y;
  Float::Ptr &origin_z;
  Float::Ptr &direction_x;
  Float::Ptr &direction_y;
  Float::Ptr &direction_z;

  RayPtr(
    Float::Ptr &p_origin_x,    Float::Ptr &p_origin_y,    Float::Ptr &p_origin_z,
    Float::Ptr &p_direction_x, Float::Ptr &p_direction_y, Float::Ptr &p_direction_z
  ) : 
    origin_x(p_origin_x),
    origin_y(p_origin_y),
    origin_z(p_origin_z),
    direction_x(p_direction_x),
    direction_y(p_direction_y),
    direction_z(p_direction_z)
  {
  }

  void inc() {
    nop(1); comment("RayPtr inc");
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

  void load(RayPtr &ptr) {
    nop(1);  sub_header("load RayPtr");

    origin.x    = *ptr.origin_x;
    origin.y    = *ptr.origin_y;
    origin.z    = *ptr.origin_z;
    direction.x = *ptr.direction_x;
    direction.y = *ptr.direction_y;
    direction.z = *ptr.direction_z;
  }


  void load(RayPtr &ptr, Int &ray_index) {
    nop(1);  sub_header("load RayPtr index");
    origin.x    = *(ptr.origin_x + ray_index);
    origin.y    = *(ptr.origin_y + ray_index);
    origin.z    = *(ptr.origin_z + ray_index);
    direction.x = *(ptr.direction_x + ray_index);
    direction.y = *(ptr.direction_y + ray_index);
    direction.z = *(ptr.direction_z + ray_index);
  }

  void element_at(Int &n, Ray &rhs) {
    origin.   element_at(n, rhs.origin);
    direction.element_at(n, rhs.direction);
  }    

  void set_at(Int &n, Ray &rhs) {
    nop(1);  sub_header("set_at");

    origin.   set_at(n, rhs.origin);
    direction.set_at(n, rhs.direction);
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
    nop(1);                                      sub_header("Load SpherePtr");
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


struct HitRecordPtr {
  Int::Ptr   &sphere_index;
  Float::Ptr &p_x;
  Float::Ptr &p_y;
  Float::Ptr &p_z;
  Float::Ptr &normal_x;
   Float::Ptr &normal_y;
  Float::Ptr &normal_z;
  Float::Ptr &t;
  Float::Ptr &front_face;

  HitRecordPtr(
    Int::Ptr   &rec_sphere_index,
    Float::Ptr &rec_p_x, Float::Ptr &rec_p_y, Float::Ptr &rec_p_z,
    Float::Ptr &rec_normal_x, Float::Ptr &rec_normal_y, Float::Ptr &rec_normal_z,
    Float::Ptr &rec_t,
    Float::Ptr &rec_front_face
  ) :
    sphere_index(rec_sphere_index),
    p_x(rec_p_x),
    p_y(rec_p_y),
    p_z(rec_p_z),
    normal_x(rec_normal_x),
    normal_y(rec_normal_y),
    normal_z(rec_normal_z),
    t(rec_t),
    front_face(rec_front_face)
    {}
};


struct HitRecord {
  Int    sphere_index;
  Vector p;
  Vector outward_normal;
  Float  t;
  Float  front_face;

  void init() {
    sphere_index = -1;
    p.init();
    outward_normal.init();
    t = -1;
    front_face = -1;
  }

  void store(HitRecordPtr &ptr, Int &offset) {
    nop(1);                                            sub_header("Store HitRecord");
    *(ptr.sphere_index + offset) = sphere_index;
    *(ptr.p_x        + offset) = p.x;
    *(ptr.p_y        + offset) = p.y;
    *(ptr.p_z        + offset) = p.z;
    *(ptr.normal_x   + offset) = outward_normal.x;
    *(ptr.normal_y   + offset) = outward_normal.y;
    *(ptr.normal_z   + offset) = outward_normal.z;
    *(ptr.t          + offset) = t;
    *(ptr.front_face + offset) = front_face;
  }


  void set_at(Int &n, HitRecord &rhs) {
    nop(1);  sub_header("set_at");

    V3DLib::set_at(sphere_index, n, rhs.sphere_index);
    p.set_at(n, rhs.p);
    outward_normal.set_at(n, rhs.outward_normal);
    V3DLib::set_at(t, n, rhs.t);
    V3DLib::set_at(front_face, n, rhs.front_face);
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
  HitRecord &hr
) {
  nop(1);             sub_header("Start hit_record_partial");

  hr.t = -1;
  hr.front_face = -1.0f;

  // Get the best t of the values in the 16-vector `in_t`.
  // This should be the lowest value.
  Int min_index;
  rotate_min(in_t, hr.t, min_index);
  element_at(in_sphere_index, min_index, hr.sphere_index);

  // rec.p = r.at(rec.t);
  r.at(hr.p, hr.t);                          comment("Start ray.at()");

  Int sphere_offset = hr.sphere_index - index();
  Sphere sphere;
  sphere.load(sphere_ptr, sphere_offset);

  //vec3 outward_normal = (rec.p - m_center) / m_radius;
  sphere.normal(hr.outward_normal, hr.p);

  // rec.set_face_normal(r, outward_normal);
  //
  // This sets the sign for the normal vector and stores it in rec.normal.
  //
  Float tmp;
  r.direction.inner(tmp, hr.outward_normal);

  // NOTE: minus sign is the other way around as I would expect; counter-intuitive but correct.
  Where (tmp < 0)
    hr.front_face = 1.0f;
  End

  hr.outward_normal.mult(hr.front_face);
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
    sphere.center.sub(oc, r.origin);

    //auto a = r.direction().length_squared();
    Float a;
    r.direction.length_squared(a);

    //auto h = f_dot(r.direction(), oc);
    Float h;
    r.direction.inner(h, oc);

    //auto c = oc.length_squared() - m_radius*m_radius;
    Float c;
    oc.length_squared(c);
    c -= sphere.radius*sphere.radius;

    //auto discriminant = h*h - a*c;
    Float discriminant = h*h - a*c;

    // if (discriminant < 0) return false;
    Where (discriminant < 0.0f)  // `<=` leads to differences
      valid = 0;
    End

    // auto  std::sqrt(discriminant);
    Float sqrtd  = 0.0f;                         sub_header("Start test root");
    Float root   = 0.0f;

    Where (valid == 1)
      sqrtd = sqrt_f(discriminant);
      // Find the nearest root that lies in the acceptable range.
      // auto root = (h - sqrtd) / a;
      root = (h - sqrtd) / a;

      // if (!ray_t.surrounds(root))
      Where (!(ray_t_min < root && root < ray_t_max))
      	// auto root = (h + sqrtd) / a;
      	root = (h + sqrtd) / a;

        //if (!ray_t.surrounds(root)) return false;
        Where (!(ray_t_min < root && root < ray_t_max))
          valid = 0;
        End
      End
    End

    Where (valid == 1)
      ray_t_max = root;
      sphere_index = (i << 4) + index();
    End

    sphere_ptr.inc();                   comment("Increment pointers");
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
 *
 * **NOTE:** For BLOCK_WRITE, the current implementation uses _all_ registers on v3d.
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

#define BLOCK_WRITE

  RayPtr ray_ptr(
    p_origin_x,    p_origin_y,    p_origin_z,
    p_direction_x, p_direction_y, p_direction_z
  );

  HitRecordPtr hr_ptr(
    rec_sphere_index,
    rec_p_x, rec_p_y, rec_p_z,
    rec_normal_x, rec_normal_y, rec_normal_z,
    rec_t,
    rec_front_face
  );

  nop(1);                                        sub_header("Start ray_index loop");
  const Int ray_blocks = ray_num >> 4;

#ifdef BLOCK_WRITE
  HitRecord hr_total;
  hr_total.init();
#endif  

  For (Int ray_block = 0, ray_block < ray_blocks, ray_block++)
    Ray in_ray;
    in_ray.load(ray_ptr);

    nop(1);                                      sub_header("Start index element loop");
    const Int n_max = 16;

    For (Int n = 0, n < n_max, n++)
      Int ray_index = n_max*ray_block + n;

      Ray ray;
      ray.element_at(n, in_ray);

      SpherePtr sphere_ptr;
      sphere_ptr.load(in_center_x, in_center_y, in_center_z, in_radius);

      Int   sphere_index = -1;       // Used to store sphere indexes of best hits
      Float ray_t_max    = 1*Inf();  // Is a parameter in reference app. `1*` is a workaround.

      sphere_hit_partial(
        ray,
        N_spheres,
        sphere_ptr,
        sphere_index,
        ray_t_max
      );

      // Reload the sphere pointers for the next step
      sphere_ptr.load(in_center_x, in_center_y, in_center_z, in_radius);

      // Determine best results
      HitRecord hr;

      hit_record_partial(
        ray_index,
        sphere_index,
        ray_t_max,
        ray,
        sphere_ptr,
        hr
      );

#ifdef BLOCK_WRITE
      hr_total.set_at(n, hr);
#else
      // Store best result
      //
      // `- index()` to save to a single location in main mem
      Int offset = ray_index - index();
      hr.store(hr_ptr, offset);
#endif      
    End

#ifdef BLOCK_WRITE
    Int ray_index = n_max*ray_block;
    hr_total.store(hr_ptr, ray_index);
#endif

    ray_ptr.inc();  comment("Update pointers ray_index loop");
  End
}

std::unique_ptr<BaseKernel> s_sphere_hit;

} // anon namespace


void init() {
  if (s_sphere_hit != nullptr) return;

  timers.start("kernel::init()");

  s_sphere_hit.reset(new BaseKernel(compile(sphere_hit_kernel)));
  to_file("sphere_hit_kernel.txt", s_sphere_hit->dump());
  to_file("sphere_compile_data.txt", s_sphere_hit->dump_compile_data());

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
