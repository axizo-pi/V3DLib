#include "bmp.h"
#include <cassert>
#include <cstdio>
#include "global/log.h"

using namespace Log;

/*
///////////////////////////////////////////////////////////////////////////////
// Class Color
///////////////////////////////////////////////////////////////////////////////

std::string Color::disp() const {
  std::string ret;
  ret << red << " " << green << " " << blue;
  return ret;
}


Color Color::scale(float factor) const {
  if (!(0 <= factor && factor <= 1)) {
    printf("factor: %f\n", factor);
  }
  assert(0 <= factor && factor <= 1);

  auto adjust = [factor] (int val) -> int {
   return (int) (((float) val)*factor);
  };

  return Color(adjust(red), adjust(green), adjust(blue));
}


Color Color::invert() const {
  return Color(255 - red, 255 - green, 255 - blue);
}


Color Color::operator+(Color const &rhs) const {
  return Color(rhs.red + red, rhs.green + green, rhs.blue + blue);
}


///////////////////////////////////////////////////////////////////////////////
// Class ColorMap
///////////////////////////////////////////////////////////////////////////////

Color ColorMap::calc(int value) {
  assert(0 <= value && value <= max_intensity);
  float fraction = ((float) value)/((float) max_intensity);

  if (value == max_intensity) return Color(0);

  if (fraction <= peak) {
    return main_color.scale(fraction/peak);
  } else {
    float fraction2 = (fraction - peak)/(1 - peak);
    return main_color + main_color.invert().scale(fraction2);
  }
}


///////////////////////////////////////////////////////////////////////////////
// File output function(s) 
///////////////////////////////////////////////////////////////////////////////

/ **
 * These format limits need to be taken into account:
 *
 * - Max line length of 70 characters.
 * - Max gray value is 65536
 * - Max color value is 255 (standard in file header)
 * /
void output_ppm_file(
  std::string const &header,
  int width,
  int height,
  const char *filename,
  std::function<std::string (int)> f) {
  assert(!header.empty());
  int const LINE_LIMIT = 70;

  FILE *fd = fopen(filename, "w") ;
  if (fd == nullptr) {
    printf("can't open file for graphics output\n");
    return;
  }

  fprintf(fd, "%s\n", header.c_str());

  std::string curLine;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      std::string tmp = f(x + width*y);

      std::string newLine = curLine;
      if (!newLine.empty()) {
        newLine << " ";
      }
      newLine << tmp;

      if (newLine.length() > LINE_LIMIT) {
        fprintf(fd, "%s\n", curLine.c_str());
        curLine = tmp;
      } else {
        curLine = newLine;
      }
    }
  }

  if (!curLine.empty()) {
    fprintf(fd, "%s\n", curLine.c_str());
  }

  fclose(fd);
}


///////////////////////////////////////////////////////////////////////////////
// Class PGM
///////////////////////////////////////////////////////////////////////////////

PGM::PGM(int width, int height) : m_width(width), m_height(height) {
  assert(m_width  != 0);
  assert(m_height != 0);

  m_arr = new int [width*height];    
  for (int x = 0; x < width*height; ++x) {
    m_arr[x] = 0;
  };
}


PGM::~PGM() {
  if (m_arr == nullptr) {
    assert(m_width  == 0);
    assert(m_height == 0);
  } else {
    assert(m_width  != 0);
    assert(m_height != 0);
  }

  delete [] m_arr;
}


PGM &PGM::plot(float const *arr, int size, int color) {
  assert(arr != nullptr);
  assert(size > 0);
  assert(size <= m_width);

  const float MIN_START =  1.0e20f;
  const float MAX_START = -1.0e20f;

  // determine min and max
  float min = MIN_START;
  float max = MAX_START;

  for (int x = 0; x < size; ++x) {
    //assert(arr[x] < MIN_START);
    //assert(arr[x] > MAX_START);

    if (arr[x] < min) min = arr[x];
    if (arr[x] > max) max = arr[x];
  };

  assert(min != MIN_START);
  assert(max != MAX_START);

  // Do the plot
  for (int x = 0; x < m_width; ++x) {
    // NOTE: there is an off by 1 error here somewhere,
    //       top (y == 0) leaves 1 row open, bottom gets clipped
    int y =  m_height - (int) ((arr[x] - min)/(max - min)*((float) m_height));

		if (y >= m_height) {
			// clip if out of bounds
			cdebug << "pgm y value " << y << " >= " << m_height << " out of bounds, clipped";
			y = m_height -1;
		}

    m_arr[x + y*m_width] = color;
  }

  return *this;
}


PGM &PGM::plot(V3DLib::Float::Array const &arr, int color) {
  return plot(arr.ptr(), arr.size(), color);
}


void PGM::save(char const *filename) {
  assert(filename != nullptr);

  std::string header;
  header << "P2\n"
         << m_width << " " << m_height << "\n"
         << MAX_COLOR << "\n";


  int *arr = m_arr;

  output_ppm_file(header, m_width, m_height, filename, [arr] (int index) -> std::string {
    std::string ret;
    ret << arr[index];
    return ret;
  });
}

*/


Image::Image(int width, int height) : m_width(width), m_height(height), m_img(width, height) {
	assert(width > 0);
	assert(height > 0);
}


void Image::set_conversion_factor(double val) {
	m_factor = val;
}


void Image::plot(double in_x, double in_y) {
	// Convert to bitmap coordinates
	int x = (int) (in_x/m_factor*m_width);
	int y = (int) (in_y/m_factor*m_height);

	// Assumption: (0,0) is in the centre of the image
	x += m_width/2;
	y += m_height/2;

	//warn << "(x,y): (" << x << ", " << y << ")";
	if (x < 0 || y < 0 || x >= m_width || y >= m_height) {
		//warn << "coords out of bounds, not drawing";
		return;
	}
	assert(x < m_width);
	assert(y < m_height);

	rgb_t color = {255, 255, 255};
	m_img.set_pixel(x, y, color);
}


void Image::save(std::string const &filename) const {
	m_img.save_image(filename);
}	
