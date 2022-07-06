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

// from spectrum to color
double MirrorBSDF::f(const Vector3D wo, const Vector3D wi, const double waveLength, const int color) {
  return Vector3D()[color];
}

double MirrorBSDF::sample_f(const Vector3D wo, Vector3D* wi, double* pdf, const double waveLength, const int color) {

  // TODO Assignment 7: Part 1
  // Implement MirrorBSDF
  reflect(wo, wi, waveLength);
  *pdf = 1;

  double f = wavelength_dependent_BSDF(waveLength, reflectance);
  return f / abs_cos_theta(*wi);
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

double MicrofacetBSDF::f(const Vector3D wo, const Vector3D wi, const double waveLength, const int color) {
  // TODO Assignment 7: Part 2
  // Implement microfacet model here.
  if( wo.z < 0 || wi.z < 0) return Vector3D(0, 0, 0)[color];

  Vector3D h = ((wo + wi) / 2).unit(); // Half vector
  Vector3D numerator = F(wi) * G(wo, wi) * D(h);
  double denominator = 4 * dot(Vector3D(0, 0, 1.0), wo) * dot(Vector3D(0, 0, 1.0), wi);
  Vector3D reflectance_f = numerator / denominator;
  double f = wavelength_dependent_BSDF(waveLength, reflectance_f);
  f = f * waveLength / 300;
  return f;
}

double MicrofacetBSDF::sample_f(const Vector3D wo, Vector3D* wi, double* pdf, const double waveLength, const int color) {
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
    return Vector3D(0, 0, 0)[color];
  }

  double p_theta_h_num = 2 * sin(theta_h) * exp(-tan(theta_h) * tan(theta_h) / alpha / alpha);
  double p_theta_h_de = alpha * alpha * cos(theta_h) * cos(theta_h) * cos(theta_h);
  double p_theta_h = p_theta_h_num / p_theta_h_de;

  double p_phi_h = 1.0 / (2.0 * M_PI);


  double p_wh = p_theta_h * p_phi_h / sin(theta_h);
  double p_wi = p_wh / (4 * dot(*wi, h));

  *pdf = p_wi;
  return MicrofacetBSDF::f(wo, *wi, waveLength, color);

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

double RefractionBSDF::f(const Vector3D wo, const Vector3D wi, const double waveLength, const int color) {
  
  return Vector3D()[color];
}

double RefractionBSDF::sample_f(const Vector3D wo, Vector3D* wi, double* pdf, const double waveLength, const int color) {
  // TODO Assignment 7: Part 1
  // Implement RefractionBSDF
  double A1, A2, B1, B2, C1, C2;
  double lambda2 = waveLength * waveLength * pow(0.1, 6);

  // SCHOTT_SF glass
  A1 = 1.55912923;
  A2 = 0.284246288;
  B1 = 0.968842926;
  B2 = 0.0121481001;
  C1 = 0.0534549042;
  C2 = 112.174809;

  // 1.7174

  // Dispersion function
  double new_ior = (A1 * lambda2) / (lambda2 - A2) + (B1 * lambda2) / (lambda2 - B2) + (C1 * lambda2) / (lambda2 - C2);
  new_ior = sqrt(new_ior + 1);

  if(!BSDF::refract(wo,wi,new_ior,waveLength))
    return Vector3D()[color];
  double eta;
  *pdf = 1;
  wo.z > 0 ? eta = 1 / new_ior : eta = new_ior; 
  double f = wavelength_dependent_BSDF(waveLength, transmittance);
  return f / abs_cos_theta(*wi) / (eta*eta);
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

double GlassBSDF::f(const Vector3D wo, const Vector3D wi, const double waveLength, const int color) {
  return Vector3D()[color];
}

double GlassBSDF::sample_f(const Vector3D wo, Vector3D* wi, double* pdf, const double waveLength, const int color) {
  double A1, A2, B1, B2, C1, C2;
  double lambda2 = waveLength * waveLength * pow(0.1, 6);

  // SCHOTT_SF glass
  A1 = 1.55912923;
  A2 = 0.284246288;
  B1 = 0.968842926;
  B2 = 0.0121481001;
  C1 = 0.0534549042;
  C2 = 112.174809;
  
  // Dispersion function
  double new_ior = (A1 * lambda2) / (lambda2 - A2) + (B1 * lambda2) / (lambda2 - B2) + (C1 * lambda2) / (lambda2 - C2);
  new_ior = sqrt(new_ior + 1);

  double eta;
  if(wo.z > 0){
    eta = 1 / new_ior;
  }
  else{
    eta = new_ior;
  }
  double f = wavelength_dependent_BSDF(waveLength, reflectance);
  double t = wavelength_dependent_BSDF(waveLength, transmittance);

  if(!BSDF::refract(wo,wi,new_ior,waveLength)){
    BSDF::reflect(wo,wi,waveLength);
    *pdf = 1;
    return f / abs_cos_theta(*wi);
  }
  else{
    double r0,r,cosTheta;
    cosTheta = (wo.z > 0) ? wo.z : -wo.z;
    r0 = pow((1-new_ior)/(1+new_ior),2);
    r = r0 + (1 - r0) * pow(1-cosTheta,5);
    if(coin_flip(r)){
      BSDF::reflect(wo,wi,waveLength);
      *pdf = r;
      return r * f / abs_cos_theta(*wi);
    }
    else{
      BSDF::refract(wo,wi,new_ior,waveLength);
      *pdf = 1 - r;
      return (1 - r) * t / abs_cos_theta(*wi) / pow(eta,2);
    }
  }


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

void BSDF::reflect(const Vector3D wo, Vector3D* wi, const double waveLength) {

  // TODO Assignment 7: Part 1
  // Implement reflection of wo about normal (0,0,1) and store result in wi.
  // WaveLength dependent
  wi->x = (-wo.x) * (waveLength * waveLength / 500 / 600);
  wi->y = (-wo.y) * (waveLength * waveLength / 500 / 600);
  wi->z = wo.z;
  return;

}

bool BSDF::refract(const Vector3D wo, Vector3D* wi, double ior, const double waveLength) {

  // TODO Assignment 7: Part 1
  // Use Snell's Law to refract wo surface and store result ray in wi.
  // Return false if refraction does not occur due to total internal reflection
  // and true otherwise. When dot(wo,n) is positive, then wo corresponds to a
  // ray entering the surface through vacuum.

  double A1, A2, B1, B2, C1, C2;
  double lambda2 = waveLength * waveLength * pow(0.1, 6);

  // SCHOTT_SF glass
  A1 = 1.55912923;
  A2 = 0.284246288;
  B1 = 0.968842926;
  B2 = 0.0121481001;
  C1 = 0.0534549042;
  C2 = 112.174809;

  // 1.7174

  // Dispersion function
  double new_ior = (A1 * lambda2) / (lambda2 - A2) + (B1 * lambda2) / (lambda2 - B2) + (C1 * lambda2) / (lambda2 - C2);
  new_ior = sqrt(new_ior + 1);

  double eta,delta;
  wo.z > 0 ? eta = 1 / new_ior : eta = new_ior;
  wi->x = -1 * eta * wo.x;
  wi->y = -1 * eta * wo.y;
  delta = 1 - eta * eta * (1 - wo.z*wo.z);
  if(delta < 0) return false;
  wi->z = sqrt(delta);
  if(wo.z > 0) wi->z = 0 - wi->z;
  return true;

}

} // namespace CGL
