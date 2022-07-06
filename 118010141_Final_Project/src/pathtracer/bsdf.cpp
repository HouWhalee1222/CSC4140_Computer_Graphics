#include "bsdf.h"

#include "application/visual_debugger.h"

#include <algorithm>
#include <iostream>
#include <utility>


using std::max;
using std::min;
using std::swap;

namespace CGL {

/**
 * This function creates a object space (basis vectors) from the normal vector
 */
void make_coord_space(Matrix3x3 &o2w, const Vector3D n) {

  Vector3D z = Vector3D(n.x, n.y, n.z);
  Vector3D h = z;
  if (fabs(h.x) <= fabs(h.y) && fabs(h.x) <= fabs(h.z))
    h.x = 1.0;
  else if (fabs(h.y) <= fabs(h.x) && fabs(h.y) <= fabs(h.z))
    h.y = 1.0;
  else
    h.z = 1.0;

  z.normalize();
  Vector3D y = cross(h, z);
  y.normalize();
  Vector3D x = cross(z, y);
  x.normalize();

  o2w[0] = x;
  o2w[1] = y;
  o2w[2] = z;
}

double BSDF::wavelength_dependent_BSDF(double wavelength, Vector3D reflectance) {
  double red_cof = 1.0 / (wavelength - 600);
  double green_cof = 1.0 / (wavelength - 550);
  double blue_cof = 1.0 / (wavelength - 450);
  double N = red_cof + green_cof + blue_cof;
  double albedo = (red_cof * reflectance[0] + green_cof * reflectance[1] + blue_cof * reflectance[2]) / N;
  return albedo;
}

/**
 * Evaluate diffuse lambertian BSDF.
 * Given incident light direction wi and outgoing light direction wo. Note
 * that both wi and wo are defined in the local coordinate system at the
 * point of intersection.
 * \param wo outgoing light direction in local space of point of intersection
 * \param wi incident light direction in local space of point of intersection
 * \return reflectance in the given incident/outgoing directions
 */
double DiffuseBSDF::f(const Vector3D wo, const Vector3D wi,const double waveLength, const int color) {
  // TODO (Part 3.1):
  // This function takes in both wo and wi and returns the evaluation of
  // the BSDF for those two directions.
  double albedo = wavelength_dependent_BSDF(waveLength, reflectance);
  return albedo / PI;

}

/**
 * Evalutate diffuse lambertian BSDF.
 */
double DiffuseBSDF::sample_f(const Vector3D wo, Vector3D *wi, double *pdf,const double waveLength, const int color) {
  // TODO (Part 3.1):
  // This function takes in only wo and provides pointers for wi and pdf,
  // which should be assigned by this function.
  // After sampling a value for wi, it returns the evaluation of the BSDF
  // at (wo, *wi).
  // You can use the `f` function. The reference solution only takes two lines.
  *wi = sampler.get_sample(pdf);

  return this->f(wo,*wi,waveLength,color);

}

void DiffuseBSDF::render_debugger_node()
{
  if (ImGui::TreeNode(this, "Diffuse BSDF"))
  {
    DragDouble3("Reflectance", &reflectance[0], 0.005);
    ImGui::TreePop();
  }
}

/**
 * Evalutate Emission BSDF (Light Source)
 */
double EmissionBSDF::f(const Vector3D wo, const Vector3D wi,const double waveLength, const int color) {
  return 0;
}

/**
 * Evalutate Emission BSDF (Light Source)
 */
double EmissionBSDF::sample_f(const Vector3D wo, Vector3D *wi, double *pdf,const double waveLength, const int color) {
  *pdf = 1.0 / PI;
  *wi = sampler.get_sample(pdf);
  return 0;
}

void EmissionBSDF::render_debugger_node()
{
  if (ImGui::TreeNode(this, "Emission BSDF"))
  {
    DragDouble3("Radiance", &radiance[0], 0.005);
    ImGui::TreePop();
  }
}

} // namespace CGL
