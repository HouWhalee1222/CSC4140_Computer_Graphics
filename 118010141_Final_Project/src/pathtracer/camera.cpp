#include "camera.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <random>
#include <chrono>

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

/**
 * Sets the field of view to match screen screenW/H.
 * NOTE: data and screenW/H will almost certainly disagree about the aspect
 *       ratio. screenW/H are treated as the source of truth, and the field
 *       of view is expanded along whichever dimension is too narrow.
 * NOTE2: info.hFov and info.vFov are expected to be in DEGREES.
 */
void Camera::configure(const CameraInfo& info, size_t screenW, size_t screenH) {
  this->screenW = screenW;
  this->screenH = screenH;
  nClip = info.nClip;
  fClip = info.fClip;
  hFov = info.hFov;
  vFov = info.vFov;

  double ar1 = tan(radians(hFov) / 2) / tan(radians(vFov) / 2);
  ar = static_cast<double>(screenW) / screenH;
  if (ar1 < ar) {
    // hFov is too small
    hFov = 2 * degrees(atan(tan(radians(vFov) / 2) * ar));
  } else if (ar1 > ar) {
    // vFov is too small
    vFov = 2 * degrees(atan(tan(radians(hFov) / 2) / ar));
  }
  screenDist = ((double) screenH) / (2.0 * tan(radians(vFov) / 2));
  init_lens();
}

/**
 * This function places the camera at the target position and sets the arguments.
 * Phi and theta are in RADIANS.
 */
void Camera::place(const Vector3D targetPos, const double phi,
                   const double theta, const double r, const double minR,
                   const double maxR) {
  double r_ = min(max(r, minR), maxR);
  double phi_ = (sin(phi) == 0) ? (phi + EPS_F) : phi;
  this->targetPos = targetPos;
  this->phi = phi_;
  this->theta = theta;
  this->r = r_;
  this->minR = minR;
  this->maxR = maxR;
  compute_position();
}

/**
 * This function copies the camera placement state.
 */
void Camera::copy_placement(const Camera& other) {
  pos = other.pos;
  targetPos = other.targetPos;
  phi = other.phi;
  theta = other.theta;
  minR = other.minR;
  maxR = other.maxR;
  c2w = other.c2w;
}

/**
 * This sets the screen size & compute the new FOV.
 */
void Camera::set_screen_size(const size_t screenW, const size_t screenH) {
  this->screenW = screenW;
  this->screenH = screenH;
  ar = 1.0 * screenW / screenH;
  hFov = 2 * degrees(atan(((double) screenW) / (2 * screenDist)));
  vFov = 2 * degrees(atan(((double) screenH) / (2 * screenDist)));
}

/**
 * This function translates the camera position
 */
void Camera::move_by(const double dx, const double dy, const double d) {
  const double scaleFactor = d / screenDist;
  const Vector3D displacement =
    c2w[0] * (dx * scaleFactor) + c2w[1] * (dy * scaleFactor);
  pos += displacement;
  targetPos += displacement;
}

/**
 * This function translates the camera position (in forward direction)
 */
void Camera::move_forward(const double dist) {
  double newR = min(max(r - dist, minR), maxR);
  pos = targetPos + ((pos - targetPos) * (newR / r));
  r = newR;
}

/**
 * This function rotates the camera position
 */
void Camera::rotate_by(const double dPhi, const double dTheta) {
  phi = clamp(phi + dPhi, 0.0, (double) PI);
  theta += dTheta;
  compute_position();
}

/**
 * This function computes the camera position, basis vectors, and the view matrix
 */
void Camera::compute_position() {
  double sinPhi = sin(phi);
  if (sinPhi == 0) {
    phi += EPS_F;
    sinPhi = sin(phi);
  }
  const Vector3D dirToCamera(r * sinPhi * sin(theta),
                             r * cos(phi),
                             r * sinPhi * cos(theta));
  pos = targetPos + dirToCamera;
  Vector3D upVec(0, sinPhi > 0 ? 1 : -1, 0);
  Vector3D screenXDir = cross(upVec, dirToCamera);
  screenXDir.normalize();
  Vector3D screenYDir = cross(dirToCamera, screenXDir);
  screenYDir.normalize();

  c2w[0] = screenXDir;
  c2w[1] = screenYDir;
  c2w[2] = dirToCamera.unit();   // camera's view direction is the
                                 // opposite of of dirToCamera, so
                                 // directly using dirToCamera as
                                 // column 2 of the matrix takes [0 0 -1]
                                 // to the world space view direction
}

/**
 * This function stores the camera settings into a file
 */
void Camera::dump_settings(string filename) {
  ofstream file(filename);
  file << hFov << " " << vFov << " " << ar << " " << nClip << " " << fClip << endl;
  for (int i = 0; i < 3; ++i)
    file << pos[i] << " ";
  for (int i = 0; i < 3; ++i)
    file << targetPos[i] << " ";
  file << endl;
  file << phi << " " << theta << " " << r << " " << minR << " " << maxR << endl;
  for (int i = 0; i < 9; ++i)
    file << c2w(i/3, i%3) << " ";
  file << endl;
  file << screenW << " " << screenH << " " << screenDist << endl;
  file << focalDistance << " " << lensRadius << endl;
  cout << "[Camera] Dumped settings to " << filename << endl;
}

/**
 * This function loads the camera settings from a file
 */
void Camera::load_settings(string filename) {
  ifstream file(filename);

  file >> hFov >> vFov >> ar >> nClip >> fClip;
  for (int i = 0; i < 3; ++i)
    file >> pos[i];
  for (int i = 0; i < 3; ++i)
    file >> targetPos[i];
  file >> phi >> theta >> r >> minR >> maxR;
  for (int i = 0; i < 9; ++i)
    file >> c2w(i/3, i%3);
  file >> screenW >> screenH >> screenDist;
  file >> focalDistance >> lensRadius;
  cout << "[Camera] Loaded settings from " << filename << endl;
}

/**
 * This function generates a ray from camera perspective, passing through camera / sensor plane (x,y)
 */
Ray Camera::generate_ray(double x, double y,int color) const {

  // TODO (Part 1.1):
  // compute position of the input sensor sample coordinate on the
  // canonical sensor plane one unit away from the pinhole.
  // Note: hFov and vFov are in degrees.
  //
  double half_width = tan(hFov * M_PI / 180 / 2);
  double half_height = tan(vFov * M_PI / 180 / 2);

  double x_coor = -half_width + x * 2 * half_width;
  double y_coor = -half_height + y * 2 * half_height;

  Vector3D sensor_coor(x_coor, y_coor, -1);
  sensor_coor = sensor_coor.unit();
  Vector3D world_coor = c2w * sensor_coor;

  Ray ray(pos, world_coor.unit());
  ray.min_t = nClip; ray.max_t = fClip;
  ray.color = color;

  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::default_random_engine gen(seed);
  std::normal_distribution<double> R(600.0, 25), G(550.0, 25), B(450.0, 15);
  switch (color){
    case 0:
      ray.waveLength = R(gen);
      break;
    case 1:
      ray.waveLength = G(gen);
      break;
    case 2:
      ray.waveLength = B(gen);
      break;
  }
  return ray;

}

} // namespace CGL
