#include "rasterizer.h"
#include <iostream>

using namespace std;

namespace CGL {

  RasterizerImp::RasterizerImp(PixelSampleMethod psm, LevelSampleMethod lsm,
    size_t width, size_t height,
    unsigned int sample_rate) {
    this->psm = psm;
    this->lsm = lsm;
    this->width = width;
    this->height = height;
    this->sample_rate = sample_rate;

    sample_buffer.resize(width * height, Color::White);
    // z-buffer, infinite, pick smaller one
  }

  // Used by rasterize_point and rasterize_line
  void RasterizerImp::fill_pixel(size_t x, size_t y, Color c) {
    // TODO: Task 2: You might need to this function to fix points and lines (such as the black rectangle border in test4.svg)
    // NOTE: You are not required to implement proper supersampling for points and lines
    // It is sufficient to use the same color for all supersamples of a pixel for points and lines (not triangles)
      sample_buffer[y * width + x] = c;
  }

  // Rasterize a point: simple example to help you start familiarizing
  // yourself with the starter code.
  //
  void RasterizerImp::rasterize_point(float x, float y, Color color) {
    // fill in the nearest pixel
    int sx = (int)floor(x);
    int sy = (int)floor(y);

    // check bounds
    if (sx < 0 || sx >= width) return;
    if (sy < 0 || sy >= height) return;

    fill_pixel(sx, sy, color);
    return;
  }

  // Rasterize a line.
  void RasterizerImp::rasterize_line(float x0, float y0,
    float x1, float y1,
    Color color) {
    if (x0 > x1) {
      swap(x0, x1); swap(y0, y1);
    }

    float pt[] = { x0,y0 };
    float m = (y1 - y0) / (x1 - x0);
    float dpt[] = { 1,m };
    int steep = abs(m) > 1;
    if (steep) {
      dpt[0] = x1 == x0 ? 0 : 1 / abs(m);
      dpt[1] = x1 == x0 ? (y1 - y0) / abs(y1 - y0) : m / abs(m);
    }

    while (floor(pt[0]) <= floor(x1) && abs(pt[1] - y0) <= abs(y1 - y0)) {
      rasterize_point(pt[0], pt[1], color);
      pt[0] += dpt[0]; pt[1] += dpt[1];
    }
  }

  // Rasterize a triangle.
  void RasterizerImp::rasterize_triangle(float x0, float y0,
    float x1, float y1,
    float x2, float y2,
    Color color) {
    // TODO: Task 1: Implement basic triangle rasterization here, no supersampling
    
    // float xMin = min(x0, min(x1, x2));
    // float xMax = max(x0, max(x1, x2));
    // float yMin = min(y0, min(y1, y2));
    // float yMax = max(y0, max(y1, y2));

    // for (int x = floor(xMin); x <= floor(xMax); x++) {
    //   for (int y = floor(yMin); y <= floor(yMax); y++) {
    //     if (inside_tri(x + 0.5, y + 0.5, x0, y0, x1, y1, x2, y2)) {
    //       rasterize_point(x, y, color);
    //     }
    //   }
    // }


    // TODO: Task 2: Update to implement super-sampled rasterization
    float xMin = min(x0, min(x1, x2));
    float xMax = max(x0, max(x1, x2));
    float yMin = min(y0, min(y1, y2));
    float yMax = max(y0, max(y1, y2));

    float step = 1.0 / sqrt(sample_rate);
    int length = sqrt(sample_rate);
    // cout << color.r << " " << color.g << " " << color.b << " "; 

    for (float x = floor(xMin); x <= floor(xMax); x++) {
      for (float y = floor(yMin); y <= floor(yMax); y++) {
        float count = 0;
        for (int idx = 0; idx < sample_rate; idx++) {
          float xSample = x + step / 2 + step * (idx / length);
          float ySample = y + step / 2 + step * (idx % length);
          if (inside_tri(xSample, ySample, x0, y0, x1, y1, x2, y2)) count++;
        }
        if (count > 0) {  
          Color c;
          Color bg = sample_buffer[y * width + x];
          if (bg != Color::White) {
            rasterize_point(x, y, color);
            continue;
          }
          float ratio = count / sample_rate;
          c.r = color.r * ratio + bg.r * (1 - ratio);
          c.g = color.g * ratio + bg.g * (1 - ratio);
          c.b = color.b * ratio + bg.b * (1 - ratio);
          rasterize_point(x, y, c);
        }
      }
    }


    // Extra, SSAA filtering, convolution
    // float xMin = min(x0, min(x1, x2));
    // float xMax = max(x0, max(x1, x2));
    // float yMin = min(y0, min(y1, y2));
    // float yMax = max(y0, max(y1, y2));

    // // rasterize once
    // for (float x = floor(xMin); x <= floor(xMax); x++) {
    //   for (float y = floor(yMin); y <= floor(yMax); y++) {
    //     if (inside_tri(x + 0.5, y + 0.5, x0, y0, x1, y1, x2, y2)) {
    //       rasterize_point(x, y, color);
    //     }
    //   }
    // }

    // vector<Color> copy_buffer = this->sample_buffer;
    // int length = sqrt(sample_rate);

    // for (float x = floor(xMin); x <= floor(xMax); x++) {
    //   for (float y = floor(yMin); y <= floor(yMax); y++) {
    //     Color c;
    //     float red = 0.0, green = 0.0, blue = 0.0;
    //     for (int idx = 0; idx < sample_rate; idx++) {
    //       float xSample = x - (length - 1) / 2 +  idx / length;
    //       float ySample = y - (length - 1) / 2 + idx % length;
    //       red += copy_buffer[ySample * width + xSample].r;
    //       green += copy_buffer[ySample * width + xSample].g;
    //       blue += copy_buffer[ySample * width + xSample].b;
    //     }
        
    //     c.r = red / sample_rate; c.g = green / sample_rate; c.b = blue / sample_rate;
    //     rasterize_point(x, y, c);  
    //   }
    // }
  }


