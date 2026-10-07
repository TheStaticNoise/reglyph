#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <cstdlib>
#ifndef GLAD_GL_H_
#include <glad/gl.h>
#endif
#include <iostream>
#include <cstdint>
#include <variant>
#include <vector>
#include <memory>
#include <string>
#include <optional>

#define RGrend_FULL 0
#define RGrend_CRASH -1

namespace rg::rend {
  template <typename T>
    struct vec2 {
      T c[2];
      vec2(T a, T b) : c{a, b} {}
      T& operator[] (uint64_t i) {
        if (i > 1) {
          std::cerr << "[rg.rend] bad vec2 access! dump: " << c[0] << " : " << c[1] << "! index: " << i << "\n[regraph]> Aborting...\n";
          std::abort();
        }
        return c[i];
      }
      vec2 operator+(vec2 o) {
        return vec2(c[0] + o[0], c[1] + o[1]);
      }
      vec2& operator+=(vec2 o) {
        c[0] += o[0];
        c[1] += o[1];
        return *this;
      }
    };
  template <typename T>
    struct vec4 {
      T internal_list[4];
      vec4(T a, T b, T c, T d) {internal_list[0] = a; internal_list[1] = b; internal_list[2] = c; internal_list[3] = d;}
      T& operator[] (uint64_t i) {
        if (i > 3) {
          std::cerr << "[rg.rend] bad vec4 access! dump: " << internal_list[0] << " : " << internal_list[1] << " : " << internal_list[2] << ":" << internal_list[3] << "! index: " << i << "\n[regraph]> Aborting...\n";
          std::abort();
        }
        return internal_list[i];
      }
      vec4 operator+(vec4& o) {
        return vec4(internal_list[0] + o[0], internal_list[1] + o[1], internal_list[2] + o[2], internal_list[3] + o[3]);
      }
      vec4& operator+=(vec4& o) {
        internal_list[0] += o[0];
        internal_list[1] += o[1];
        internal_list[2] += o[2];
        internal_list[3] += o[3];
        return *this;
      }
    };

  class rend_obj {
    public:
      rend_obj() {
        glGenVertexArrays(1, &rect_vao);
        glGenBuffers(1, &rect_vbo);
        glBindVertexArray(rect_vao);
        glBindBuffer(GL_ARRAY_BUFFER, rect_vbo);
        glVertexAttribPointer(
            0,                  // location
            2,                  // x, y
            GL_FLOAT,
            GL_FALSE,
            6 * sizeof(float),  // stride
            (void*)0             // offset
            );
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(
            1,                  // location
            4,                  // r,g,b,a
            GL_FLOAT,
            GL_FALSE,
            6 * sizeof(float),
            (void*)(2 * sizeof(float))
            );
        glEnableVertexAttribArray(1);
      }
    private:
      GLuint rect_vao, rect_vbo;
      vec2<float> ndc(vec2<int> pos, vec2<int> size) {
        return {(static_cast<float>(pos[0]) / size[0]) * 2.0f - 1.0f, 1.0f - (static_cast<float>(pos[1]) / size[1]) * 2.0f};
      }
      // size format: X Y
      // color format: R G B A
      void render_rect(vec2<int> pos, vec2<int> size, vec2<int> screen_size, vec4<float> color) {
        auto a = ndc(pos, screen_size);
        auto b = ndc({pos[0] + size[0], pos[1]}, screen_size);
        auto c = ndc({pos[0], pos[1] + size[1]}, screen_size);
        auto d = ndc(pos + size, screen_size);

        float v[] = { // layout: x y r g b a 
                      // first triangle
          a[0], a[1], color[0], color[1], color[2], color[3], 
          b[0], b[1], color[0], color[1], color[2], color[3],
          c[0], c[1], color[0], color[1], color[2], color[3],
          // second
          b[0], b[1], color[0], color[1], color[2], color[3], 
          d[0], d[1], color[0], color[1], color[2], color[3], 
          c[0], c[1], color[0], color[1], color[2], color[3], 
        };

        GLuint vao = rect_vao, vbo = rect_vbo;
        
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, 6);
      }
    public:
      inline void draw_rect(vec2<int> pos, vec2<int> size, vec2<int> screen_size, vec4<float> color) {
        render_rect(pos, size, screen_size, color);
      }
  };
}
