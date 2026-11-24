#pragma once

#include "rend.h"
#include "vecx.h"
#include <vector>

namespace rg::editor {
  enum class kind_t { None, Insert, Delete };
  struct edit_t {                                                                                                                                                                                            
    std::size_t index;
    kind_t kind;
    std::string text;  
  };
  struct buffer_t {
    std::string original;
    uint64_t clump_start = 0;
    bool clumping = false;
    std::vector<edit_t> edits;
  };
  class instance {
    private:
      rg::rend::rend_obj& rend;
      rg::rend::rect parent;
      rg::rend::vec4<float> bgc = {0.0f, 0.28627452f, 0.32549021f, 1.0f};
      std::vector<buffer_t> buffers;
      uint64_t active = 0;
      uint64_t index = 0;
    public:
      enum class action {left, right};
      instance(rg::rend::rend_obj& r) : rend(r) {buffers.push_back({"", {}});}
      void set_parent(rg::rend::rect r) {parent = r;}
      void set_rend(rg::rend::rend_obj& r) {rend = r;}
      int64_t new_buffer(std::string start) {buffers.push_back({start, {}}); return buffers.size()-1;}
      void set_current(int64_t id) {active = id;}
      inline void insert(std::string inserted, bool clump) {
        insert_at(index, inserted, clump);
      }
      void insert_at(uint64_t where, std::string inserted, bool clump) {
        index = where;
        auto& ac = buffers[active]; 

        if (clump && !ac.edits.empty()) {
          auto& last = ac.edits.back();
          if (last.kind == kind_t::Insert && last.index + last.text.size() == where) {
            last.text += inserted;
            index = last.index + last.text.size();
            return;
          }
        }

        ac.edits.push_back({where, kind_t::Insert, inserted});
        index = where + inserted.size();
      }      
      void remove(uint64_t where, std::string range) {
        index = where;
        buffers[active].edits.push_back({where, kind_t::Insert, range});

      }
  };
}
