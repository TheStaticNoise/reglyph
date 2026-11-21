#pragma once
// pretty ironic, glyph.h, while the app is called reglyph

#include "freetype/fttypes.h"
#include <cstring>
#include <exception>
#include <ft2build.h>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include FT_FREETYPE_H



namespace rg::glyph {
  struct atlas_pos {
    FT_Int 
  }
  class glyphManager {
    private:
      struct {
        const uint64_t SizeCutoff = 1000;
        const uint64_t LayerCutoff = 10;
      } glyph_confdata;

      struct fontdata {
        char* data = nullptr;
        int64_t size = 0;
        FT_Face face;
        bool completed = false;
      };
      std::vector<fontdata> fonts;
      FT_Library lib = nullptr;

    public:
      glyphManager() {
        auto error = FT_Init_FreeType(&lib);
        if (error) {
          throw std::runtime_error("failed FT lib instance init");
        }
        // for now done
      }
      int64_t regFont(std::string& path, uint64_t size, uint64_t layer) {
        if (size > glyph_confdata.SizeCutoff || layer >= glyph_confdata.LayerCutoff) {
          return -1l;
        }
        fontdata data; 
        auto err = FT_New_Face(lib, path.c_str(), layer, &data.face);

        if (err) {
          return -9l-err; // to extract -(res + 9)
        }
        fonts.emplace_back(data);
        return fonts.size()-1;
      }
      int8_t load_data(int64_t id, char* data, int64_t size) {
        if (id < 0 || id >= fonts.size() || fonts[id].completed || size <= 0 || !data) {
          return -1;
        }
        try {
          fontdata& font = fonts.at(id);
          font.data = (char*) new char[size];
          font.size = size;
          std::memcpy(font.data, data, size);
          font.completed = true;
          return 0;
        } catch (std::exception& e) {
          return -2;
        }
      }
  };
}
