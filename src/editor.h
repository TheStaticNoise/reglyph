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
    bool snapshot_valid = true;
    std::string snapshot;
    std::vector<edit_t> edits;
  };
  class Instance {
    private:
      rg::rend::rend_obj& rend;
      rg::rend::rect parent;
      rg::rend::vec4<float> bgc = {0.0f, 0.28627452f, 0.32549021f, 1.0f};
      std::vector<buffer_t> buffers;
      uint64_t active = 0;
      uint64_t index = 0;
      uint64_t rdindx = 0;
      inline void gen_snapshot() {
        auto& ac = buffers[active];
        ac.snapshot = ac.original;
        for (auto& edit : ac.edits) {
          size_t at = std::min<size_t>(edit.index, ac.snapshot.size());
          if (edit.kind == kind_t::Insert)
            ac.snapshot.insert(at, edit.text);
          else if (edit.kind == kind_t::Delete)
            ac.snapshot.erase(at, edit.text.size());   // erase clamps the count, so it can't throw
        }
      }    public:
      enum class action {left, right};
      Instance(rg::rend::rend_obj& r) : rend(r) {buffers.push_back({"", {}});}
      void set_parent(rg::rend::rect r) {parent = r;}
      void set_rend(rg::rend::rend_obj& r) {rend = r;}
      int64_t new_buffer(std::string start) {buffers.push_back({start, {}}); return buffers.size()-1;}
      void set_current(int64_t id) {active = id;}
      inline void insert(std::string inserted, bool clump) {
        insert_at(index, inserted, clump);
      }
      inline void remove(std::string deleted, bool clump) {
        remove_at(index, deleted, clump);
      }
      void insert_at(uint64_t where, std::string inserted, bool clump) {
        index = where;
        auto& ac = buffers[active]; 
        ac.snapshot_valid = false;


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
      void remove_at(uint64_t where, std::string range, bool clump) {
        index = where;
        auto& ac = buffers[active]; 
        ac.snapshot_valid = false;

        if (clump && !ac.edits.empty()) {
          auto& last = ac.edits.back();
          if (last.kind == kind_t::Insert && last.index + last.text.size() == where) {
            last.text += range;
            index = last.index + last.text.size();
            return;
          }
        }

        ac.edits.push_back({where, kind_t::Delete, range});
        index = where - range.size();      
      }
      void return_carriage() {rdindx = 0; gen_snapshot();}

      std::string readline(bool& indxline, int64_t& indx_offset) {
        gen_snapshot();
        indxline = false;
        auto &ac = buffers[active];
        int64_t st = rdindx;
        for (; rdindx < ac.snapshot.size() && ac.snapshot[rdindx] != '\n'; rdindx++);
        rdindx++;
        if (index >= st && index < rdindx) {
          indxline = true;
          indx_offset = index - st;
        }
        return ac.snapshot.substr(st, rdindx - st);
      }
      void backspace() {
        gen_snapshot();
        auto& ac = buffers[active];
        if (index > ac.snapshot.size()) index = ac.snapshot.size();
        if (index == 0) return;
        ac.snapshot_valid = false;

        if (!ac.edits.empty()) {
          auto& last = ac.edits.back();
          if (last.kind == kind_t::Insert && last.index + last.text.size() == index) {
            last.text.pop_back();
            index--;
            if (last.text.empty()) ac.edits.pop_back();
            return;
          }
        }
        ac.edits.push_back({index - 1, kind_t::Delete, std::string(1, ac.snapshot[index - 1])});
        index--;
      }
      inline void move_ptr(int64_t off) {
        gen_snapshot();
        int temp = index;
        auto& ac = buffers[active];
        if (temp + off >= 0 && temp + off <= ac.snapshot.size()) {
          index += off;
        }
      }
      inline int64_t get_index() {return index;}
      inline bool is_eof() { return rdindx >= buffers[active].snapshot.size(); }
  };
}
