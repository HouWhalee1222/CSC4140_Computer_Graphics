#include "texture.h"
#include "CGL/color.h"

#include <cmath>
#include <algorithm>

namespace CGL {

  Color Texture::sample(const SampleParams& sp) {
    // TODO: Task 6: Fill this in.
    if (sp.lsm == L_ZERO) {

      if (sp.psm == P_NEAREST) {
        return sample_nearest(sp.p_uv);
      } else if (sp.psm == P_LINEAR) {
        return sample_bilinear(sp.p_uv);
      }
      
    } 
    
    else if (sp.lsm == L_NEAREST) {
      int level = round(get_level(sp));

      if (sp.psm == P_NEAREST) {
        return sample_nearest(sp.p_uv, level);
      } else if (sp.psm == P_LINEAR) {
        return sample_bilinear(sp.p_uv, level);
      }

    } 
    
    else if (sp.lsm == L_LINEAR) {
      float level = get_level(sp);
      float r = level - floor(level);

      if (sp.psm == P_NEAREST) {
        return lerp(r, sample_nearest(sp.p_uv, floor(level)), sample_nearest(sp.p_uv, ceil(level)));
      } else if (sp.psm == P_LINEAR) {
        return lerp(r, sample_bilinear(sp.p_uv, floor(level)), sample_bilinear(sp.p_uv, ceil(level)));
      }
    }

// return magenta for invalid level
    return Color(1, 0, 1);
  }

  float Texture::get_level(const SampleParams& sp) {
    // TODO: Task 6: Fill this in.
    float du_dx = (sp.p_dx_uv[0] - sp.p_uv[0]) * this->width;
    float du_dy = (sp.p_dy_uv[0] - sp.p_uv[0]) * this->width;
    float dv_dx = (sp.p_dx_uv[1] - sp.p_uv[1]) * this->height;
    float dv_dy = (sp.p_dy_uv[1] - sp.p_uv[1]) * this->height;
    
    float L = max(sqrt(du_dx * du_dx + dv_dx * dv_dx), sqrt(du_dy * du_dy + dv_dy * dv_dy));
    return log2f(L);
  }

  Color MipLevel::get_texel(int tx, int ty) {
    return Color(&texels[tx * 3 + ty * width * 3]);
  }

  Color Texture::sample_nearest(Vector2D uv, int level) {
    // TODO: Task 5: Fill this in.
    // return magenta for invalid level
    if (level < 0) return Color(1, 0, 1);

    auto& mip = mipmap[level];
    int tx = round(uv[0] * mip.width);
    int ty = round(uv[1] * mip.height);
    return mip.get_texel(tx, ty);

  }

  Color Texture::sample_bilinear(Vector2D uv, int level) {
    // TODO: Task 5: Fill this in.
    // return magenta for invalid level
    if (level < 0) return Color(1, 0, 1);

    auto& mip = mipmap[level];
    float u = uv[0] * mip.width;
    float v = uv[1] * mip.height;

    float s = u - round(u) + 0.5;
    float t = v - round(v) + 0.5;
    Color c00 = mip.get_texel(round(u) - 1, round(v) - 1);
    Color c10 = mip.get_texel(round(u), round(v) - 1);
    Color c01 = mip.get_texel(round(u) - 1, round(v));
    Color c11 = mip.get_texel(round(u), round(v));
    Color c0 = lerp(s, c00, c10);
    Color c1 = lerp(s, c01, c11);
    return lerp(t, c0, c1);
  }

  Color Texture::lerp(float x, Color v0, Color v1) {
    Color c;
    c.r = v0.r + x * (v1.r - v0.r);
    c.g = v0.g + x * (v1.g - v0.g);
    c.b = v0.b + x * (v1.b - v0.b);
    return c;
  }



  /****************************************************************************/

  // Helpers

  inline void uint8_to_float(float dst[3], unsigned char* src) {
    uint8_t* src_uint8 = (uint8_t*)src;
    dst[0] = src_uint8[0] / 255.f;
    dst[1] = src_uint8[1] / 255.f;
    dst[2] = src_uint8[2] / 255.f;
  }

  inline void float_to_uint8(unsigned char* dst, float src[3]) {
    uint8_t* dst_uint8 = (uint8_t*)dst;
    dst_uint8[0] = (uint8_t)(255.f * max(0.0f, min(1.0f, src[0])));
    dst_uint8[1] = (uint8_t)(255.f * max(0.0f, min(1.0f, src[1])));
    dst_uint8[2] = (uint8_t)(255.f * max(0.0f, min(1.0f, src[2])));
  }

