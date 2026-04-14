#include "bmp.h"
#include <cassert>
#include <cstdio>

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
