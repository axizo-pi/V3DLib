#include "kernel.h"
#include "Source/GlobalConstants.h"
#include "Support/Helpers.h"
#include "Support/Timer.h"
#include <memory>

namespace kernel {
namespace {

/**
 * @brief return the nearest sphere hit for the current ray.
 */
void hit_record_partial(
  Int   &ray_index,
  Int   &in_sphere_index,
  Float &in_t,
  Float &origin_x,    Float &origin_y,    Float &origin_z,
  Float &direction_x, Float &direction_y, Float &direction_z,
  Float::Ptr &in_center_x, Float::Ptr &in_center_y, Float::Ptr &in_center_z,
  Float::Ptr &in_radius,
  // Output parameters
  Float::Ptr &rec_p_x, Float::Ptr &rec_p_y, Float::Ptr &rec_p_z,
  Float::Ptr &rec_normal_x, Float::Ptr &rec_normal_y, Float::Ptr &rec_normal_z,
  Float::Ptr &rec_t,
  Float::Ptr &rec_front_face,
  Int::Ptr   &rec_sphere_index
) {
  Float t;

  // Get the best t of the values in the 16-vector `in_t`.
  // This should be the lowest value.
  Int min_index;
  rotate_min(in_t, t, min_index);
  Int sphere_index;
  element_at(in_sphere_index, min_index, sphere_index);

  // rec.p = r.at(rec.t);
  Float p_x = origin_x + (t*direction_x);                     comment("Start ray.at()");
                                                              sub_header("Update rec");
  Float p_y = origin_y + (t*direction_y);
  Float p_z = origin_z + (t*direction_z);

  //vec3 outward_normal = (rec.p - m_center) / m_radius;
  Int sphere_offset = sphere_index - index();
  Float center_x = *(in_center_x + sphere_offset);
  Float center_y = *(in_center_y + sphere_offset);
  Float center_z = *(in_center_z + sphere_offset);
  Float radius   = *(in_radius   + sphere_offset);

  Float outward_normal_x = (p_x - center_x) / radius;     comment("Calc outward_normal");
  Float outward_normal_y = (p_y - center_y) / radius;
  Float outward_normal_z = (p_z - center_z) / radius;

  // rec.set_face_normal(r, outward_normal);
  //
  // This sets the sign for the normal vector and stores it in rec.normal.
  //
  Float tmp = direction_x*outward_normal_x
            + direction_y*outward_normal_y
            + direction_z*outward_normal_z;

  // NOTE: minus sign is the other way around as I would expect; counter-intuitive but correct.
  Float front_face = -1.0f;
  Where (tmp < 0)
    front_face = 1.0f;
  End

  // `- index()` to save to a single location in main mem
  Int offset = ray_index - index();

  *(rec_p_x        + offset) = p_x;
  *(rec_p_y        + offset) = p_y;
  *(rec_p_z        + offset) = p_z;
  *(rec_normal_x   + offset) = front_face*outward_normal_x;
  *(rec_normal_y   + offset) = front_face*outward_normal_y;
  *(rec_normal_z   + offset) = front_face*outward_normal_z;
  *(rec_t          + offset) = t;
  *(rec_front_face + offset) = front_face;
  *(rec_sphere_index + offset) = sphere_index;
}


void sphere_hit_partial(
  Float &origin_x   , Float &origin_y   , Float &origin_z,
  Float &direction_x, Float &direction_y, Float &direction_z,
  Int &N_spheres,
  Float::Ptr &in_center_x, Float::Ptr &in_center_y, Float::Ptr &in_center_z,
  Float::Ptr &in_radius,
  // Internal variables
  Int &sphere_index,
  Float &ray_t_max,
  // Debug output
  Float::Ptr &ret_x, Float::Ptr &ret_y, Float::Ptr &ret_z,
  Float::Ptr &ret_f,
  Int::Ptr   &ret_valid
) {
  Float ray_t_min  = 0.001f;

  // Make copies of pointers, they are also used after the loop
  Float::Ptr p_center_x = in_center_x;
  Float::Ptr p_center_y = in_center_y;
  Float::Ptr p_center_z = in_center_z;
  Float::Ptr p_radius   = in_radius;

  For (Int i = 0, i < N_spheres, i++)
    Int valid = 1;

    Float center_x = *p_center_x;                                 comment("Start sphere loop");
    Float center_y = *p_center_y;
    Float center_z = *p_center_z;
    Float radius   = *p_radius;

    // Exclude items added to resize to multiple of 16 blocks
    Where (radius == 0.0f)
      valid = 0;
    End

    // vec3 oc = m_center - r.origin();
    Float oc_x = center_x - origin_x;                              comment("vec3 oc");
    Float oc_y = center_y - origin_y;
    Float oc_z = center_z - origin_z;

    //auto a = r.direction().length_squared();
    Float dir_x = direction_x;                                     comment("auto a");
    Float dir_y = direction_y;
    Float dir_z = direction_z;

    Float a = dir_x*dir_x + dir_y*dir_y + dir_z*dir_z;             comment("Float a");
    //*ret_f = a;

    //auto h = f_dot(r.direction(), oc);
    Float h = dir_x*oc_x + dir_y*oc_y + dir_z*oc_z;                comment("Float h");
    //*ret_f = h;

    //auto c = oc.length_squared() - m_radius*m_radius;
    Float c = (oc_x*oc_x + oc_y*oc_y + oc_z*oc_z) - radius*radius; comment("Float c");
    //*ret_f = c;

    //auto discriminant = h*h - a*c;
    Float discriminant = h*h - a*c;                                comment("Float discriminant");
    *ret_f = discriminant;

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
      ray_t_max = root;
      sphere_index = 16*i + index();
    End

    // Debug output
    //*ret_f = sqrtd;
    *ret_f = root;
    //*ret_f = root_2;
    //*ret_x = rec_normal_x;
    //*ret_y = rec_normal_y;
    //*ret_z = rec_normal_z;

    *ret_valid = valid;

    p_center_x.inc();    header("Start increment pointers");
    p_center_y.inc();
    p_center_z.inc();
    p_radius.inc();

    // Increment debug pointers
    ret_x.inc();
    ret_y.inc();
    ret_z.inc();
    ret_f.inc();
    ret_valid.inc();
  End
}


/**
 * @brief Get the nearest hit for the given ray.
 *
 * All spheres are checked for a hit. The best hit, if any, is returned.
 *
 * A single ray is checked. The 16-vectors contain consecutive spheres.
 *
 * A bad hit can be detected by checking the coordinates of `rec_p_*`; a failed
 * hit has Inf coordinates.
 */
void sphere_hit_kernel(
  // Input values
  Int ray_index,
  Float origin_x, Float origin_y, Float origin_z,
  Float direction_x, Float direction_y, Float direction_z,
  Int N_spheres, // Blocks of 16
  Float::Ptr in_center_x, Float::Ptr in_center_y, Float::Ptr in_center_z,
  Float::Ptr in_radius,
  // Output values
  Float::Ptr rec_p_x, Float::Ptr rec_p_y, Float::Ptr rec_p_z,
  Float::Ptr rec_normal_x, Float::Ptr rec_normal_y, Float::Ptr rec_normal_z,
  Float::Ptr rec_t,
  Float::Ptr rec_front_face,
  Int::Ptr   rec_sphere_index,
  // Debug output
  Float::Ptr ret_x, Float::Ptr ret_y, Float::Ptr ret_z,
  Float::Ptr ret_f,
  Int::Ptr   ret_valid
) {
  Int sphere_index = -1;
  Float ray_t_max  = Inf();    // Is a parameter in reference app

  sphere_hit_partial(
    origin_x, origin_y, origin_z,
    direction_x, direction_y, direction_z,
    N_spheres,
    in_center_x, in_center_y, in_center_z,
    in_radius,
    sphere_index,
    ray_t_max,
    ret_x, ret_y, ret_z,
    ret_f,
    ret_valid
  );

  // Store best results
  hit_record_partial(
    ray_index,
    sphere_index,
    ray_t_max,
    origin_x, origin_y, origin_z,
    direction_x, direction_y, direction_z,
    in_center_x, in_center_y, in_center_z,
    in_radius,
    rec_p_x, rec_p_y, rec_p_z,
    rec_normal_x, rec_normal_y, rec_normal_z,
    rec_t,
    rec_front_face,
    rec_sphere_index
  );
}

std::unique_ptr<BaseKernel> s_sphere_hit;


} // anon namespace

void init() {
  if (s_sphere_hit != nullptr) return;

  timers.start("kernel::init()");

  s_sphere_hit.reset(new BaseKernel(compile(sphere_hit_kernel))); //, settings())));
  //to_file("sphere_hit_kernel.txt", s_sphere_hit->dump());

  timers.stop("kernel::init()");
}


void sphere_hit(
  ray const &r, int ray_index,
  int N_spheres,
  Float::Array &center_x, Float::Array &center_y, Float::Array &center_z,
  Float::Array &radius,
  Float::Array &rec_p_x, Float::Array &rec_p_y, Float::Array &rec_p_z,
  Float::Array &rec_normal_x, Float::Array &rec_normal_y, Float::Array &rec_normal_z,
  Float::Array &rec_t,
  Float::Array &rec_front_face,
  Int::Array   &rec_sphere_index,
  Float::Array &ret_x, Float::Array &ret_y, Float::Array &ret_z,
  Float::Array &ret_f,
  Int::Array   &ret_valid
) {
  //warn << "sphere_hit N_spheres: " << N_spheres;

  float o_x = (float) r.origin().x();
  float o_y = (float) r.origin().y();
  float o_z = (float) r.origin().z();

  float d_x = (float) r.direction().x();
  float d_y = (float) r.direction().y();
  float d_z = (float) r.direction().z();

  int sphere_blocks = resize_16(N_spheres) >> 4;

  s_sphere_hit->load(
    ray_index,
    o_x, o_y, o_z,
    d_x, d_y, d_z,
    sphere_blocks,
    &center_x, &center_y, &center_z,
    &radius,
    &rec_p_x, &rec_p_y, &rec_p_z,
    &rec_normal_x, &rec_normal_y, &rec_normal_z,
    &rec_t,
    &rec_front_face,
    &rec_sphere_index,
    &ret_x, &ret_y, &ret_z,
    &ret_f,
    &ret_valid
  ).run();

  // Checking kernel values origin (kernel adjusted)
  // All tests xyz and index exact
  //bitdiff_stats::add(ret_x[256], (float) r.origin().x(), 100);
}

} // namespace kernel
