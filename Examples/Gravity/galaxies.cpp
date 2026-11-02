/**
 * /file
 *
 * Derived from: https://github.com/rtoal/galaxies/blob/master/galaxies.js
 * **TODO:** Consider adding licence info; is it necessary?
 */
#include "galaxies.h"
#include "Vector3.h"
#include "global/Log.h"


namespace galaxies {
namespace {

float random() {
  return (float) rand() / (float) RAND_MAX;
}

// Original colors are not very distinctive, replacing these.
const unsigned colors[12] = {
  0x0404cc, 0x04cc04, 0xcc0404,  /*0xffffcc, 0xccffff, 0xffccff, */
  0xccffcc, 0xccccff, 0xffcccc,
  0x80ffbf, 0xbf80ff, 0xffbf80, 0xbfff80, 0x80bfff, 0xff80bf
};

//
// Default values, taken from original
//
const int   GALAXY_COUNT = 3;     // Number of galaxies to start up with
const int   MAX_STARS    = 700;   // Amount of stars in a galaxy
const float RANGE_SIZE   = 0.1f;
const float MIN_SIZE     = 0.1f;

}  // anon namespace


class Universe;  // Forward declaration
class Galaxy;    // Forward declaration


/*
 * A star belongs to a galaxy and has a size, pos, and vel.
 */
class Star {
public:
  Star(Galaxy const &galaxy);

  float pos[3];
  float vel[3];
  float size;
};


/*
 * A galaxy exists within a universe and has a mass, size, vel, pos, mat, and stars.
 */
class Galaxy {
public:
  float mat[3][3];
  float pos[3];
  float vel[3];
  std::vector<Star> stars;

  Galaxy(Universe const &universe, int num_stars = MAX_STARS);

  float mass() const { return m_mass;  }
  float size() const { return m_size; }

private:
  float m_mass;
  float m_size;
};


/*
 * A universe contains the galaxies, and some global-ish stuff.
 */
class Universe {
public:  
  float deltat;
  int hitIterations;
  std::vector<Galaxy> galaxies;

  Universe(int galaxyCount) {
    reset(galaxyCount);
  }

  void reset(int galaxyCount) {
    hitIterations = 100;
    deltat = 0.005f;

    galaxies.clear();

    for (int i = 0; i < galaxyCount; i++) {
      galaxies.push_back(Galaxy(*this, 2000));
    }
  }

