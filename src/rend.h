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
#ifndef RG_REND_Z_ORDERS
#define RG_REND_Z_ORDERS 8
#endif

const char* RG_RECT_VERTEX = 
"#version 330 core\n"
"layout (location = 0) in vec2 posData;\n"
"layout (location = 1) in vec4 Coldata;\n"
"out vec4 col;\n"
"void main() {\n"
"gl_Position=vec4(posData, 0.0, 1.0);\n"
"col = Coldata;\n"
"}";
const char* RG_RECT_FRAGMENTS = 
"#version 330 core\n"
"in vec4 col;\n"
"out vec4 FragColor;\n"
"void main() {\n"
"FragColor = col;\n"
"}";



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
  struct rect {
    vec2<int64_t> pos = {0, 0};
    vec2<int64_t> size = {0, 0};
    vec4<float>   color = {0.0f, 0.0f, 0.0f, 0.0f};
  };
  struct text {
    std::string text;
    int64_t fontid;
  };
  class rend_obj {
    private: 
      GLint rectsh = 0, textsh = -1;
      struct shader_pack {
        GLuint id = 0;
        GLint status = 0;
      };

      shader_pack create_shader(const char* src, GLenum type) {
        shader_pack pack{};
        pack.id = glCreateShader(type);
        glShaderSource(pack.id, 1, &src, NULL);
        glCompileShader(pack.id);
        glGetShaderiv(pack.id, GL_COMPILE_STATUS, &pack.status);
        return pack;
      }
    public:
      rend_obj() {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
        shader_pack vertex = create_shader(RG_RECT_VERTEX, GL_VERTEX_SHADER);
        if (!vertex.status) {
          std::cerr << "[FAIL]\ncompiling vertex shader fail!\n";
          abort();
        }
        shader_pack fragment = create_shader(RG_RECT_FRAGMENTS, GL_FRAGMENT_SHADER);
        if (!fragment.status) {
          std::cerr << "[FAIL]\nfailed compilation of fragment shader!\n";
          abort();
        }
        GLuint program = glCreateProgram();

        glAttachShader(program, vertex.id);
        glAttachShader(program, fragment.id);

        glLinkProgram(program);

        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);

        char log[512]{};
        if (!success) {
          glGetProgramInfoLog(program, sizeof(log), nullptr, log);
          std::cerr << "Shader compilation failed:\n" << log << '\n';
          abort();
        }

        glDeleteShader(vertex.id);
        glDeleteShader(fragment.id);
        rectsh = program;
      }
    private:
      GLuint rect_vao, rect_vbo;
      std::array<std::vector<std::variant<rect, text>>, RG_REND_Z_ORDERS> objects;
      vec2<float> ndc(vec2<int64_t> pos, vec2<int64_t> size) {
        return {(static_cast<float>(pos[0]) / size[0]) * 2.0f - 1.0f, 1.0f - (static_cast<float>(pos[1]) / size[1]) * 2.0f};
      }
      vec2<int64_t> screensize;
      // size format: X Y
      // color format: R G B A
      void render_rect(vec2<int64_t> pos, vec2<int64_t> size, vec4<float> color) {
        auto a = ndc(pos, screensize);
        auto b = ndc({pos[0] + size[0], pos[1]}, screensize);
        auto c = ndc({pos[0], pos[1] + size[1]}, screensize);
        auto d = ndc(pos + size, screensize);

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
      inline void schedule_rect(rect rect_obj, int64_t layer) {
        objects[layer].push_back(rect_obj);
      }
      inline void render() {
        struct vertex {
          float x, y;
          float r, g, b, a;
        };

        // Avoid division by zero in ndc().
        if (screensize[0] <= 0 || screensize[1] <= 0) {
          std::cerr << "[rg.rend] invalid screen dimensions\n";
          return;
        }

        glUseProgram(rectsh);

        for (auto& layer : objects) {
          std::vector<vertex> vertices_d;
          std::vector<text> texts;

          // Count rectangles to reserve exactly enough space.
          size_t rect_count = 0;

          for (const auto& object : layer) {
            if (std::holds_alternative<rect>(object)) {
              ++rect_count;
            } else if (std::holds_alternative<text>(object)) {
              texts.push_back(std::get<text>(object));
            }
          }

          vertices_d.reserve(rect_count * 6);

          // Generate rectangle vertices.
          for (const auto& object : layer) {
            const rect* rect_ = std::get_if<rect>(&object);

            if (!rect_)
              continue;

            auto a = ndc(rect_->pos, screensize);
            auto b = ndc(
                {rect_->pos[0] + rect_->size[0], rect_->pos[1]},
                screensize
                );
            auto c = ndc(
                {rect_->pos[0], rect_->pos[1] + rect_->size[1]},
                screensize
                );
            auto d = ndc(rect_->pos + rect_->size, screensize);

            const auto& color = rect_->color;

            // First triangle.
            vertices_d.push_back({
                a[0], a[1],
                color[0], color[1], color[2], color[3]
                });

            vertices_d.push_back({
                b[0], b[1],
                color[0], color[1], color[2], color[3]
                });

            vertices_d.push_back({
                c[0], c[1],
                color[0], color[1], color[2], color[3]
                });

            // Second triangle.
            vertices_d.push_back({
                b[0], b[1],
                color[0], color[1], color[2], color[3]
                });

            vertices_d.push_back({
                d[0], d[1],
                color[0], color[1], color[2], color[3]
                });

            vertices_d.push_back({
                c[0], c[1],
                color[0], color[1], color[2], color[3]
                });
          }

          // Upload and draw the entire rectangle batch.
          if (!vertices_d.empty()) {
            glBindVertexArray(rect_vao);
            glBindBuffer(GL_ARRAY_BUFFER, rect_vbo);

            glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(
                  vertices_d.size() * sizeof(vertex)
                  ),
                vertices_d.data(),
                GL_DYNAMIC_DRAW
                );

            glDrawArrays(
                GL_TRIANGLES,
                0,
                static_cast<GLsizei>(vertices_d.size())
                );
          }
        }

        glBindVertexArray(0);
        glUseProgram(0);
      }
    public:
      inline void set_screensize(vec2<int64_t> screen_size) {
        screensize = screen_size;
      }
      inline void add_rect(vec2<int64_t> pos, vec2<int64_t> size, vec4<float> color, int64_t layer) {
        schedule_rect({pos, size, color}, layer);
      }
      inline void submit() {
        render();
      }
      inline void clear() {
        for (auto& l : objects) l.clear();
      }

  };
}
