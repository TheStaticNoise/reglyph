#pragma once
#include "vecx.h"
// pretty ironic, glyph.h, while the app is called reglyph
// always monospace, breaks on any non-monospace 
#include "freetype/fttypes.h"
#include <cstdio>
#include <cstring>
#include <exception>
#include <ft2build.h>
#include <cstdint>
#include <optional>
#include <sys/types.h>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <unordered_map>
#include "rend.h"
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include FT_FREETYPE_H


FT_Library ft;



namespace rg::glyph {
  struct Character {
    rg::rend::vec2<int64_t> atlas_pos, size, advance;
  };
  int ascii_start = 32;
  int ascii_end = 126;
  int ascii_count = ascii_end - ascii_start + 1;

  class glyphManager {
    private:
      std::unordered_map<int, Character> charmap;
      std::vector<uint8_t> font_data;
      int64_t adv = 0;
      std::vector<uint8_t> atlas;
      int64_t atlas_width = 0, atlas_height = 0, ascent = 0, descent = 0;
      GLuint atlas_gl;
      void copy_glyph(std::vector<uint8_t>& current_atlas, unsigned char* bitmap, uint64_t atlas_width, uint64_t atlas_xbase, uint64_t glyph_x, uint64_t glyph_y, FT_Int btop) { 
        // assumes that glyph fits in atlas, glyph_x and glyph_y must be bitmap size
        auto address = [&current_atlas, atlas_width, atlas_xbase](ssize_t x, ssize_t y) -> uint8_t* {
          return &current_atlas.at(y * atlas_width + atlas_xbase + x);
        };
        auto bitmap_address = [&bitmap, glyph_x](ssize_t x, ssize_t y) -> unsigned char* {
          return &bitmap[y * glyph_x + x];
        };
        uint64_t y0 = ascent - btop;
        for (uint64_t r = 0; r < glyph_y; r++)
          for (uint64_t i = 0; i < glyph_x; i++)
            *address(i, y0 + r) = bitmap[r * glyph_x + i];
      }
      void create_atlas(FT_Face face, int64_t start_inclusive, int64_t end_inclusive, int64_t fontsize) {
        uint64_t max_width = 0, max_height = 0;
        FT_Set_Pixel_Sizes(face, 0, fontsize);

        for (int i = start_inclusive; i <= end_inclusive; i++) {
          if (FT_Load_Char(face, i, FT_LOAD_RENDER))
            continue;
          FT_GlyphSlot glyph = face->glyph; // reglyph mention
          max_width = std::max(max_width, (uint64_t)glyph->bitmap.width);
          adv = std::max(adv, (int64_t)(glyph->advance.x >> 6));
          ascent  = std::max(ascent,  (int64_t)glyph->bitmap_top);
          descent = std::max(descent, (int64_t)glyph->bitmap.rows - glyph->bitmap_top);
          if (glyph->bitmap.pitch < 0) throw std::runtime_error("glyph unsuported pitch");
        }
        uint64_t gw = (max_width*12)/10;
        uint64_t gc = ((end_inclusive - start_inclusive + 4) * 12) / 10;
        atlas_width = gw*gc;

        atlas.assign((ascent + descent) * gw * gc, 0); // extra reserved space, to account for rounding err; zero initialized;
                                                       // makes atlas and adds 20% for padding, padding = (max_width * 11) / 10;
        atlas_height = ascent+descent;
        atlas_width = gw * gc;
        for (int i = start_inclusive; i <= end_inclusive; i++) {
          if (FT_Load_Char(face, i, FT_LOAD_RENDER)) throw std::runtime_error("ft load error");
          int glyphslot = i - start_inclusive;
          uint64_t glyph_start = ((max_width * 12) / 10) * glyphslot; // get the start
          FT_GlyphSlot glyph = face->glyph;

          copy_glyph(atlas, glyph->bitmap.buffer, gw*gc, glyph_start, glyph->bitmap.width, glyph->bitmap.rows, glyph->bitmap_top);
          //    rg::rend::vec2<int> atlas_pos, size, bearings;
          charmap.emplace(i, Character{{glyph_start, 0}, {glyph->bitmap.width, ascent + descent}, {glyph->advance.x, 0}});// bearings are not populated;

        }
      }
    public:
      // small hack: runtime errors are: [err component] [spec], so you can createa string from the error and use starts_with()
      glyphManager(std::string path, int fontsize) { 
        if (FT_Init_FreeType(&ft)) throw std::runtime_error("ft init err");
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) throw std::runtime_error("font cannot open");
        auto size = file.tellg();
        if (size <= 0) throw std::runtime_error("font size invalid");
        font_data.resize(size);
        file.seekg(0);
        if (!file.read(reinterpret_cast<char*>(font_data.data()), size)) throw std::runtime_error("file read err");
        FT_Face face;
        if (FT_New_Face(ft, path.c_str(), 0, &face)) throw std::runtime_error("font load err");
        if (FT_New_Memory_Face(ft, font_data.data(), size, 0, &face)) throw std::runtime_error("face load err");
        file.seekg(0);
        // even C is easier to write
        // in mean of reading a file
        // loaded the font file into our shitty class
        try {
          create_atlas(face, ascii_start, ascii_end, fontsize);
        } catch (std::exception& e) {
          throw e; // cascade exception
        }
        glGenTextures(1, &atlas_gl);
        glBindTexture(GL_TEXTURE_2D, atlas_gl);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_R8,
            atlas_width,
            atlas_height,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            atlas.data()
            );
      }
      glyphManager(unsigned char* data, unsigned int data_size, int64_t fontsize) {
        if (FT_Init_FreeType(&ft)) throw std::runtime_error("ft init err");
        FT_Face face;
        if (FT_New_Memory_Face(ft,
              data,
              (FT_Long)data_size,
              0, &face)) {
          throw std::runtime_error("font bad ttf");
        }
        try {
          create_atlas(face, ascii_start, ascii_end, fontsize);
        } catch (std::exception& e) {
          throw e; // cascade exception
        }
        glGenTextures(1, &atlas_gl);
        glBindTexture(GL_TEXTURE_2D, atlas_gl);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_R8,
            atlas_width,
            atlas_height,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            atlas.data()
            );
      }
      std::optional<Character> get_glyph(int code) {
        auto e = charmap.find(code);

        if (e == charmap.end())
          return charmap.find((int)'?')->second;

        return e->second;
      } 
      GLuint get_texture() {
        return atlas_gl;
      }
      inline rg::rend::vec2<int64_t> get_atlas_size() {
        return {atlas_width, atlas_height};
      }
      inline int64_t get_adv() {
        return adv;
      }
      inline int64_t get_height() {
        return ascent + descent;
      }
  };
}
