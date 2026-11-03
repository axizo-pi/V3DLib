/**
 * /file
 *
 * Mass centers of galaxies are used for the gravity calculation,
 * as opposed to full calculation between stars.
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
// Configuration values
//
const int   GALAXY_COUNT = 3;    // Number of galaxies to start up with
const int   MAX_STARS    = 2000; // Original: 700;   // Amount of stars in a galaxy
const float RANGE_SIZE   = 0.1f;
const float MIN_SIZE     = 0.1f;

}  // anon namespace


class Universe;  // Forward declaration
class Galaxy;    // Forward declaration

class Entity {
public:  
  float pos[3];
  float vel[3];

  void set_pos(Vector3 const &rhs) {
    pos[0] = (float) rhs.x();
    pos[1] = (float) rhs.y();
    pos[2] = (float) rhs.z();
  }

  void set_vel(Vector3 const &rhs) {
    vel[0] = (float) rhs.x();
    vel[1] = (float) rhs.y();
    vel[2] = (float) rhs.z();
  }
};

/*
 * A star belongs to a galaxy and has a size, pos, and vel.
 */
class Star : public Entity {
public:
  float size;

  Star(Galaxy const &galaxy);
};


/*
 * A galaxy exists within a universe and has a mass, size, vel, pos, mat, and stars.
 */
class Galaxy : public Entity {
public:
  float mat[3][3];
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
      galaxies.push_back(Galaxy(*this));
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
    Vector3 gt_p(gt.pos);

    // Update each star in the galaxy
    for (auto &star: gt.stars) {
      Vector3 star_v(star.vel);

      //
      // Stars are affected by the galaxies, not individual stars
      //
      for (int k = 0; k < (int) universe.galaxies.size(); ++k) {
        const auto &gtk = universe.galaxies[k];
        
        const Vector3 dv = Vector3(gtk.pos) - Vector3(star.pos);

        // Mass of stars is always 1
        auto dot = dv.dot();
        dot = gt.mass() / (dot * std::sqrt(dot)) * delta;

        star_v += dv * dot;
      }

      Vector3 star_p(star.pos);
      star_p += (star_v * deltat);
      star.set_pos(star_p);
      star.set_vel(star_v);
    }

    //
    // Affect the rest of the galaxies.
    // Calculation is between galaxies only, not stars.
    //
    for (int k = i + 1; k < (int) universe.galaxies.size(); ++k) {
      auto &gtk = universe.galaxies[k];

      Vector3 d_pos = Vector3(gtk.pos) - gt_p;

      auto dot = d_pos.dot();
      dot = gtk.mass() * gt.mass() / (dot * std::sqrt(dot)) * delta;

      d_pos *= dot;

      Vector3 gt_vel(gt.vel);
      gt_vel += d_pos / gt.mass();
      gt.set_vel(gt_vel);

      Vector3 gtk_vel(gtk.vel);
      gtk_vel -= d_pos / gtk.mass();
      gtk.set_vel(gtk_vel);
    }

    Vector3 gt_pos(gt.pos);
    Vector3 gt_vel(gt.vel);
    gt_pos += gt_vel * deltat;
    gt.set_pos(gt_pos);
  }
}

}  // anon namespace


void draw(Image &img) {
  // Can't properly figure out how coordinates are converted
  // Following factors work OK
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
