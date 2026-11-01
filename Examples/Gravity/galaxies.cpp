/**
 * galaxies.js
 *
 * Adapted from galaxy.c, the Galaxy Collision Screen Saver, originally written by Uli Siegmund
 * <uli@wombat.okapi.sub.org> on Amiga for EGS in Cluster.
 *
 * Port from Cluster/EGS to C/Intuition by Harald Backert.
 *
 * Port to X11 & incorporation into xlockmore by Hubert Feyrer <hubert.feyrer@rz.uni-regensburg.de>.
 *
 * Permission to use, copy, modify, and distribute this software and its documentation for any
 * purpose and without fee is hereby granted, provided that the above copyright notice appear in
 * all copies and that both that copyright notice and this permission notice appear in supporting
 * documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author shall have no liability
 * with respect to the infringement of copyrights, trade secrets or any patents by this file or any
 * part thereof.  In no event will the author be liable for any lost revenue or profits or other
 * special, indirect and consequential damages.
 *
 * Revision History:
 *
 * 2016-08-14: Ported to ES2015 JavaScript by Ray Toal <rtoal@lmu.edu>
 * 2011-03-12: Ported to JavaScript by Ray Toal <rtoal@lmu.edu>
 * 2001-11-08: Converted to Java applet by Chris Stevenson <chris.stevenson@adelaide.on.net>
 *             http://www.users.on.net/zhcchz/java/galaxy/UniverseApplet.html with z axis
 *             information for star size, perspective view by Richard Loftin <rich@sevenravens.com>
 * 2000-11-14: Port to Win32 by Richard Loftin <rich@sevenravens.com>, now uses z axis information
 *             for star size, perspective view.
 * 1997-05-10: Compatible with xscreensaver
 * 1997-04-18: Memory leak fixed by Tom Schmidt <tschmidt@micron.com>
 * 1997-04-07: Modified by Dave Mitchell <davem@magnet.com>: random star sizes, colors change
 *             depending on velocity
 * 1994-10-23: Modified by David Bagley <bagleyd@tux.org>
 * 1994-10-10: Add colors by Hubert Feyer
 * 1994-09-30: Initial port by Hubert Feyer
 * 1994-03-09: VMS can generate a random number 0.0 which results in a division by zero,
 *             corrected by Jouk Jansen <joukj@hrem.stm.tudelft.nl>
 *
 * =================================================================================================
 * 
 * Source: https://github.com/rtoal/galaxies/blob/master/galaxies.js
 */
#include "galaxies.h"
#include <math.h>        // M_PI
#include <cmath>
#include <cstdlib>       // rand();
#include <vector>
#include <cassert>


namespace galaxies {
namespace {

float random() {
  return (float) rand() / (float) RAND_MAX;
}

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

  Galaxy(Universe const &universe);

  float mass() const { return m_mass;  }
  float size() const { return m_size; }

private:
  float m_mass;
  float m_size;
  static int   MAX_STARS();
  static float RANGE_SIZE();
  static float MIN_SIZE();
};

int Galaxy::MAX_STARS()    { return 700;  }
float Galaxy::RANGE_SIZE() { return 0.1f; }
float Galaxy::MIN_SIZE()   { return 0.1f; }


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


Galaxy::Galaxy(Universe const &universe) {
  const float Z_OFFSET = 1.5f;
  m_mass = (float) MAX_STARS();
  m_size = RANGE_SIZE() * random() + MIN_SIZE();

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

  for (int i = 0; i < MAX_STARS(); i++) {
    stars.push_back(Star(*this));
  }
}


namespace {

// Global state, changed during simulation
int galaxyCount = 3;
int s_step = 0;
Universe universe(galaxyCount);

/*
// UI
const canvas = document.getElementById("galaxies");
const canvasWidth = canvas.width;
const canvasHeight = canvas.height;
const halfCanvasWidth = canvasWidth / 2;
const halfCanvasHeight = canvasHeight / 2;

const unsigned colors[12] = {
  0xffffcc, 0xccffff, 0xffccff, 0xccffcc, 0xccccff, 0xffcccc,
  0x80ffbf, 0xbf80ff, 0xffbf80, 0xbfff80, 0x80bfff, 0xff80bf
};

const ctx = canvas.getContext("2d");
*/
}  // anon namespace


void updateUniverse() {
  float delta = 0.000005f;
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


void draw(Image &img) {
  float halfCanvasWidth  = (float) (img.width()  / 2);
  float halfCanvasHeight = (float) (img.height() / 2);

  // Draw each galaxy
  for (int i = 0; i < (int) universe.galaxies.size(); ++i) {
    auto &gt = universe.galaxies[i];
    // TODO ctx.fillStyle = colors[i];

    // Draw each star in the galaxy
    for (auto &star: gt.stars) {
      if (star.pos[2] > 0.0) {
        float x = std::floor((150 * star.pos[0] / star.pos[2]) + halfCanvasWidth);
        float y = std::floor((150 * star.pos[1] / star.pos[2]) + halfCanvasHeight);

        // TODO draw rect
        //float z = std::min(std::floor(2 / (star.pos[2] + star.size)) + 1, 10.0f);
        //ctx.fillRect(x, y, z, z);

        img.plot(x,y);
      }
    }
  }
}


void step() {
  updateUniverse();

  s_step += 1;
  if (s_step > Universe::MAX_LIFETIME()) {
    s_step = 0;
    universe.reset(galaxyCount);
  }
}

}  // namespace galaxies
