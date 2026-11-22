#pragma once
// pretty ironic, glyph.h, while the app is called reglyph
// always monospace, breaks on any non-monospace 
#include "freetype/fttypes.h"
#include <cstring>
#include <exception>
#include <ft2build.h>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <unordered_map>
#include "rend.h"
#include FT_FREETYPE_H


FT_Library ft;



namespace rg::glyph {
  struct Character {
    rg::rend::vec2<int> atlas_pos, size, bearings;
  };
  int ascii_start = 32;
  int ascii_end = 126;
  int ascii_count = ascii_end - ascii_start + 1;
  class glyphManager {
    private:
      std::unordered_map<int, Character> Translate;
      std::vector<uint8_t> font_data;
      std::vector<uint8_t> atlas;
      void create_atlas(int64_t start_inclusive, int64_t end_inclusive) {
        FT_Face face;
        uint64_t max_width = 0, max_height = 0;
        float max_advance = 0;
        FT_Set_Pixel_Sizes(face, 0, 32);

        for (int i = start_inclusive; i <= end_inclusive; i++) {
          if (FT_Load_Char(face, i, FT_LOAD_RENDER))
            continue;
          FT_GlyphSlot glyph = face->glyph; // reglyph mention
          max_width = std::max(max_width, (uint64_t)glyph->bitmap.width);
          max_height = std::max(max_height, (uint64_t)glyph->bitmap.rows);
          max_advance = std::max(max_advance, glyph->advance.x / 64.0f);
          if (glyph->bitmap.pitch < 0) throw std::runtime_error("glyph unsuported pitch");
        }
        atlas.resize((max_height * (uint64_t) ((double) max_width * 1.2f) * ((end_inclusive - start_inclusive + 1 + 3) * 12) / 10)); // extra reserved space, 
                                                                                                                                 // account for rounding err;
                                                                            // make atlas and add 20% for padding, padding = max_width * 1.1;
                                                                            // or padding = (max_width * 11) / 10
        
        
        for (int i = start_inclusive; i <= end_inclusive; i++) {
          FT_Load_Char(face, i, FT_LOAD_RENDER);
          int glyphslot = i - start_inclusive;
          uint64_t glyph_start = max_width
          FT_GlyphSlot glyph = face->glyph;
          
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
      }
  };
}
