#include "pathtracer.h"

#include "scene/light.h"
#include "scene/sphere.h"
#include "scene/triangle.h"
#include <iostream>

using namespace CGL::SceneObjects;

namespace CGL
{

  PathTracer::PathTracer()
  {
    gridSampler = new UniformGridSampler2D();
    hemisphereSampler = new UniformHemisphereSampler3D();

    tm_gamma = 2.2f;
    tm_level = 1.0f;
    tm_key = 0.18;
    tm_wht = 5.0f;
  }

  PathTracer::~PathTracer()
  {
    delete gridSampler;
    delete hemisphereSampler;
  }

  void PathTracer::set_frame_size(size_t width, size_t height)
  {
    sampleBuffer.resize(width, height);
    sampleCountBuffer.resize(width * height);
  }

  void PathTracer::clear()
  {
    bvh = NULL;
    scene = NULL;
    camera = NULL;
    sampleBuffer.clear();
    sampleCountBuffer.clear();
    sampleBuffer.resize(0, 0);
    sampleCountBuffer.resize(0, 0);
  }

  void PathTracer::write_to_framebuffer(ImageBuffer &framebuffer, size_t x0,
                                        size_t y0, size_t x1, size_t y1)
  {
    sampleBuffer.toColor(framebuffer, x0, y0, x1, y1);
  }

  Vector3D PathTracer::estimate_direct_lighting_hemisphere(const Ray &r,
                                                  const Intersection &isect)
  {
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
    Vector3D L_out(0.0, 0.0, 0.0);
    // std::cout << "here1" << std::endl;

    // TODO (Part 3): Write your sampling loop here
    // TODO BEFORE YOU BEGIN
    // UPDATE `est_radiance_global_illumination` to return direct lighting instead of normal shading

    for (int i = 0; i < num_samples; i++) {
      Vector3D w_in = hemisphereSampler->get_sample();
      Ray r_in(hit_p, o2w * w_in);  // Transform from world space to object to world (ray intersection)
      r_in.min_t = EPS_F;
      Intersection isect_sample;  // intersect with light source
      if (bvh->intersect(r_in, &isect_sample)) {
        Vector3D f_win_wout = isect.bsdf->f(w_out, w_in);  // intersect with camera!!!!!!!
        Vector3D L_i = isect_sample.bsdf->get_emission();
        double cos_theta = dot(Vector3D(0.0, 0.0, 1.0), w_in);
        L_out += f_win_wout * L_i * cos_theta / (1.0 / M_PI);
      }
    }
    L_out /= double(num_samples);

    return L_out;
  }

  Vector3D PathTracer::estimate_direct_lighting_importance(const Ray &r,
                                                           const Intersection &isect)
  {
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
    Vector3D L_out(0.0, 0.0, 0.0);

    int num_lights = scene->lights.size();

    // point source: only one ray; light source: can sample different ray to hit_p
    for (auto light : scene->lights) {
      Vector3D L_sample(0.0, 0.0, 0.0);
      int num_samples = ns_area_light;
      if (light->is_delta_light()) {
        num_samples = 1;
      }
      for (int i = 0; i < num_samples; i++) {
        Vector3D w_in;
        double disToLight, pdf;
        Vector3D L_i = light->sample_L(hit_p, &w_in, &disToLight, &pdf);
        
        Ray r_in(hit_p, w_in);
        r_in.min_t = EPS_F;
        r_in.max_t = disToLight - EPS_F;
        Intersection isect_sample;
        Vector3D w_in_obj = w2o * w_in;

        if (w_in_obj[2] >= 0 && !bvh->intersect(r_in, &isect_sample)) {
          Vector3D f_win_wout = isect.bsdf->f(w_out, w_in_obj);
          double cos_theta = dot(Vector3D(0.0, 0.0, 1.0), w_in_obj);
          L_sample += f_win_wout * L_i * cos_theta / pdf;
        }
      }
      L_sample /= double(num_samples);
      L_out += L_sample;  // Not divided by light, sum of light
    }
    return L_out;
  }

  Vector3D PathTracer::zero_bounce_radiance(const Ray &r,
                                            const Intersection &isect)
  {
    // TODO: Part 3, Task 2
    // Returns the light that results from no bounces of light

    return isect.bsdf->get_emission();
  }

  Vector3D PathTracer::one_bounce_radiance(const Ray &r,
                                           const Intersection &isect)
  {
    // TODO: Part 3, Task 3
    // Returns either the direct illumination by hemisphere or importance sampling
    // depending on `direct_hemisphere_sample`
    if (direct_hemisphere_sample) {
      return estimate_direct_lighting_hemisphere(r, isect);
    } else {
      return estimate_direct_lighting_importance(r, isect);
    }
    // pdf
    // iterate light, intersect, light->sample(), average
  }

