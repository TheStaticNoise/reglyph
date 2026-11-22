#pragma once
#include <cstdint>
#include <iostream>
namespace rg::rend {
    template <typename T>
    struct vec2 {
      T c[2]{};
      vec2() = default;   
      vec2(T a, T b) : c{a, b} {}
      T& operator[] (uint64_t i) {
        if (i > 1) {
          std::cerr << "[rg.rend] bad vec2 access! dump: " << c[0] << " : " << c[1] << "! index: " << i << "\n[regraph]> Aborting...\n";
          std::abort();
        }
        return c[i];
      }
      const T& operator[](uint64_t i) const {
        if (i > 1) {
          std::cerr << "[rg.rend] bad vec2 access! index: " << i << '\n';
          std::abort();
        }
        return c[i];
      }

      vec2 operator+(const vec2& o) const {
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
      T internal_list[4]{};
      vec4() = default;   
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
      const T& operator[](uint64_t i) const {
        if (i > 3) {
          std::cerr << "[rg.rend] bad vec4 access! index: " << i << '\n';
          std::abort();
        }
        return internal_list[i];
      }
      vec4& operator+=(vec4& o) {
        internal_list[0] += o[0];
        internal_list[1] += o[1];
        internal_list[2] += o[2];
        internal_list[3] += o[3];
        return *this;
      }
    };
}
