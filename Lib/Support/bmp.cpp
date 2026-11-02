#include "bmp.h"
#include "global/Log.h"
#include <cassert>
#include <cstdio>

using namespace Log;

Image::Image(int width, int height) : m_width(width), m_height(height), m_img(width, height) {
  assert(width > 0);
  assert(height > 0);
}


void Image::set_conversion_factor(double val) {
  m_factor = val;
}


void Image::set_color(unsigned rgb) {
  unsigned char r = (rgb >> 16) & 0xff;
  unsigned char g = (rgb >>  8) & 0xff;
  unsigned char b =  rgb        & 0xff;

  cur_color = {r, g, b};
}

/**
 * @brief Convert incoming coordinates to bitmap coordinates
 *
 * @return true if can draw, false if out of bounds
 */
bool Image::convert_coords(int &x, int &y, double in_x, double in_y) {
  x = (int) (in_x/m_factor*m_width);
  y = (int) (in_y/m_factor*m_height);

  // Assumption: (0,0) is in the centre of the image
  x += m_width/2;
  y += m_height/2;

  //warn << "(x,y): (" << x << ", " << y << ")";
  if (x < 0 || y < 0 || x >= m_width || y >= m_height) {
    //warn << "coords out of bounds, not drawing";
    return false;
  }
  assert(x < m_width);
  assert(y < m_height);

  return true;
}


void Image::plot(double in_x, double in_y) {
  int x, y;
  if (!convert_coords(x, y, in_x, in_y)) return;

  m_img.set_pixel(x, y, cur_color);
}


void Image::plot_rect(double in_x, double in_y, int width, int height) {
  assert(width > 0);
  assert(height > 0);

  int x, y;
  if (!convert_coords(x, y, in_x, in_y)) return;

  for (int i = 0; i < width; ++i) {
    for (int j = 0; j < height; ++j) {
      m_img.set_pixel(x + i, y + j, cur_color);
    }
  }
}


void Image::save(std::string const &filename) const {
  m_img.save_image(filename);
}  
