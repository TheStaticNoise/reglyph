#pragma once

#include "glyph.h"
#include <stdexcept>
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
#include "vecx.h"

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
const char* RG_TEX_VERTEX = 
"#version 330 core\n"
"layout (location = 0) in vec2 position;\n"
"layout (location = 1) in vec2 texCoord;\n"
"out vec2 uv;\n"
"void main() {"
"gl_Position = vec4(position, 0.0, 1.0);"
"uv = texCoord;"
"}";
const char* RG_TEX_FRAGMENT =
"#version 330 core\n"
"in vec2 uv;\n"
"out vec4 color;\n"
"uniform sampler2D atlas;\n"
"uniform vec4 filter;\n"
"void main() {\n"
"float alpha = texture(atlas, uv).r;\n"
"color = vec4(filter.rgb, filter.a * alpha);\n"
"}";

namespace rg::rend {
  struct rect {
    vec2<int64_t> pos = {0, 0};
    vec2<int64_t> size = {0, 0};
    vec4<float>   color = {0.0f, 0.0f, 0.0f, 0.0f};
  };
  struct text {
    vec2<int64_t> pos;
    std::string text;
    int64_t fontid;
  };
  class rend_obj {
    private: 
      GLint rectsh = -1, textsh = -1;
      GLuint rect_vao, rect_vbo;
      GLuint char_vao, char_vbo;

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
        // texture shader
        shader_pack tv = create_shader(RG_TEX_VERTEX, GL_VERTEX_SHADER);
        shader_pack tf = create_shader(RG_TEX_FRAGMENT, GL_FRAGMENT_SHADER);
        // check status + glGetShaderInfoLog on both, like the rect path
        GLuint tp = glCreateProgram();
        glAttachShader(tp, tv.id);
        glAttachShader(tp, tf.id);
        glLinkProgram(tp);
        // check GL_LINK_STATUS
        glDeleteShader(tv.id);
        glDeleteShader(tf.id);
        textsh = tp;
        glGenVertexArrays(1, &char_vao);
        glGenBuffers(1, &char_vbo);
        glBindVertexArray(char_vao);
        glBindBuffer(GL_ARRAY_BUFFER, char_vbo);
        glVertexAttribPointer(
            0,                  // Shader location 0
            2,                  // Read 2 floats
            GL_FLOAT,
            GL_FALSE,
            4 * sizeof(float),  // Skip to next vertex
            (void*)0            // Start at x
            );
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            1,                         // Shader location 1
            2,                         // Read 2 floats
            GL_FLOAT,
            GL_FALSE,
            4 * sizeof(float),         // Same vertex stride
            (void*)(2 * sizeof(float)) // Skip x and y
            );
        glEnableVertexAttribArray(1);
      }
    private:
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
      inline void schedule_text(text textobj, int64_t layer) {
        objects[layer].push_back(textobj);
      }
      inline void render() {
        struct vertex {
          float x, y;
          float r, g, b, a;
        };
        auto uv = [](rg::rend::vec2<int64_t> pos, rg::rend::vec2<int64_t> size) {
          return rg::rend::vec2<float>{
            (float)pos[0] / (float)size[0],
              (float)pos[1] / (float)size[1]
          };
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
            } // otherwise ignored;
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
          // draw text
          struct text_vertex {
            float x, y, u, v;
          };
          std::vector<text_vertex> vertices;
          uint64_t total_chars = 0;
          // preprocessing
          for (auto& text : texts) {
            if (text.fontid >= fonts.size()) throw std::runtime_error("text err Out of ID Range");
            total_chars += text.text.size();
          }
          // processing
          vertices.reserve(total_chars * 6);
          for (auto& text : texts) {
            int64_t textpos = 0;
            for (auto& ch : text.text) {
              if ((unsigned char)ch < 32) continue;
              auto glyph = fonts[text.fontid].get_glyph(ch);
              if (!glyph.has_value()) continue;
              auto glyphd = glyph.value();
              rg::rend::vec2<int64_t> mask = {textpos, 0};
              auto a = ndc(text.pos + mask, screensize);
              auto b = ndc({text.pos[0] + glyphd.size[0] + mask[0], text.pos[1]}, screensize);
              auto c = ndc({text.pos[0] + textpos, text.pos[1] + glyphd.size[1]}, screensize);
              auto d = ndc(text.pos + glyphd.size + mask, screensize);
              /*a[0], a[1], color[0], color[1], color[2], color[3], 
                b[0], b[1], color[0], color[1], color[2], color[3],
                c[0], c[1], color[0], color[1], color[2], color[3],
              // second
              b[0], b[1], color[0], color[1], color[2], color[3], 
              d[0], d[1], color[0], color[1], color[2], color[3], 
              c[0], c[1], color[0], color[1], color[2], color[3], */
              /* format: x y u v */
              auto asize = fonts[text.fontid].get_atlas_size();
              auto aa = uv(glyphd.atlas_pos, asize);
              auto ab = uv({glyphd.atlas_pos[0] + glyphd.size[0], glyphd.atlas_pos[1]}, asize);
              auto ac = uv({glyphd.atlas_pos[0], glyphd.atlas_pos[1] + glyphd.size[1]}, asize);
              auto ad = uv(glyphd.atlas_pos + glyphd.size, asize);
              vertices.push_back({a[0], a[1], aa[0], aa[1]});
              vertices.push_back({b[0], b[1], ab[0], ab[1]});
              vertices.push_back({c[0], c[1], ac[0], ac[1]});
              vertices.push_back({b[0], b[1], ab[0], ab[1]});
              vertices.push_back({d[0], d[1], ad[0], ad[1]});
              vertices.push_back({c[0], c[1], ac[0], ac[1]});
              textpos+=(glyphd.advance[0] >> 6);
            }
            // submit
            if (!vertices.empty()) {
              glUseProgram(textsh);
              glUniform4f(glGetUniformLocation(textsh, "filter"), 1.0, 1.0, 1.0, 1.0);
              glActiveTexture(GL_TEXTURE0);
              glBindTexture(GL_TEXTURE_2D, fonts[text.fontid].get_texture());

              glBindVertexArray(char_vao);
              glBindBuffer(GL_ARRAY_BUFFER, char_vbo);
              glBufferData(
                  GL_ARRAY_BUFFER,
                  static_cast<GLsizeiptr>(
                    vertices.size() * sizeof(text_vertex)
                    ),
                  vertices.data(),
                  GL_DYNAMIC_DRAW
                  );

              glDrawArrays(
                  GL_TRIANGLES,
                  0,
                  static_cast<GLsizei>(vertices.size())
                  );
            }
            glBindVertexArray(0);
            glUseProgram(0);
          }

        }
      }
      std::vector<rg::glyph::glyphManager> fonts;
    public:
      inline void set_screensize(vec2<int64_t> screen_size) {
        screensize = screen_size;
      }
      inline void clear_from(uint64_t inclusive_layer) {
        for (int64_t i = inclusive_layer; i < RG_REND_Z_ORDERS; i++) {
          objects[i].clear();
        }
      }
      inline void add_rect(vec2<int64_t> pos, vec2<int64_t> size, vec4<float> color, int64_t layer) {
        schedule_rect({pos, size, color}, layer);
      }
      inline void add_text(vec2<int64_t> pos, int64_t id, std::string text, uint64_t layer) {
        schedule_text({pos, text, id}, layer);
      }
      inline void submit() {
        render();
      }
      inline void clear() {
        for (auto& l : objects) l.clear();
      }
      inline int64_t str_glyphxlen(std::string str, int64_t fontid) {
        auto& head = fonts[fontid];
        int64_t ctn = 0;
        for (auto& chr : str) {
          auto data = head.get_glyph(chr);
          if (!data.has_value()) continue;
          ctn += data.value().advance[0] >> 6;
        }
        return ctn;
      }
      inline uint64_t load_fontObj(const char* path, uint64_t fontsize) {
        try {
          fonts.emplace_back(rg::glyph::glyphManager(path, fontsize));
          return fonts.size()-1;
        } catch (std::exception& e) {
          throw e; // cascade
        }
      }
      inline uint64_t load_fontObj(unsigned char* data, unsigned int len, uint64_t fontsize) {
        try {
          fonts.emplace_back(rg::glyph::glyphManager(data, len, fontsize));
          return fonts.size()-1;
        } catch (std::exception& e) {
          throw; // cascade
        }
      }
      inline int64_t get_fontwidth(int64_t id) {
        return fonts[id].get_adv();
      }
      inline int64_t get_fontheight(int64_t id) {
        return fonts[id].get_height();
      }
  };
}
