#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <stdio.h>
#include "graph.h"

constexpr std::array<uint16_t, 3> ver = {0, 0, 1};

// todo:
// 1. Make arg parsing
// desc:
// - debug (-d)
// - version (-v)
// - help (-h)
// - kill on write (-k) exit after write
// - read only (-r) not allow any edits
// 2. init graphics
// desc:
// 3. file tree
// desc:
// 4. text viewer/loader
// desc:
// 5. actual editing
// desc:
// 6. file streaming (optional)
// desc:


enum class arg_bit: uint16_t {
  help, version, kill_after_write, readonly, debug 
};

constexpr auto getbit = [](arg_bit n) -> uint16_t {
  return (static_cast<uint16_t>(1) << static_cast<uint16_t>(n));
};

struct args_proc {
  std::vector<std::string> files;
  uint16_t bitmask = 0; 
};

args_proc bitmask_argp(std::vector<std::string> args) {
  args_proc ret;
  std::unordered_map<std::string, arg_bit> args_vocab = {
    {"kill-after-write", arg_bit::kill_after_write},
    {"help", arg_bit::help},
    {"version", arg_bit::version},
    {"read-only", arg_bit::readonly},
    {"debug", arg_bit::debug}
  };


  for (uint64_t i = 0; i < args.size(); i++) {
    if (args[i].starts_with("--")) {
      // long
      auto e = args_vocab.find(args[i].substr(2));
      if (e!=args_vocab.end()) {
        // found it
        ret.bitmask |= getbit(e->second);
      } else {
        std::cerr << "[reglyph]> Unknown flag! \"" << args[i] << "\"\n[reglyph]! aborting!\n";
        std::abort();
      }
      continue;
    }
    // short flag or filename
    if (!args[i].empty() && args[i][0] == '-') {
      // short flag
      for (uint64_t j = 1; j < args[i].size(); j++) {
        switch (args[i][j]) {
          case 'd':
            ret.bitmask |= getbit(arg_bit::debug);
            break;
          case 'h':
            ret.bitmask |= getbit(arg_bit::help);
            break;
          case 'v':
            ret.bitmask |= getbit(arg_bit::version);
            break;
          case 'r':
            ret.bitmask |= getbit(arg_bit::readonly);
            break;
          case 'k':
            ret.bitmask |= getbit(arg_bit::kill_after_write);
            break;
          default:
            std::cerr << "[reglyph]> Unknown flag! \"-" << args[i][j] << "\"";
            std::cerr << "[reglyph]! aborting!\n";
            std::abort();
            //do not try to recover from what is originally user's intent
        }
      }
      continue;
    }
    // file
    ret.files.push_back(args[i]);
  }
  return ret;
}

int main(int argc, char** argv) {
  std::cerr << "[reglyph]! booting up reglyph... [ver " << ver[0] << "." << ver[1] << "." << ver[2] << "]\n[reglyph]> Hello, reglyph!\n";
  std::cerr << "[reglyph]! [MIT licensed!]\n[reglyph]! find out more about project: [github] static_noise/reglyph!" << std::endl;

  std::cerr << "[reglyph]! boot up stage 1... [creating arg vector!]" << std::flush;
  std::vector<std::string> args;
  std::string exec_name=std::string(argv[0]);
  args.reserve(argc);
  for (uint64_t i = 1; i < argc; i++) {
    args.push_back(std::string(argv[i]));
  }
  std::cerr << " [OK!]" << std::endl;
  auto data_args = bitmask_argp(args);
  if (data_args.bitmask & getbit(arg_bit::debug)) {
    for (uint64_t i = 0; i < data_args.files.size(); i++) {
      std::cerr << "[reglyph]! file passed: \"" << data_args.files[i] << "\"!\n";
    }
  }
  std::cerr << "[reglyph]! boot up stage 2... [creating window!]\n" << std::flush;
  rg::graph::window_config cfg = {
    .height = 400,

    .width = 600,
    .handle = "reglyph",

    .hints = {
      .resizable = true,
      .decorated = true,
      .transparent = false,
      .floating = true,
      .maximized = false,
      .samples = 2,
      .refresh_rate = GLFW_DONT_CARE,
    }
  };
  if (!glfwInit()) {
    std::cerr << "[reglyph]! GLFW initialization failed!\n";
    return 1;
  }
  GLFWwindow* window = rg::graph::create_window(cfg);
  glfwMakeContextCurrent(window);

  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    glClear(GL_COLOR_BUFFER_BIT);

    glfwSwapBuffers(window);
  }
  glfwDestroyWindow(window);
  return 0;
}
