#include "camera.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <random>
#include <chrono>
#include "pathtracer/sampler.h"
#include "bsdf.h"
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

void Camera::init_lens(){
  this->elts.clear();
  double radius[11] = {29.475,84.83,19.275,40.77,12.75,0,-14.495,40.77,-20.385,437.065,-39.73};
  double axpos[11] = {3.76,0.12,4.025,3.275,5.705,4.5,1.18,6.065,0.19,3.22,0};
  double N[11] = {1.67,1,1.67,1.699,1,0,1.603,1.658,1,1.717,1};
  double aperture[11] = {25.2,25.2,23,23,18,17.1,17,20,20,20,20};
  for(int i = 0; i < 11 ; i++){
    LensElement e;
    e.radius = radius[i];
    e.center = axpos[i];
    e.ior = N[i];
    e.aperture = aperture[i];
    elts.push_back(e);
  }

}

Vector3D Camera::back_lens_sample() const {

  // Part 1 Task 2: Implement this. Should return a point randomly sampled
  // on the back element of the lens (the element closest to the sensor)
  double aperture = elts.front().aperture;
  double radius = elts.front().radius;
  double center = elts.front().center;
  

  double theta = 2.0*M_PI*random_uniform();
  double r = aperture*.5*sqrt(random_uniform());
  double x = r*cos(theta);
  double y = r*sin(theta);
  double better_z = center - (radius > 0 ? 1 : -1) * sqrt(radius * radius - aperture * aperture * 0.25);
  return Vector3D(x,y,better_z);
}

bool Sphere_intersect(double radius,const Vector3D &center,const Ray &r,double &t1,double &t2) {
  Vector3D o_to_center = r.o - center;

  double a = dot(r.d, r.d);
  double b = 2 * dot(o_to_center, r.d);
  double c = dot(o_to_center, o_to_center) - radius * radius;

  double delta = b * b - 4 * a * c;
  if (delta < 0) return false;

  t1 = (-b - sqrt(delta)) / (2 * a);
  t2 = (-b + sqrt(delta)) / (2 * a);
  
  return true;
}

bool LensElement::pass_through(Ray &r, double &prev_ior) const {
  if (radius == 0) {
    double t = (center - r.o.z) / r.d.z;
    if (t < 0) return false;
    Vector3D p_intersect = r.at_time(t);
    // double actual_aperture = aperture_override != 0 ? aperture_override : aperture;
    double actual_aperture = aperture;

    if (4 * (p_intersect.x * p_intersect.x + p_intersect.y * p_intersect.y) > actual_aperture * actual_aperture) {
      return false;
    }
    return true;
  }

  // intersect with the surface (sphere) defined by (center, radius)
  double t1, t2;
  double t;
  Vector3D p_center(0, 0, center);

  if (!Sphere_intersect(radius, p_center, r, t1, t2)) {
    return false;
  }

  if (r.d.z * radius < 0) {
    // check t2 (the further-away one first)
    if (t2 >= 0) {
      t = t2;
    } else if (t1 >= 0) {
      t = t1;
    } else return false;
  } else {
    if (t1 >= 0) {
      t = t1;
    } else if (t2 >= 0) {
      t = t2;
    } else return false;
  }

  Vector3D p_intersect = r.at_time(t);

  // distance to the z-axis is greater than (aperture / 2)
  if (4 * (p_intersect.x * p_intersect.x + p_intersect.y * p_intersect.y) > aperture * aperture) {
    return false;
  }

  // refract using the Snell's law
  Vector3D normal = p_intersect - p_center;
  normal.normalize();

  // regularize normal to point in the opposite direction of the incoming ray
  if (dot(normal, r.d) > 0) {
    normal = -normal;
  }
  // assuming r.d is normalized
  double cos_theta_i = fabs(dot(r.d, normal));

  // double dis = 1;
  
  double A1, A2, B1, B2, C1, C2;
  double lambda2 = r.waveLength * r.waveLength * pow(0.1, 6);

  // SCHOTT_SF
  A1 = 1.55912923;
  A2 = 0.284246288;
  B1 = 0.968842926;
  B2 = 0.0121481001;
  C1 = 0.0534549042;
  C2 = 112.174809;

  // 1.7174

  // Dispersion function
  double real_ior = (A1 * lambda2) / (lambda2 - A2) + (B1 * lambda2) / (lambda2 - B2) + (C1 * lambda2) / (lambda2 - C2);
  real_ior = sqrt(real_ior + 1);
  
  double k = prev_ior / real_ior;
  double sin_theta_o_2 = k * k * (1 - cos_theta_i * cos_theta_i);

  // total internal reflection
  if (sin_theta_o_2 > 1) {
    return false;
  }

  Vector3D old_direction = r.d;
  r.d = k * r.d + (k * cos_theta_i - sqrt(1 - sin_theta_o_2)) * normal;
  r.d.normalize();
  
  r.o = p_intersect;

  prev_ior = real_ior;

  return true;

}

bool Camera::trace(Ray &r) const{

  double current_ior = 1; // air
  r.d.normalize();

  for (int i = 0; i < elts.size(); i++) {
    if (!elts[i].pass_through(r, current_ior)) return false;
    current_ior = elts[i].ior;
  }
  return true;
}


bool Camera::generate_ray_real_len(Ray &sampled_ray,double x, double y,int color) const {


    // Vector3D sample = lens.back_lens_sample();
    
  LensElement closest = elts[0];

  UniformGridSampler2D sampler;
  Vector2D sample = sampler.get_sample();
  double r = sample.x;
  double theta = sample.y * 2.0 * PI;
  double sensor_depth = 46.2;
  Vector3D random_position = Vector3D(r * closest.radius * cos(theta),r * closest.radius * sin(theta),closest.center - closest.radius);

  double h_fov_radian = hFov * PI / 180;
  double v_fov_radian = vFov * PI / 180;
  double w_half = tan(h_fov_radian / 2) * sensor_depth;
  double h_half = tan(v_fov_radian / 2) * sensor_depth;
  Vector3D sensor_position(w_half - x * w_half * 2,h_half - y * h_half * 2,sensor_depth);

  Vector3D direction = random_position - sensor_position;
  direction.normalize();

  sampled_ray.o = sensor_position;
  sampled_ray.d = direction;

  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::default_random_engine gen(seed);
  std::normal_distribution<double> R(600,25), G(550,25), B(450,15);
  switch (color){
    case 0:
      sampled_ray.waveLength = R(gen);
      break;
    case 1:
      sampled_ray.waveLength = G(gen);
      break;
    case 2:
      sampled_ray.waveLength = B(gen);
      break;
  }
  sampled_ray.color = color;
  // (cos theta)^4 for an unbiased estimate of the radiance
  // coeff = direction.z;
  // coeff *= coeff;
  // trace the sensor ray through the compound lens
  // cout <<"before" << x << " " << y << "" << sampled_ray.d << endl;
  if (trace(sampled_ray)) {
    return false;
  }
  Vector3D sampled_dir = c2w * sampled_ray.d;
  sampled_dir.normalize(); 
  sampled_ray.o = pos + c2w * sampled_ray.o;
  sampled_ray.d = sampled_dir;
  sampled_ray.min_t = nClip;
  sampled_ray.max_t = fClip;
  // cout <<"after:"<< x << " " << y << "" << sampled_ray.d << endl;

  return true;
    
 
}



} // namespace CGL
