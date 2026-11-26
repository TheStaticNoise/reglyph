#pragma once

// ui tools, such as :
/* vertical lists
 * horizontal lists
 * buttons
 * ui rend abstractions
 * editor managing
 */
#include "vecx.h"
#include "freetype/fttypes.h"
#include <GLFW/glfw3.h>
#include <cstring>
#include <exception>
#include <ft2build.h>
#include <cstdint>
#include <optional>
#include <string>
#include <sys/types.h>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <unordered_map>
#include <format>
#include "rend.h"
#include "editor.h"
#include "segs.h"
#include FT_FREETYPE_H

namespace rg::uiUtilz {
  struct element {
    bool active = false;
    double part = 0.0f;
    rg::segs::direction_t direction = rg::segs::direction_t::UP;
    rg::rend::vec4<float> color = {1.0f, 0.0f, 0.0f, 1.0f};
    std::optional<std::string> label = std::nullopt;
  };
  enum class widget_t {none, horizontal_list, vertical_list, text, button, vertical_splist, horizontal_splist}; // sp: sepparated, pressable
  struct wlistd {
    std::string name;
    int64_t signald = 0;
  };
  struct widget {
    bool active = false;
    std::string hostId;
    int64_t fontid;
    widget_t kind = widget_t::none;
    float part_x = 0.0f, part_y = 0.0f;

    bool pressable = false;
    std::vector<wlistd> elements;
  };
  class Instance {
    private:
      bool refs_valid = false;
      rg::rend::rend_obj& Render;
      rg::segs::splitter split;
      std::string editorlb;
      int64_t editor_fontid = -1;
      int64_t misc_font = -1;
      bool editor_lines = true;
      int64_t editor_layer = 2;
      double lastblink = 0.0;
      bool blink = false;
      double last_render = 0.0;
      struct header {
        std::vector<element> elements;
        std::vector<widget> widgets;
      };
      std::array<header, RG_SEGS_LAYERS> elements;
    public:
      rg::editor::Instance editor;
      Instance(rg::rend::rend_obj& rend, int64_t editorfont, int64_t miscfont, rg::rend::vec2<int64_t> screensize) : Render(rend), editor(rend), editor_fontid(editorfont), misc_font(miscfont) {
        Render.set_screensize(screensize);
        split.screensize(screensize);
        lastblink = glfwGetTime();
      }
      void scrap() {for (auto& vector : elements) {vector.elements.clear();vector.widgets.clear();}}
      void add_element(int64_t layer, element et) {if (layer < RG_SEGS_LAYERS) {elements[layer].elements.push_back(et);}}
      void add_widget(int64_t layer, widget wt) {if (layer < RG_SEGS_LAYERS) {elements[layer].widgets.push_back(wt);}}
      void set_editorlabel(std::string editor_label) {editorlb = editor_label;}
      void set_screensize(rg::rend::vec2<int64_t> screensize) {
        Render.set_screensize(screensize);
        split.screensize(screensize);
      }
      void set_editorlines(bool val) {editor_lines = val;}
      // no wordwrap for now
      void update_editor() {
        auto ref = split.get_ref(editorlb);
        if (!ref.has_value()) {
          std::cerr << "[reglyph]! no segment for editor label \"" << editorlb << "\"\n";
          return;
        }
        auto parent = split.get_info(ref.value());

        const int64_t line_h = Render.get_fontheight(editor_fontid);
        const int64_t fw     = Render.get_fontwidth(editor_fontid);
        const int64_t gutter = parent->size[0] / 15;                         // idk how much % of width
        const int64_t bottom = parent->pos[1] + (parent->size[1] * 39) / 40; // 97.5%
        int64_t y = parent->pos[1] + parent->size[1] / 40;                   // 2.5%
        int64_t line_no = 0;

        editor.return_carriage();

        while (!editor.is_eof() && y + line_h <= bottom) {
          bool is_indx = false;
          int64_t collumn = 0;
          std::string text  = editor.readline(is_indx, collumn);
          if (!text.empty() && text.back() == '\n') text.pop_back();
          std::string label = "[" + std::to_string(line_no) + "]";
          Render.add_text(
              {parent->pos[0] + gutter - (int64_t)(label.size() + 1) * fw, y},
              editor_fontid, label, editor_layer);
          if (is_indx && blink) {
            std::string sub = text.substr(0, collumn);
            auto len = Render.str_glyphxlen(sub, editor_fontid);
            Render.add_rect({parent->pos[0] + gutter + len, y}, {2, line_h}, 
                {1.0f, 1.0f, 1.0f, 1.0f}, editor_layer+1);
          }
          Render.add_text({parent->pos[0] + gutter, y}, editor_fontid, text, editor_layer);
          y += line_h;
          line_no++;
        }
        std::string label = "[" + std::to_string(line_no) + "]";
        Render.add_text(
            {parent->pos[0] + gutter - (int64_t)(label.size() + 1) * fw, y},
            editor_fontid, label, editor_layer);
        y += line_h;
      }

      void publish() {
        double now = glfwGetTime();
        if (now >= lastblink + 0.5) {
          blink = !blink;
          lastblink = glfwGetTime();
        }
        Render.clear();
        split.scrap();
        uint64_t i = 0;
        for (auto& header : elements) {
          for (auto& element : header.elements)
            if (element.active)
              split.split(element.direction, i, element.part, element.color, element.label);
          for (auto& widget : header.widgets) { /* nothing yet */ }
          split.rend_add(Render, i);
          i++;
        }
        update_editor();  
        Render.submit();
      }  
      bool drawtest(double interval) {
        double now = glfwGetTime();
        if (now >= last_render + interval) {
          last_render = now;
          return true;
        }
        return false;
      }
      void set_blink() {
        lastblink = glfwGetTime();
        blink = true;
      }
  };
}

