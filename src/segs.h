#pragma once
#include "rend.h"
#include <algorithm>
#include <cstring>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <cstdint>
#include <array>

#ifndef RG_SEGS_LAYERS
#define RG_SEGS_LAYERS 8
#endif
#define RG_SEGS_LAST_LAYER (RG_SEGS_LAYERS-1)

static_assert(RG_SEGS_LAYERS <= RG_REND_Z_ORDERS, 
    "RG segs cannot have more layers than RG rend! (if you do not pair rg segs with rg rend, please, comment out the error.)"
    ); 

namespace rg {
  namespace segs {
    enum class direction_t {LEFT, RIGHT, UP, DOWN}; // Non composable!!! DO NOT TRY TO | (BITWISE OR) IT
    struct reference {
      uint64_t layer, id;
    };

    class splitter {
      private:
        rend::vec2<int64_t> rg__screen_size = {0, 0};
        struct free_space {
          rend::vec2<int64_t> begin = {0, 0};
          rend::vec2<int64_t> size = {0, 0};
        };


        struct layer_header {
          std::vector<rend::rect> rects;
          free_space space;
          bool invalidate = false;  // if true then not allow drawing/export until first split call or scrap.
        };
        std::array<layer_header, RG_SEGS_LAYERS> layers;
        std::unordered_map<std::string, reference> ids;
        uint64_t gindx = 0;
      public:
        inline void scrap() {
          for (layer_header& header : layers) {
            header.rects.clear();
            header.invalidate = false;
            header.space.begin = {0, 0};
            header.space.size = rg__screen_size;
          }
          ids.clear();
          gindx = 0;
        }
        inline std::optional<reference> get_ref(std::string name) {
          auto e = ids.find(name);
          if (e != ids.end()) {
            return e->second;
          }
          return std::nullopt;
        }
        inline std::optional<rg::rend::rect> get_info(reference ref) {
          if (ref.id < layers[ref.layer].rects.size()) {
            return layers[ref.layer].rects.at(ref.id);
          }
          return std::nullopt;
        } 

        inline int split(direction_t direction, uint64_t layer, double part, rend::vec4<float> color, std::optional<std::string> strid) {
          if (layer >= RG_SEGS_LAYERS) {
            return -1;
          }
          if (part > 1.0 || part <= 0.0) {
            return -2;
          }
          auto& head = layers[layer];
          auto& space = head.space;
          int ret = -4;
          auto independent_action = [&]() {
            if (direction == direction_t::DOWN) {
              // subtract from size (y)
              double mod = static_cast<double>(space.size[1]);
              int32_t res = static_cast<int32_t>(mod * part); // allowed: rounding up, final, smaller loss
              head.rects.push_back({
                  .pos = {space.begin[0], space.begin[1] + space.size[1] - res}, // origin
                  .size = {space.size[0], res},
                  .color = color
                  });
              space.size[1]  -= res;
              ret = 0;
            }
            else if (direction == direction_t::UP) {
              // from both size (y) and origin (y)
              double mod = static_cast<double>(space.size[1]);
              int32_t res = static_cast<int32_t>(mod * part);

              head.rects.push_back({
                  .pos   = {space.begin[0], space.begin[1]},
                  .size  = {space.size[0],  res},
                  .color = color
                  });
              space.begin[1] += res;
              space.size[1]  -= res;
              ret = 0;
            }
            else {
              ret = -4;
            }
          };
          auto left_action = [&]() {
            // -left action
            // affect origin and size.

            double mod = static_cast<double>(space.size[0]);
            int32_t res = static_cast<int32_t>(mod * part);

            head.rects.push_back({
                .pos = {space.begin[0], space.begin[1]},
                .size = {res, space.size[1]},
                .color = color
                });

            space.begin[0] += res;
            space.size[0] -= res;
            ret = 0;
          };
          auto right_action = [&]() {
            // -right action
            // affect size.
            double mod = static_cast<double>(space.size[0]);
            int32_t res = static_cast<int32_t>(mod * part);

            head.rects.push_back({
                .pos = {space.begin[0] + space.size[0] - res, space.begin[1]},
                .size = {res, space.size[1]},
                .color = color
                });

            space.size[0] -= res;
            ret = 0;
          };

          switch (direction) {
            case (direction_t::DOWN):
            case (direction_t::UP):
              independent_action();
              break;
            case (direction_t::LEFT):
              left_action();
              break;
            case (direction_t::RIGHT):
              right_action();
              break;
            default:
              return -3;
          }
          if (strid.has_value()) {
            ids.emplace(strid.value(), reference{layer, head.rects.size()-1});
          }
          return ret;
        }

        inline void screensize(rend::vec2<int64_t> size) { //n1
          if (size[0] != rg__screen_size[0] || size[1] != rg__screen_size[1]) { //n2
            rg__screen_size = size;
            for (layer_header& head : layers) { //n3
              head.invalidate = true;
              head.space.size = { //n4
                size[0],
                size[1]
              };//n4e
              head.space.begin = {//n4
                0, 
                0
              }; //n4e
            } //n3e
          } //n2e
        } //n1e

        // callee owns the vector
        std::optional<std::vector<rend::rect>> export_rects(int64_t layer) {
          if (layer >= RG_SEGS_LAYERS) {
            return std::nullopt;
          }
          std::vector<rend::rect> vec;
          for (auto& obj : layers[layer].rects) {
            vec.push_back(obj);
          }
          return vec;
        }

        void rend_add(rend::rend_obj& render, int64_t layer) {
          if (layer >= RG_SEGS_LAYERS) {
            return;
          }

          for (auto& obj : layers[layer].rects) {
            render.add_rect(obj.pos, obj.size, obj.color, layer);
          }
        }
    };
  }
}
