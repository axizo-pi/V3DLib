#ifndef _EXAMPLE_SUPPORT_BMP_H
#define _EXAMPLE_SUPPORT_BMP_H
#include "bitmap_image.hpp"

/*
class PGM {
public:
  PGM(int width, int height);
  ~PGM();

  PGM &plot(float const *arr, int size, int color = MAX_COLOR);
  PGM &plot(V3DLib::Float::Array const &arr, int color = MAX_COLOR);
  PGM &plot(std::vector<float> const arr, int color = MAX_COLOR) { return plot(arr.data(), (int) arr.size(), color); }
  void save(char const *filename);

private:
  static int const MAX_COLOR = 127;

  int m_width  = 0;
  int m_height = 0;
  int *m_arr   = nullptr;
};
*/


template<class Array>
void output_bmp(
	Array &arr,
	int width,
	int height,
	int in_max_value,
	const char *filename,
	bool do_color
) {
	const int PaletteSize  = 1000;
  int const RGBLimit     = 255;        // largest allowed value in pgm file is 65536 (-1?)
  int const LINEAR_LIMIT = 128;        // Use log scale above this number of iterations
  bool do_log            = (in_max_value > LINEAR_LIMIT);
  float max_value        = (float) in_max_value;

  if (do_log) {
    max_value = (float) log2((float) in_max_value);
  }
  assert(max_value >= 1.0f);


  auto scale = [do_log, in_max_value, max_value] (float value, int out_size) -> int {
  	float factor           = 1.0f;
  	factor = ((float) out_size)/max_value;

    if (value  <= 0) return 0;
    if (value  >= (float) in_max_value) return 0;

    if (do_log) {
      value = (float) log2(value);
    }

    int ret = (int) (factor*value);
    if (ret <= 0) return 0;
    if (ret > out_size) ret = out_size;

    return ret;
  };


	bitmap_image img(width, height);

	for (int y = 0; y < height; ++y) {
 		for (int x = 0; x < width; ++x) {

			// Following was in mandelbrot example of bitmap project
			//const unsigned int index = static_cast<unsigned int>
      //            (1000.0 * log2(1.75 + i - log2(log2(z))) / log2(max_iterations));

			auto val    = arr[x + width*y];
			rgb_t color = {0, 0, 0};

			unsigned index;
			if (do_color) {
				if (val != 0 && val != (float) in_max_value) {
  				index = scale((float) val, PaletteSize);
					color = jet_colormap[index];
				}
			} else {
  			unsigned char rgb = (unsigned char) scale((float) val, RGBLimit);
				color = {rgb, rgb, rgb};
			}


			img.set_pixel(x, y, color);
		}
	}

	img.save_image(filename);
}

#endif  // _EXAMPLE_SUPPORT_BMP_H
