#include "defaults.h"
#include "animate.h"
#include "settings.h"
#include "global/Log.h"

using namespace Log;

//
// Status for animate
//
namespace {

const int ANIMATE_SKIP = 720;  //> Number of days to skip for every animate output
int animate_count      = 0;    //> Number of times `save_animate()` is called
int s_index            = 0;    //> Prefix index for animate

}  // anon namespace


/**
 * @brief If specified, output the next animation image
 *
 * Output of animations image is optional, depends on settings.
 *
 * @param force If true, output animate regardles of `animate_count`.
 *              This is to output the final image.
 *              It is possible that the final image is the same as the previous image, soit.
 */
void save_animate(std::function<void(Image &img)> plot_image, bool force) {
  if (!settings.do_animate) return;

  if (force || (animate_count >= ANIMATE_SKIP)) {
    std::string filename;
    filename << s_index << "_gravity.bmp";
    //info << "Model outputting animate to: " << filename;

    Image img{512, 512};
    img.set_conversion_factor(IMG_CONVERSION_FACTOR);
    plot_image(img);
    img.save(filename);
    s_index++;
  }

  if (!force) {
    if (animate_count >= ANIMATE_SKIP) {
      animate_count -= ANIMATE_SKIP;
    }

    animate_count += batch_steps();
  }
}
