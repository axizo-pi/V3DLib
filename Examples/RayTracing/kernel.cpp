#include "kernel.h"
#include "global.h"  // SINGLE_RAY
#include "Source/GlobalConstants.h"
#include "Support/Helpers.h"
#include "Support/Timer.h"

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
  nop(1);             sub_header("Start hit_record_partial");
  Float t = -1;

  // Get the best t of the values in the 16-vector `in_t`.
  // This should be the lowest value.
  Int min_index;
  rotate_min(in_t, t, min_index);
  Int sphere_index;
  element_at(in_sphere_index, min_index, sphere_index);

  // rec.p = r.at(rec.t);
  Float p_x = origin_x + (t*direction_x);    sub_header("Update rec"); comment("Start ray.at()");
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

  *(rec_sphere_index + offset) = sphere_index;
  *(rec_p_x        + offset) = p_x;
  *(rec_p_y        + offset) = p_y;
  *(rec_p_z        + offset) = p_z;
  *(rec_normal_x   + offset) = front_face*outward_normal_x;
  *(rec_normal_y   + offset) = front_face*outward_normal_y;
  *(rec_normal_z   + offset) = front_face*outward_normal_z;
  *(rec_t          + offset) = t;
  *(rec_front_face + offset) = front_face;
}


void sphere_hit_partial(
  Float &origin_x   , Float &origin_y   , Float &origin_z,
  Float &direction_x, Float &direction_y, Float &direction_z,
  Int &N_spheres,
  Float::Ptr &in_center_x, Float::Ptr &in_center_y, Float::Ptr &in_center_z,
  Float::Ptr &in_radius,
  // Internal variables
  Int &sphere_index,
  Float &ray_t_max
) {
  nop(1);             sub_header("Start sphere_hit_partial");
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

    //auto h = f_dot(r.direction(), oc);
    Float h = dir_x*oc_x + dir_y*oc_y + dir_z*oc_z;                comment("Float h");

    //auto c = oc.length_squared() - m_radius*m_radius;
    Float c = (oc_x*oc_x + oc_y*oc_y + oc_z*oc_z) - radius*radius; comment("Float c");

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

    p_center_x.inc();    header("Start increment pointers");
    p_center_y.inc();
    p_center_z.inc();
    p_radius.inc();
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

#define BLOCK_READ

#ifndef BLOCK_READ
  nop(1);                        sub_header("Adjust point pointers");
  Int offset = index()*-4;
  p_origin_x.offset(   offset);  comment("p_origin_x");
  p_origin_y.offset(   offset);  comment("p_origin_y");
  p_origin_z.offset(   offset);
  p_direction_x.offset(offset);
  p_direction_y.offset(offset);
  p_direction_z.offset(offset);  comment("End adjust point pointers");
#endif  


#ifdef SINGLE_RAY
  warn << "SINGLE_RAY defined kernel";

  Int ray_index = ray_dummy;

  Float origin_x    = *(p_origin_x + ray_index);
  Float origin_y    = *(p_origin_y + ray_index);
  Float origin_z    = *(p_origin_z + ray_index);
  Float direction_x = *(p_direction_x + ray_index);
  Float direction_y = *(p_direction_y + ray_index);
  Float direction_z = *(p_direction_z + ray_index);
#else
  nop(1);                        sub_header("Start ray_index loop");

#ifdef BLOCK_READ
  const Int ray_blocks = ray_num >> 4;

  For (Int ray_block = 0, ray_block < ray_blocks, ray_block++)
    Float in_origin_x    = *p_origin_x;
    Float in_origin_y    = *p_origin_y;
    Float in_origin_z    = *p_origin_z;
    Float in_direction_x = *p_direction_x;
    Float in_direction_y = *p_direction_y;
    Float in_direction_z = *p_direction_z;

    Float origin_x;
    Float origin_y;
    Float origin_z;
    Float direction_x;
    Float direction_y;
    Float direction_z;

    nop(1);                                      sub_header("Start index element loop");
    const Int n_max = 16;

    For (Int n = 0, n < n_max, n++)
      Int ray_index = n_max*ray_block + n;
      element_at(in_origin_x, n, origin_x);      comment("element origin_x");
      element_at(in_origin_y, n, origin_y);      comment("element origin_y");
      element_at(in_origin_z, n, origin_z);
      element_at(in_direction_x, n, direction_x);
      element_at(in_direction_y, n, direction_y);
      element_at(in_direction_z, n, direction_z);
#else  
  For (Int ray_index = 0, ray_index < ray_num, ray_index++)
    Float origin_x    = *p_origin_x;
    Float origin_y    = *p_origin_y;
    Float origin_z    = *p_origin_z;
    Float direction_x = *p_direction_x;
    Float direction_y = *p_direction_y;
    Float direction_z = *p_direction_z;
#endif  
#endif  // SINGLE_RAY

    Int   sphere_index = -1;       comment("sphere_index"); // Used to store sphere indexes of best hits
    Float ray_t_max    = 1*Inf();  comment("ray_t_max"); // Is a parameter in reference app
    Float dummy = 123;             comment("Dummy load");

    sphere_hit_partial(
      origin_x, origin_y, origin_z,
      direction_x, direction_y, direction_z,
      N_spheres,
      in_center_x, in_center_y, in_center_z,
      in_radius,
      sphere_index,
      ray_t_max
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


#ifndef SINGLE_RAY
#ifdef BLOCK_READ
    End

    nop(1);                        sub_header("Update pointers ray_index block");
    p_origin_x.inc();
    p_origin_y.inc();
    p_origin_z.inc();
    p_direction_x.inc();
    p_direction_y.inc();
    p_direction_z.inc();
#else
    nop(1);                        sub_header("Update pointers ray_index loop");
    p_origin_x++;
    p_origin_y++;
    p_origin_z++;
    p_direction_x++;
    p_direction_y++;
    p_direction_z++;
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
  //to_file("sphere_hit_kernel.txt", s_sphere_hit->dump());

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
