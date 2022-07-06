#include "pathtracer.h"

#include "scene/light.h"
#include "scene/sphere.h"
#include "scene/triangle.h"

using namespace CGL::SceneObjects;

namespace CGL {

PathTracer::PathTracer() {
  gridSampler = new UniformGridSampler2D();
  hemisphereSampler = new UniformHemisphereSampler3D();

  tm_gamma = 2.2f;
  tm_level = 1.0f;
  tm_key = 0.18;
  tm_wht = 5.0f;
}

PathTracer::~PathTracer() {
  delete gridSampler;
  delete hemisphereSampler;
}

void PathTracer::set_frame_size(size_t width, size_t height) {
  sampleBuffer.resize(width, height);
  sampleCountBuffer.resize(width * height);
}

void PathTracer::clear() {
  bvh = NULL;
  scene = NULL;
  camera = NULL;
  sampleBuffer.clear();
  sampleCountBuffer.clear();
  sampleBuffer.resize(0, 0);
  sampleCountBuffer.resize(0, 0);
}

void PathTracer::write_to_framebuffer(ImageBuffer &framebuffer, size_t x0,
                                      size_t y0, size_t x1, size_t y1) {
  sampleBuffer.toColor(framebuffer, x0, y0, x1, y1);
}

//to be converted to wavelength dependent
double PathTracer::estimate_direct_lighting_hemisphere(const Ray &r,
                                                const Intersection &isect) {
  // Estimate the lighting from this intersection coming directly from a light.
  // For this function, sample uniformly in a hemisphere.

  // Note: When comparing Cornel Box (CBxxx.dae) results to importance sampling, you may find the "glow" around the light source is gone.
  // This is totally fine: the area lights in importance sampling has directionality, however in hemisphere sampling we don't model this behaviour.

  // make a coordinate system for a hit point
  // with N aligned with the Z direction.
  Matrix3x3 o2w;
  make_coord_space(o2w, isect.n);
  Matrix3x3 w2o = o2w.T();

  // w_out points towards the source of the ray (e.g.,
  // toward the camera if this is a primary ray)
  const Vector3D hit_p = r.o + r.d * isect.t;
  const Vector3D w_out = w2o * (-r.d);

  // This is the same number of total samples as
  // estimate_direct_lighting_importance (outside of delta lights). We keep the
  // same number of samples for clarity of comparison.
  int num_samples = scene->lights.size() * ns_area_light;
  double L_out = 0;

  for (int i = 0; i < num_samples; i++) {
    Vector3D w_in = hemisphereSampler->get_sample();
    Ray r_in(hit_p, o2w * w_in, 1);  // Transform from world space to object to world (ray intersection)
    r_in.min_t = EPS_F;
    Intersection isect_sample;  // intersect with light source
    if (bvh->intersect(r_in, &isect_sample)) {
      double f_win_wout = isect.bsdf->f(w_out, w_in, r.waveLength, r.color);  // intersect with camera!!!!!!!
      Vector3D L_i = isect_sample.bsdf->get_emission();
      double cos_theta = dot(Vector3D(0.0, 0.0, 1.0), w_in);
      L_out += f_win_wout * L_i[r.color] * cos_theta / (2.0 / M_PI);
    }
  }
  L_out /= double(num_samples);

  return L_out;
}

double PathTracer::estimate_direct_lighting_importance(const Ray &r,
                                                const Intersection &isect) {
  // Estimate the lighting from this intersection coming directly from a light.
  // To implement importance sampling, sample only from lights, not uniformly in
  // a hemisphere.

  // make a coordinate system for a hit point
  // with N aligned with the Z direction.
  Matrix3x3 o2w;
  make_coord_space(o2w, isect.n);
  Matrix3x3 w2o = o2w.T();

  // w_out points towards the source of the ray (e.g.,
  // toward the camera if this is a primary ray)
  const Vector3D hit_p = r.o + r.d * isect.t;
  const Vector3D w_out = w2o * (-r.d);
  double L_out = 0;

  int num_lights = scene->lights.size();

  // point source: only one ray; light source: can sample different ray to hit_p
  for (auto light : scene->lights) {
    double L_sample = 0.0;
    int num_samples = ns_area_light;
    if (light->is_delta_light()) {
      num_samples = 1;
    }
    for (int i = 0; i < num_samples; i++) {
      Vector3D w_in;
      double disToLight, pdf;
      double L_i = light->sample_L(hit_p, &w_in, &disToLight, &pdf, r.color, r.waveLength);
      
      Ray r_in(hit_p, w_in, 1);
      r_in.min_t = EPS_F;
      r_in.max_t = disToLight - EPS_F;
      r_in.waveLength = r.waveLength;
      r_in.color = r.color;
      Intersection isect_sample;
      Vector3D w_in_obj = w2o * w_in;

      if (w_in_obj[2] >= 0 && !bvh->intersect(r_in, &isect_sample)) {
        double f_win_wout = isect.bsdf->f(w_out, w_in_obj, r.waveLength, r.color);
        double cos_theta = dot(Vector3D(0.0, 0.0, 1.0), w_in_obj);
        L_sample += f_win_wout * L_i * cos_theta / pdf;
      }
    }
    L_sample /= double(num_samples);
    L_out += L_sample;  // Not divided by light, sum of light
  }
  return L_out;

}

double PathTracer::zero_bounce_radiance(const Ray &r,
                                          const Intersection &isect) {

  return isect.bsdf->get_emission()[r.color];

}

double PathTracer::one_bounce_radiance(const Ray &r,
                                         const Intersection &isect) {
  // TODO: Part 3, Task 3
  // Returns either the direct illumination by hemisphere or importance sampling
  // depending on `direct_hemisphere_sample`

  // if(direct_hemisphere_sample) return estimate_direct_lighting_hemisphere(r,isect);
  // else return estimate_direct_lighting_importance(r,isect);
  if (direct_hemisphere_sample) {
    return estimate_direct_lighting_hemisphere(r, isect);
  } else {
    return estimate_direct_lighting_importance(r, isect);
  }
}

double PathTracer::at_least_one_bounce_radiance(const Ray &r,
                                                  const Intersection &isect) {

  Matrix3x3 o2w;
  make_coord_space(o2w, isect.n);
  Matrix3x3 w2o = o2w.T();

  Vector3D hit_p = r.o + r.d * isect.t;
  Vector3D w_out = w2o * (-r.d);

  double L_out = 0;

    if (!isect.bsdf->is_delta()) {
        L_out += one_bounce_radiance(r, isect);
    }
    
    if (r.depth < max_ray_depth && coin_flip(0.7)) {
        Vector3D w_in;
        double pdf;
        double f = isect.bsdf->sample_f(w_out, &w_in, &pdf,r.waveLength,r.color);    
        Ray sample_ray = Ray(hit_p, (o2w * w_in).unit(), INF_D, r.depth + 1);
        sample_ray.min_t = EPS_D;
        sample_ray.waveLength = r.waveLength;
        sample_ray.color = r.color;
        Intersection sample_i;
        if (bvh->intersect(sample_ray, &sample_i)) {
            double l = at_least_one_bounce_radiance(sample_ray, sample_i);
            if (isect.bsdf->is_delta()) {
                l += zero_bounce_radiance(sample_ray, sample_i);
            }
            double f_l = f * l;
            L_out += f_l * abs_cos_theta(w_in) / pdf / 0.7;
        }
    }
  return L_out;

}

double PathTracer::est_radiance_global_illumination(const Ray &r) {
    Intersection isect;
    double L_out = 0.0;

    // You will extend this in assignment 3-2.
    // If no intersection occurs, we simply return black.
    // This changes if you implement hemispherical lighting for extra credit.

    // The following line of code returns a debug color depending
    // on whether ray intersection with triangles or spheres has
    // been implemented.
    //
    // REMOVE THIS LINE when you are ready to begin Part 3.

    if (!bvh->intersect(r, &isect))
      return envLight ? envLight->sample_dir(r)[r.color] : L_out;

    // normal_shading(isect.n)
    // normal -> zero_bounce

    // TODO (Part 3): Return the direct illumination.

    // TODO (Part 4): Accumulate the "direct" and "indirect"
    // parts of global illumination into L_out rather than just direct
    L_out = (isect.t == INF_D) ? debug_shading(r.d)[r.color] : (zero_bounce_radiance(r, isect) + at_least_one_bounce_radiance(r, isect));
    // L_out = (isect.t == INF_D) ? debug_shading(r.d) : normal_shading(isect.n);


    return L_out;
}

void PathTracer::raytrace_pixel(size_t x, size_t y) {
  int num_samples = ns_aa;          // total samples to evaluate
  Vector2D origin = Vector2D(x, y); // bottom left corner of the pixel
  Vector3D radiance_sum(0., 0., 0.);
  Vector3D radiance(0., 0., 0.);
  double s1 = 0, s2 = 0;
  for(int i = 0; i < num_samples; i++){
    for(int color = 0; color < 3; color++){
      Vector2D sample = origin + gridSampler->get_sample();
      Ray r = camera->generate_ray(sample[0] / sampleBuffer.w, sample[1] / sampleBuffer.h, color);
      Vector3D temp;
      double cos_term;
      r.depth = 0;
      r.color = color;
      radiance[color] = PathTracer::est_radiance_global_illumination(r);
      radiance_sum[color] += radiance[color];
    }
    // double illuminance = radiance.illum();
    // s1 += illuminance;
    // s2 += illuminance * illuminance;

    // if (i % samplesPerBatch == 0) {
    //   double miu = s1 / double(i);
    //   double rou_square = (s2 - s1 * s1 / double(i)) / (double(i) - 1);
    //   double I = 1.96 * sqrt(rou_square) / sqrt(double(i));
    //   if (I <= maxTolerance * miu) {
    //     num_samples = i;
    //     break;
    //   }
    // }

  }
  for (int i = 0; i < 3; i++) {
    radiance[i] = radiance_sum[i] / num_samples;
  }
  int temperature = 10000;
  for (int color = 0; color < 3; color++) {
    double radiance_cof = color_temperature(temperature, color);
    radiance_cof /= 255;
    radiance[color] *= radiance_cof;
  }

  sampleBuffer.update_pixel(radiance, x, y);
  sampleCountBuffer[x + y * sampleBuffer.w] = num_samples;
}

void PathTracer::autofocus(Vector2D loc) {
  Ray r = camera->generate_ray(loc.x / sampleBuffer.w, loc.y / sampleBuffer.h,0); //todo
  Intersection isect;

  bvh->intersect(r, &isect);

  camera->focalDistance = isect.t;
}

// temperature: 1000 - 40000
double PathTracer::color_temperature(int temperature, int color) {
  double radiance;
  temperature /= 100;
  switch (color)
  {
  case 0: // R
    if (temperature <= 66) {
      radiance = 255;
    } else {
      radiance = temperature - 60;
      radiance = 329.698727446 * (pow(radiance, -0.1332047592));
      if (radiance < 0) radiance = 0;
      if (radiance > 255) radiance = 255;
    }
    break;
  
  case 1: // G
    if (temperature <= 66) {
      radiance = temperature;
      radiance = 99.4708025861 * log(radiance) - 161.1195681661;
      if (radiance < 0) radiance = 0;
      if (radiance > 255) radiance = 255;      
    } else {
      radiance = temperature - 60;
      radiance = 288.1221695283 * pow(radiance, -0.0755148492);
      if (radiance < 0) radiance = 0;
      if (radiance > 255) radiance = 255;    
    }
    break;

  case 2: // B
    if (temperature >= 66) {
      radiance = 255;
    } else {
      if (temperature <= 19) {
        radiance = 0;
      } else {
        radiance = temperature - 10;
        radiance = 138.5177312231 * log(radiance) - 305.0447927307;
      if (radiance < 0) radiance = 0;
      if (radiance > 255) radiance = 255;    
      }
    }
  
  default:
    break;
  }

  return radiance;
}


} // namespace CGL
