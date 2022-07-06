#include "bsdf.h"

#include <algorithm>
#include <iostream>
#include <utility>

#include "application/visual_debugger.h"

using std::max;
using std::min;
using std::swap;

namespace CGL {

// Mirror BSDF //

Vector3D MirrorBSDF::f(const Vector3D wo, const Vector3D wi) {
  return Vector3D();
}

Vector3D MirrorBSDF::sample_f(const Vector3D wo, Vector3D* wi, double* pdf) {

  // TODO Assignment 7: Part 1
  // Implement MirrorBSDF
  return Vector3D();
}

void MirrorBSDF::render_debugger_node()
{
  if (ImGui::TreeNode(this, "Mirror BSDF"))
  {
    DragDouble3("Reflectance", &reflectance[0], 0.005);
    ImGui::TreePop();
  }
}

// Microfacet BSDF //

double MicrofacetBSDF::G(const Vector3D wo, const Vector3D wi) {
  return 1.0 / (1.0 + Lambda(wi) + Lambda(wo));
}

double MicrofacetBSDF::D(const Vector3D h) {
  // TODO Assignment 7: Part 2
  // Compute Beckmann normal distribution function (NDF) here.
  // You will need the roughness alpha.
  double theta_h = getTheta(h.unit());
  double cos_theta_h = cos(theta_h);
  double cos_theta_h_square = cos_theta_h * cos_theta_h;
  double tan_theta_h_square = (1 - cos_theta_h_square) / cos_theta_h_square;
  
  double numerator = exp(-tan_theta_h_square / (alpha * alpha));
  double denominator = M_PI * alpha * alpha * cos_theta_h_square * cos_theta_h_square;

  return numerator / denominator;
}

Vector3D MicrofacetBSDF::F(const Vector3D wi) {
  // TODO Assignment 7: Part 2
  // Compute Fresnel term for reflection on dielectric-conductor interface.
  // You will need both eta and etaK, both of which are Vector3D.
  double theta_i = getTheta(wi.unit());
  double cos_theta_i = cos(theta_i);
  Vector3D Rs_num = eta * eta + k * k - 2 * eta * cos_theta_i + cos_theta_i * cos_theta_i;
  Vector3D Rs_de = eta * eta + k * k + 2 * eta * cos_theta_i + cos_theta_i * cos_theta_i;
  Vector3D Rs = Rs_num / Rs_de;

  Vector3D Rp_num = (eta * eta + k * k) * cos_theta_i * cos_theta_i - 2 * eta * cos_theta_i + 1;
  Vector3D Rp_de = (eta * eta + k * k) * cos_theta_i * cos_theta_i + 2 * eta * cos_theta_i + 1;
  Vector3D Rp = Rp_num / Rp_de;

  return (Rs + Rp) / 2;
}

Vector3D MicrofacetBSDF::f(const Vector3D wo, const Vector3D wi) {
  // TODO Assignment 7: Part 2
  // Implement microfacet model here.
  if (wo.z < 0 || wi.z < 0) return Vector3D(0, 0, 0);

  Vector3D h = ((wo + wi) / 2).unit(); // Half vector
  Vector3D numerator = F(wi) * G(wo, wi) * D(h);
  double denominator = 4 * dot(Vector3D(0, 0, 1.0), wo) * dot(Vector3D(0, 0, 1.0), wi);
  return numerator / denominator;
}

Vector3D MicrofacetBSDF::sample_f(const Vector3D wo, Vector3D* wi, double* pdf) {
  // TODO Assignment 7: Part 2
  // *Importance* sample Beckmann normal distribution function (NDF) here.
  // Note: You should fill in the sampled direction *wi and the corresponding *pdf,
  //       and return the sampled BRDF value.

  Vector2D r = sampler.get_sample();
  double r1 = r[0];
  double r2 = r[1];

  double theta_h = atan(sqrt(-alpha * alpha * log(1 - r1)));
  double phi_h = 2 * M_PI * r2;

  Vector3D h(sin(theta_h) * cos(phi_h), sin(theta_h) * sin(phi_h), cos(theta_h));  // multiply!!
  h = h.unit();
  *wi = 2 * dot(wo, h) * h  - wo;
  *wi = wi->unit();

  if (wi->z < 0) {
    *pdf = 0;
    return Vector3D(0, 0, 0);
  }

  double p_theta_h_num = 2 * sin(theta_h) * exp(-tan(theta_h) * tan(theta_h) / alpha / alpha);
  double p_theta_h_de = alpha * alpha * cos(theta_h) * cos(theta_h) * cos(theta_h);
  double p_theta_h = p_theta_h_num / p_theta_h_de;

  double p_phi_h = 1.0 / (2.0 * M_PI);


  double p_wh = p_theta_h * p_phi_h / sin(theta_h);
  double p_wi = p_wh / (4 * dot(*wi, h));

  *pdf = p_wi;
  return MicrofacetBSDF::f(wo, *wi);

  // Default
  // *wi = cosineHemisphereSampler.get_sample(pdf);
  // return MicrofacetBSDF::f(wo, *wi);
}

void MicrofacetBSDF::render_debugger_node()
{
  if (ImGui::TreeNode(this, "Micofacet BSDF"))
  {
    DragDouble3("eta", &eta[0], 0.005);
    DragDouble3("K", &k[0], 0.005);
    DragDouble("alpha", &alpha, 0.005);
    ImGui::TreePop();
  }
}

// Refraction BSDF //

Vector3D RefractionBSDF::f(const Vector3D wo, const Vector3D wi) {
  return Vector3D();
}

Vector3D RefractionBSDF::sample_f(const Vector3D wo, Vector3D* wi, double* pdf) {
  // TODO Assignment 7: Part 1
  // Implement RefractionBSDF
  return Vector3D();
}

void RefractionBSDF::render_debugger_node()
{
  if (ImGui::TreeNode(this, "Refraction BSDF"))
  {
    DragDouble3("Transmittance", &transmittance[0], 0.005);
    DragDouble("ior", &ior, 0.005);
    ImGui::TreePop();
  }
}

// Glass BSDF //

Vector3D GlassBSDF::f(const Vector3D wo, const Vector3D wi) {
  return Vector3D();
}

Vector3D GlassBSDF::sample_f(const Vector3D wo, Vector3D* wi, double* pdf) {

  // TODO Assignment 7: Part 1
  // Compute Fresnel coefficient and either reflect or refract based on it.

  // compute Fresnel coefficient and use it as the probability of reflection
  // - Fundamentals of Computer Graphics page 305
  return Vector3D();
}

void GlassBSDF::render_debugger_node()
{
  if (ImGui::TreeNode(this, "Refraction BSDF"))
  {
    DragDouble3("Reflectance", &reflectance[0], 0.005);
    DragDouble3("Transmittance", &transmittance[0], 0.005);
    DragDouble("ior", &ior, 0.005);
    ImGui::TreePop();
  }
}

void BSDF::reflect(const Vector3D wo, Vector3D* wi) {

  // TODO Assignment 7: Part 1
  // Implement reflection of wo about normal (0,0,1) and store result in wi.


}

bool BSDF::refract(const Vector3D wo, Vector3D* wi, double ior) {

  // TODO Assignment 7: Part 1
  // Use Snell's Law to refract wo surface and store result ray in wi.
  // Return false if refraction does not occur due to total internal reflection
  // and true otherwise. When dot(wo,n) is positive, then wo corresponds to a
  // ray entering the surface through vacuum.

  return true;

}

} // namespace CGL
