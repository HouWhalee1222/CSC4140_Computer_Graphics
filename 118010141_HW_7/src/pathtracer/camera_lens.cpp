#include "camera.h"

#include <iostream>
#include <sstream>
#include <fstream>

#include "CGL/misc.h"
#include "CGL/vector2D.h"
#include "CGL/vector3D.h"

using std::cout;
using std::endl;
using std::max;
using std::min;
using std::ifstream;
using std::ofstream;

namespace CGL {

using Collada::CameraInfo;

Ray Camera::generate_ray_for_thin_lens(double x, double y, double rndR, double rndTheta) const {

  // TODO Assignment 7: Part 4
  // compute position and direction of ray from the input sensor sample coordinate.
  // Note: use rndR and rndTheta to uniformly sample a unit disk.
  double half_width = tan(hFov * M_PI / 180 / 2);
  double half_height = tan(vFov * M_PI / 180 / 2);

  double x_coor = -half_width + x * 2 * half_width;
  double y_coor = -half_height + y * 2 * half_height;

  Vector3D direction(x_coor, y_coor, -1);
  direction = direction.unit();

  double t = -focalDistance / direction.z;
  Vector3D pFocus = t * direction;

  double pLensx = lensRadius * sqrt(rndR) * cos(rndTheta);
  double pLensy = lensRadius * sqrt(rndR) * sin(rndTheta);
  Vector3D pLens(pLensx, pLensy, 0);

  Vector3D r_ori = c2w * pLens + pos;
  Vector3D r_dir = c2w * (pFocus - pLens).unit();
  Ray ray(r_ori, r_dir);
  ray.min_t = nClip; ray.max_t = fClip;
  
  return ray;
}


} // namespace CGL