  void RasterizerImp::rasterize_interpolated_color_triangle(float x0, float y0, Color c0,
    float x1, float y1, Color c1,
    float x2, float y2, Color c2)
  {
    // TODO: Task 4: Rasterize the triangle, calculating barycentric coordinates and using them to interpolate vertex colors across the triangle
    // Hint: You can reuse code from rasterize_triangle
    float xMin = min(x0, min(x1, x2));
    float xMax = max(x0, max(x1, x2));
    float yMin = min(y0, min(y1, y2));
    float yMax = max(y0, max(y1, y2));

    float step = 1.0 / sqrt(sample_rate);
    int length = sqrt(sample_rate);

    for (int x = floor(xMin); x <= floor(xMax); x++) {
      for (int y = floor(yMin); y <= floor(yMax); y++) {
        float count = 0;
        for (int idx = 0; idx < sample_rate; idx++) {
          float xSample = x + step / 2 + step * (idx / length);
          float ySample = y + step / 2 + step * (idx % length);
          if (inside_tri(xSample, ySample, x0, y0, x1, y1, x2, y2)) count++;
        }
        if (count > 0) {  
          Color c;
          float xMip = x + 0.5;
          float yMip = y + 0.5;
          float alpha = (-(xMip - x1) * (y2 - y1) + (yMip - y1) * (x2 - x1)) / (-(x0 - x1) * (y2 - y1) + (y0 - y1) * (x2 - x1));
          float beta = (-(xMip - x2) * (y0 - y2) + (yMip - y2) * (x0 - x2)) / (-(x1 - x2) * (y0 - y2) + (y1 - y2) * (x0 - x2));
          // float gamma = (-(x - x0) * (y1 - y0) + (y - y0) * (x1 - x0)) / (-(x2 - x0) * (y1 - y0) + (y2 - y0) * (x1 - x0));
          float gamma = 1 - alpha - beta;
          c.r = alpha * c0.r + beta * c1.r + gamma * c2.r;
          c.g = alpha * c0.g + beta * c1.g + gamma * c2.g;
          c.b = alpha * c0.b + beta * c1.b + gamma * c2.b;

          Color bg = sample_buffer[y * width + x];
          if (bg != Color::White) {
            rasterize_point(x, y, c);
            continue;
          }

          float ratio = count / sample_rate;
          c.r = c.r * ratio + bg.r * (1 - ratio);
          c.g = c.g * ratio + bg.g * (1 - ratio);
          c.b = c.b * ratio + bg.b * (1 - ratio);
          rasterize_point(x, y, c);
        }

      }                           
    }


  }