  static int MAX_LIFETIME();
};

int Universe::MAX_LIFETIME() { return 800; }


Star::Star(Galaxy const &galaxy) {
  const float w = 2 * (float) M_PI * random();  // M_PI is double
  const float d = random() * galaxy.size();
  const float dsinw = d * std::sin(w);
  const float dcosw = d * std::cos(w);

  float h = random() * std::exp(-2 * (d / galaxy.size())) / 5 * galaxy.size();
  if (random() < 0.5) {
    h = -h;
  }
  const float v = (float) std::sqrt(galaxy.mass() * 0.001f / std::sqrt(d * d + h * h));
  const float vsinw = v * std::sin(w);
  const float vcosw = v * std::cos(w);

  size = std::floor(random() * 7);

  pos[0] = galaxy.mat[0][0] * dcosw + galaxy.mat[1][0] * dsinw + galaxy.mat[2][0] * h + galaxy.pos[0];
  pos[1] = galaxy.mat[0][1] * dcosw + galaxy.mat[1][1] * dsinw + galaxy.mat[2][1] * h + galaxy.pos[1];
  pos[2] = galaxy.mat[0][2] * dcosw + galaxy.mat[1][2] * dsinw + galaxy.mat[2][2] * h + galaxy.pos[2];

  vel[0] =  -galaxy.mat[0][0] * vsinw + galaxy.mat[1][0] * vcosw + galaxy.vel[0];
  vel[1] =  -galaxy.mat[0][1] * vsinw + galaxy.mat[1][1] * vcosw + galaxy.vel[1];
  vel[2] =  -galaxy.mat[0][2] * vsinw + galaxy.mat[1][2] * vcosw + galaxy.vel[2];
}


Galaxy::Galaxy(Universe const &universe, int num_stars) {
  const float Z_OFFSET = 1.5f;
  m_mass = (float) num_stars;
  m_size = RANGE_SIZE * random() + MIN_SIZE;

  vel[0] = random() * 2 - 1;
  vel[1] = random() * 2 - 1;
  vel[2] = random() * 2 - 1;

  float hitIterations = (float) universe.hitIterations;
  pos[0] = (float) ( -vel[0] * universe.deltat * hitIterations + random() - 0.5);
  pos[1] = (float) ( -vel[1] * universe.deltat * hitIterations + random() - 0.5);
  pos[2] = (float) ((-vel[2] * universe.deltat * hitIterations + random() - 0.5) + Z_OFFSET);

  const float w1 = (float) (2.0 * M_PI * (double) random());
  const float w2 = (float) (2.0 * M_PI * (double) random());
  const float sinw1 = std::sin(w1);
  const float sinw2 = std::sin(w2);
  const float cosw1 = std::cos(w1);
  const float cosw2 = std::cos(w2);

  mat[0][0] = cosw2;
  mat[0][1] = -sinw1 * sinw2;
  mat[0][2] = cosw1 * sinw2;
  mat[1][0] = 0;
  mat[1][1] = cosw1;
  mat[1][2] = sinw1;
  mat[2][0] = -sinw2;
  mat[2][1] = -sinw1 * cosw2;
  mat[2][2] = cosw1 * cosw2;

  for (int i = 0; i < num_stars; i++) {
    stars.push_back(Star(*this));
  }
}


namespace {

int s_step = 0;
Universe universe(GALAXY_COUNT);

void updateUniverse() {
  float delta  = 0.000005f;
  float deltat = universe.deltat;

  // Update each galaxy
  for (int i = 0; i < (int) universe.galaxies.size(); ++i) {
    auto &gt = universe.galaxies[i];

    // Update each star in the galaxy
    for (auto &star: gt.stars) {
      float v0 = star.vel[0];
      float v1 = star.vel[1];
      float v2 = star.vel[2];

      for (int k = 0; k < (int) universe.galaxies.size(); ++k) {
        const auto &gtk = universe.galaxies[k];
        const float d0 = gtk.pos[0] - star.pos[0];
        const float d1 = gtk.pos[1] - star.pos[1];
        const float d2 = gtk.pos[2] - star.pos[2];

        float d = d0 * d0 + d1 * d1 + d2 * d2;
        d = gt.mass() / (d * std::sqrt(d)) * delta;

        v0 += d0 * d;
        v1 += d1 * d;
        v2 += d2 * d;
      }

      star.vel[0] = v0;
      star.vel[1] = v1;
      star.vel[2] = v2;

      star.pos[0] += v0 * deltat;
      star.pos[1] += v1 * deltat;
      star.pos[2] += v2 * deltat;
    }

    // Affect the rest of the galaxies
    for (int k = i + 1; k < (int) universe.galaxies.size(); ++k) {
      auto &gtk = universe.galaxies[k];
      float d0 = gtk.pos[0] - gt.pos[0];
      float d1 = gtk.pos[1] - gt.pos[1];
      float d2 = gtk.pos[2] - gt.pos[2];

      float d = d0 * d0 + d1 * d1 + d2 * d2;
      d = gt.mass() * gt.mass() / (d * std::sqrt(d)) * delta;

      d0 *= d;
      d1 *= d;
      d2 *= d;
      gt.vel[0] += d0 / gt.mass();
      gt.vel[1] += d1 / gt.mass();
      gt.vel[2] += d2 / gt.mass();
      gtk.vel[0] -= d0 / gtk.mass();
      gtk.vel[1] -= d1 / gtk.mass();
      gtk.vel[2] -= d2 / gtk.mass();
    }

    gt.pos[0] += gt.vel[0] * deltat;
    gt.pos[1] += gt.vel[1] * deltat;
    gt.pos[2] += gt.vel[2] * deltat;
  }
}

}  // anon namespace


void determine_factor(Image const &img) {
  bool did_first = false;
  Vector3 vec_min;
  Vector3 vec_max;

  for (int i = 0; i < (int) universe.galaxies.size(); ++i) {
    auto &gt = universe.galaxies[i];

    for (auto &star: gt.stars) {
      Vector3 cur(star.pos[0], star.pos[1], star.pos[2]);
      if (did_first) {
        vec_min.set_min(cur);
        vec_max.set_max(cur);
      } else {
        vec_min = cur;
        vec_max = cur;
        did_first = true;
      }
    }
  }

  assert(vec_min.x() < 0);
  assert(vec_min.y() < 0);
  assert(vec_max.x() > 0);
  assert(vec_max.y() > 0);

  double max_x = std::abs(vec_min.x());
  if (max_x < vec_max.x()) max_x = vec_max.x();

  double max_y = vec_max.y();
  if (max_y < vec_max.y()) max_y = vec_max.y();

  Log::warn << "min/max:\n"
            << "  vec_min: " << vec_min.dump() << "\n"
            << "  vec_max: " << vec_max.dump();

  Log::warn << "img w/h: " << img.width() << ", " << img.height() << "\n"
            << "Max x/y: " << max_x       << ", " << max_y;
}


void draw(Image &img) {
  img.set_conversion_factor(1000.0);
  float Factor = 300;

  // Draw each galaxy
  for (int i = 0; i < (int) universe.galaxies.size(); ++i) {
    auto &gt = universe.galaxies[i];
    img.set_color(colors[i]);

    // Draw each star in the galaxy
    for (auto &star: gt.stars) {
      if (star.pos[2] <= 0.0) continue;  // Checked: negative z occurs often

      float x = std::floor((Factor * star.pos[0] / star.pos[2]));
      float y = std::floor((Factor * star.pos[1] / star.pos[2]));

      // TODO draw rect
      float z = std::min(std::floor(2 / (star.pos[2] + star.size)) + 1, 10.0f);
      img.plot_rect(x, y, (int) z, (int) z);
      //img.plot(x, y);
    }
  }
}


void step() {
  updateUniverse();

  s_step += 1;
/*
  if (s_step > Universe::MAX_LIFETIME()) {
    s_step = 0;
    universe.reset(GALAXY_COUNT);
  }
*/
}

}  // namespace galaxies
