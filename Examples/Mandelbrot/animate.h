#ifndef _INCLUDE_MANDELBROT_ANIMATE
#define _INCLUDE_MANDELBROT_ANIMATE

struct MandRange;

namespace animate {

void init(MandRange const &in_start);
MandRange next();
bool done();

} // namespace animate


#endif // _INCLUDE_MANDELBROT_ANIMATE