  Vector3D PathTracer::at_least_one_bounce_radiance(const Ray &r,
                                                    const Intersection &isect)
  {
    Matrix3x3 o2w;
    make_coord_space(o2w, isect.n);
    Matrix3x3 w2o = o2w.T();

    Vector3D hit_p = r.o + r.d * isect.t;
    Vector3D w_out = w2o * (-r.d);

    if (r.depth == 0) return Vector3D();

    Vector3D L_out = one_bounce_radiance(r, isect);
    // TODO: Part 4, Task 2
    // Returns the one bounce radiance + radiance from extra bounces at this point.
    // Should be called recursively to simulate extra bounces.

    double terminate_p = 0.3;
    if (r.depth <= 1 || coin_flip(terminate_p)) { 
      return L_out;
    }

    Vector3D w_in;
    double pdf;
    Vector3D f_i = isect.bsdf->sample_f(w_out, &w_in, &pdf);
    Ray r_sample(hit_p, o2w * w_in);
    r_sample.depth = r.depth - 1;
    r_sample.min_t = EPS_F;
    Intersection isect_sample;
    if (bvh->intersect(r_sample, &isect_sample)) {
      double cos_theta = dot(Vector3D(0.0, 0.0, 1.0), w_in);
      Vector3D L_bounce = f_i * at_least_one_bounce_radiance(r_sample, isect_sample) * cos_theta / pdf / (1 - terminate_p);
      L_out += L_bounce;
    }

    return L_out;
  }

  Vector3D PathTracer::est_radiance_global_illumination(const Ray &r)
  {
    Intersection isect;
    Vector3D L_out;

    // You will extend this in assignment 3-2.
    // If no intersection occurs, we simply return black.
    // This changes if you implement hemispherical lighting for extra credit.

    // The following line of code returns a debug color depending
    // on whether ray intersection with triangles or spheres has
    // been implemented.
    //
    // REMOVE THIS LINE when you are ready to begin Part 3.

    if (!bvh->intersect(r, &isect))
      return envLight ? envLight->sample_dir(r) : L_out;

    // normal_shading(isect.n)
    // normal -> zero_bounce

    // TODO (Part 3): Return the direct illumination.

    // TODO (Part 4): Accumulate the "direct" and "indirect"
    // parts of global illumination into L_out rather than just direct
    L_out = (isect.t == INF_D) ? debug_shading(r.d) : (zero_bounce_radiance(r, isect) + at_least_one_bounce_radiance(r, isect));
    // L_out = (isect.t == INF_D) ? debug_shading(r.d) : normal_shading(isect.n);


    return L_out;
  }

  void PathTracer::raytrace_pixel(size_t x, size_t y)
  {
    // TODO (Part 1.2):
    // Make a loop that generates num_samples camera rays and traces them
    // through the scene. Return the average Vector3D.
    // You should call est_radiance_global_illumination in this function.

    // TODO (Part 5):
    // Modify your implementation to include adaptive sampling.
    // Use the command line parameters "samplesPerBatch" and "maxTolerance"

    int num_samples = ns_aa;          // total samples to evaluate
    Vector2D origin = Vector2D(x, y); // bottom left corner of the pixel
    Vector3D radiance;
    Vector3D radiance_sum;

    double s1 = 0.0, s2 = 0.0;

    for (int i = 1; i <= num_samples; i++) {
      Vector2D sample = origin + gridSampler->get_sample();
      Ray r = camera->generate_ray(sample[0] / sampleBuffer.w, sample[1] / sampleBuffer.h);
      r.depth = max_ray_depth;
      radiance = est_radiance_global_illumination(r);
      radiance_sum += radiance;

      double illuminance = radiance.illum();
      s1 += illuminance;
      s2 += illuminance * illuminance;


      if (i % samplesPerBatch == 0) {
        double miu = s1 / double(i);
        double rou_square = (s2 - s1 * s1 / double(i)) / (double(i) - 1);
        double I = 1.96 * sqrt(rou_square) / sqrt(double(i));
        if (I <= maxTolerance * miu) {
          num_samples = i;
          break;
        }
      }
    }

    radiance = radiance_sum / num_samples;

    sampleBuffer.update_pixel(radiance, x, y);
    sampleCountBuffer[x + y * sampleBuffer.w] = num_samples;
  }

  void PathTracer::autofocus(Vector2D loc)
  {
    Ray r = camera->generate_ray(loc.x / sampleBuffer.w, loc.y / sampleBuffer.h);
    Intersection isect;

    bvh->intersect(r, &isect);

    camera->focalDistance = isect.t;
  }

} // namespace CGL