  void Texture::generate_mips(int startLevel) {

    // make sure there's a valid texture
    if (startLevel >= mipmap.size()) {
      std::cerr << "Invalid start level";
    }

    // allocate sublevels
    int baseWidth = mipmap[startLevel].width;
    int baseHeight = mipmap[startLevel].height;
    int numSubLevels = (int)(log2f((float)max(baseWidth, baseHeight)));

    numSubLevels = min(numSubLevels, kMaxMipLevels - startLevel - 1);
    mipmap.resize(startLevel + numSubLevels + 1);

    int width = baseWidth;
    int height = baseHeight;
    for (int i = 1; i <= numSubLevels; i++) {

      MipLevel& level = mipmap[startLevel + i];

      // handle odd size texture by rounding down
      width = max(1, width / 2);
      //assert (width > 0);
      height = max(1, height / 2);
      //assert (height > 0);

      level.width = width;
      level.height = height;
      level.texels = vector<unsigned char>(3 * width * height);
    }

    // create mips
    int subLevels = numSubLevels - (startLevel + 1);
    for (int mipLevel = startLevel + 1; mipLevel < startLevel + subLevels + 1;
      mipLevel++) {

      MipLevel& prevLevel = mipmap[mipLevel - 1];
      MipLevel& currLevel = mipmap[mipLevel];

      int prevLevelPitch = prevLevel.width * 3; // 32 bit RGB
      int currLevelPitch = currLevel.width * 3; // 32 bit RGB

      unsigned char* prevLevelMem;
      unsigned char* currLevelMem;

      currLevelMem = (unsigned char*)&currLevel.texels[0];
      prevLevelMem = (unsigned char*)&prevLevel.texels[0];

      float wDecimal, wNorm, wWeight[3];
      int wSupport;
      float hDecimal, hNorm, hWeight[3];
      int hSupport;

      float result[3];
      float input[3];

      // conditional differentiates no rounding case from round down case
      if (prevLevel.width & 1) {
        wSupport = 3;
        wDecimal = 1.0f / (float)currLevel.width;
      }
      else {
        wSupport = 2;
        wDecimal = 0.0f;
      }

      // conditional differentiates no rounding case from round down case
      if (prevLevel.height & 1) {
        hSupport = 3;
        hDecimal = 1.0f / (float)currLevel.height;
      }
      else {
        hSupport = 2;
        hDecimal = 0.0f;
      }

      wNorm = 1.0f / (2.0f + wDecimal);
      hNorm = 1.0f / (2.0f + hDecimal);

      // case 1: reduction only in horizontal size (vertical size is 1)
      if (currLevel.height == prevLevel.height) {
        //assert (currLevel.height == 1);

        for (int i = 0; i < currLevel.width; i++) {
          wWeight[0] = wNorm * (1.0f - wDecimal * i);
          wWeight[1] = wNorm * 1.0f;
          wWeight[2] = wNorm * wDecimal * (i + 1);

          result[0] = result[1] = result[2] = 0.0f;

          for (int ii = 0; ii < wSupport; ii++) {
            uint8_to_float(input, prevLevelMem + 3 * (2 * i + ii));
            result[0] += wWeight[ii] * input[0];
            result[1] += wWeight[ii] * input[1];
            result[2] += wWeight[ii] * input[2];
          }

          // convert back to format of the texture
          float_to_uint8(currLevelMem + (3 * i), result);
        }

        // case 2: reduction only in vertical size (horizontal size is 1)
      }
      else if (currLevel.width == prevLevel.width) {
        //assert (currLevel.width == 1);

        for (int j = 0; j < currLevel.height; j++) {
          hWeight[0] = hNorm * (1.0f - hDecimal * j);
          hWeight[1] = hNorm;
          hWeight[2] = hNorm * hDecimal * (j + 1);

          result[0] = result[1] = result[2] = 0.0f;
          for (int jj = 0; jj < hSupport; jj++) {
            uint8_to_float(input, prevLevelMem + prevLevelPitch * (2 * j + jj));
            result[0] += hWeight[jj] * input[0];
            result[1] += hWeight[jj] * input[1];
            result[2] += hWeight[jj] * input[2];
          }

          // convert back to format of the texture
          float_to_uint8(currLevelMem + (currLevelPitch * j), result);
        }

        // case 3: reduction in both horizontal and vertical size
      }
      else {

        for (int j = 0; j < currLevel.height; j++) {
          hWeight[0] = hNorm * (1.0f - hDecimal * j);
          hWeight[1] = hNorm;
          hWeight[2] = hNorm * hDecimal * (j + 1);

          for (int i = 0; i < currLevel.width; i++) {
            wWeight[0] = wNorm * (1.0f - wDecimal * i);
            wWeight[1] = wNorm * 1.0f;
            wWeight[2] = wNorm * wDecimal * (i + 1);

            result[0] = result[1] = result[2] = 0.0f;

            // convolve source image with a trapezoidal filter.
            // in the case of no rounding this is just a box filter of width 2.
            // in the general case, the support region is 3x3.
            for (int jj = 0; jj < hSupport; jj++)
              for (int ii = 0; ii < wSupport; ii++) {
                float weight = hWeight[jj] * wWeight[ii];
                uint8_to_float(input, prevLevelMem +
                  prevLevelPitch * (2 * j + jj) +
                  3 * (2 * i + ii));
                result[0] += weight * input[0];
                result[1] += weight * input[1];
                result[2] += weight * input[2];
              }

            // convert back to format of the texture
            float_to_uint8(currLevelMem + currLevelPitch * j + 3 * i, result);
          }
        }
      }
    }
  }

}
