#include "qpu.h"
#include "V3DLib.h"

using namespace V3DLib;

namespace qpu {
namespace {

int image_width       = 0;
int image_height      = 0;
int samples_per_pixel = 0;

// Ray coordinates
Float::Array origin_x;
Float::Array origin_y;
Float::Array origin_z;

Float::Array direction_x;
Float::Array direction_y;
Float::Array direction_z;

}  // anon namespace


void init_arrays(int image_width, int image_height, int samples_per_pixel) {
/*
	warn << "init_arrays "
		   << "image_width: "       << image_width  << ", "
		   << "image_height: "      << image_height << ", "
		   << "samples_per_pixel: " << samples_per_pixel;
*/
	qpu::image_width       = image_width;
	qpu::image_height      = image_height;
	qpu::samples_per_pixel = samples_per_pixel;

	uint32_t size = image_width*image_height*samples_per_pixel;

	origin_x.alloc(size);
	origin_y.alloc(size);
	origin_z.alloc(size);

	direction_x.alloc(size);
	direction_y.alloc(size);
	direction_z.alloc(size);

	origin_x.fill(0.0f);
	origin_y.fill(0.0f);
	origin_z.fill(0.0f);

	direction_x.fill(0.0f);
	direction_y.fill(0.0f);
	direction_z.fill(0.0f);

	//warn << "origin_x size: " << origin_x.size();
}


int num_rays() {
	int size = image_width*image_height*samples_per_pixel;
	assert(size > 0);
	return size;
}



/**
 *
 * @param  r    Row index of ray
 * @param  c    column index of ray
 * @param  spp  Samples per pixel
 * @return      Index into Float arrays
 */
int set_ray(ray const &in_ray, int r, int c, int spp) {
	int index = (r*image_width +  c)*samples_per_pixel + spp;
	//warn << "set_ray index: " << index;

	assert(origin_x[index] == 0.0f);  // Assuming rest of arrays also zero

	origin_x[index] = (float) in_ray.origin().x();
	origin_y[index] = (float) in_ray.origin().y();
	origin_z[index] = (float) in_ray.origin().z();

	direction_x[index] = (float) in_ray.direction().x();
	direction_y[index] = (float) in_ray.direction().y();
	direction_z[index] = (float) in_ray.direction().z();

	return index;
}


ray get_ray(int index) {
	point3 origin(origin_x[index], origin_y[index], origin_z[index]);
	vec3 direction(direction_x[index], direction_y[index], direction_z[index]);

	return ray(origin, direction);
}

}  // namespace qpu


bool same(ray const &lhs, ray const &rhs) {
	//warn << "same lhs: " << lhs.dump();
	//warn << "same rhs: " << rhs.dump();
	auto diff = lhs.abs_diff(rhs);
	//warn << "diff: "     << diff.dump();

	double precision = 1.0e-7f;

	return diff.origin().near_zero(precision) && diff.direction().near_zero(precision);
}