  void RasterizerImp::rasterize_textured_triangle(float x0, float y0, float u0, float v0,
    float x1, float y1, float u1, float v1,
    float x2, float y2, float u2, float v2,
    Texture& tex)
  {

    // TODO: Task 5: Fill in the SampleParams struct and pass it to the tex.sample function.
    // TODO: Task 6: Set the correct barycentric differentials in the SampleParams struct.
    // Hint: You can reuse code from rasterize_triangle/rasterize_interpolated_color_triangle

    SampleParams sp;
    sp.lsm = lsm; sp.psm = psm;

    float xMin = min(x0, min(x1, x2));
    float xMax = max(x0, max(x1, x2));
    float yMin = min(y0, min(y1, y2));
    float yMax = max(y0, max(y1, y2));

    float step = 1.0 / sqrt(sample_rate);
    int length = sqrt(sample_rate);

    for (int x = floor(xMin); x <= floor(xMax); x++) {
      for (int y = floor(yMin); y <= floor(yMax); y++) {
        float count = 0;
        for (int idx = 0; idx < sample_rate; idx++) {
          float xSample = x + step / 2 + step * (idx / length);
          float ySample = y + step / 2 + step * (idx % length);
          if (inside_tri(xSample, ySample, x0, y0, x1, y1, x2, y2)) count++;
        }
        if (count > 0) {  
          for (int k = 0; k < 3; k++) {
            float xMip = x + 0.5 + k % 2;
            float yMip = y + 0.5 + k / 2;
            float alpha = (-(xMip - x1) * (y2 - y1) + (yMip - y1) * (x2 - x1)) / (-(x0 - x1) * (y2 - y1) + (y0 - y1) * (x2 - x1));
            float beta = (-(xMip - x2) * (y0 - y2) + (yMip - y2) * (x0 - x2)) / (-(x1 - x2) * (y0 - y2) + (y1 - y2) * (x0 - x2));
            // float gamma = (-(x - x0) * (y1 - y0) + (y - y0) * (x1 - x0)) / (-(x2 - x0) * (y1 - y0) + (y2 - y0) * (x1 - x0));
            float gamma = 1 - alpha - beta;
            float u = (alpha * u0 + beta * u1 + gamma * u2);
            float v = (alpha * v0 + beta * v1 + gamma * v2);

            if (k == 0) sp.p_uv = {u, v};
            if (k == 1) sp.p_dx_uv = {u, v};
            if (k == 2) sp.p_dy_uv = {u, v};
          }

          Color c = tex.sample(sp);
          Color bg = sample_buffer[y * width + x];
          if (bg != Color::White) {
            rasterize_point(x, y, c);
            continue;
          }

          float ratio = count / sample_rate;
          c.r = c.r * ratio + bg.r * (1 - ratio);
          c.g = c.g * ratio + bg.g * (1 - ratio);
          c.b = c.b * ratio + bg.b * (1 - ratio);
          rasterize_point(x, y, c); 
        }
      }                           
    }
  }

  void RasterizerImp::set_sample_rate(unsigned int rate) {
    // TODO: Task 2: You may want to update this function for supersampling support

    this->sample_rate = rate;
    this->sample_buffer.resize(width * height, Color::White);
  }


  void RasterizerImp::set_framebuffer_target(unsigned char* rgb_framebuffer,
    size_t width, size_t height)
  {
    // TODO: Task 2: You may want to update this function for supersampling support

    this->width = width;
    this->height = height;
    this->rgb_framebuffer_target = rgb_framebuffer;


    this->sample_buffer.resize(width * height, Color::White);
  }


  void RasterizerImp::clear_buffers() {
    std::fill(rgb_framebuffer_target, rgb_framebuffer_target + 3 * width * height, 255);
    std::fill(sample_buffer.begin(), sample_buffer.end(), Color::White);
  }


  // This function is called at the end of rasterizing all elements of the
  // SVG file.  If you use a supersample buffer to rasterize SVG elements
  // for antialising, you could use this call to fill the target framebuffer
  // pixels from the supersample buffer data.
  //
  void RasterizerImp::resolve_to_framebuffer() {
    // TODO: Task 2: You will likely want to update this function for supersampling support


    for (int x = 0; x < width; ++x) {
      for (int y = 0; y < height; ++y) {
        Color col = sample_buffer[y * width + x];
        for (int k = 0; k < 3; k++) {
          this->rgb_framebuffer_target[3 * (y * width + x) + k] = (&col.r)[k] * 255;
        }
      }
    }
  }

  bool RasterizerImp::inside_tri(float x, float y,
      float x0, float y0,
      float x1, float y1,
      float x2, float y2) {

    float L0 = -(x - x0) * (y1 - y0) + (y - y0) * (x1 - x0);
    float L1 = -(x - x1) * (y2 - y1) + (y - y1) * (x2 - x1);
    float L2 = -(x - x2) * (y0 - y2) + (y - y2) * (x0 - x2);
    if ((L0 >= 0 && L1 >= 0 && L2 >= 0) || (L0 <= 0 && L1 <= 0 && L2 <= 0)) {
      return true;
    }
    return false;
  }

  Rasterizer::~Rasterizer() { }


}// CGL
