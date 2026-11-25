#include <filesystem>
#include <optional>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <array>
#include "graph.h"
#include <glad/gl.h>

#include "rend.h"
#include "segs.h"
#include "glyph.h"
#include "vecx.h"
#include <freetype2/ft2build.h>
#include "uiUtilz.h"
#include "editor.h"
#define WINDOW_HEIGHT 400
#define WINDOW_WIDTH 600

constexpr std::array<uint16_t, 3> ver = {0, 0, 6};

std::string pending_input;
bool deletechar = true;
bool needs_update = false;

bool layout_dirty = true;

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

bool backspace = false;

int main(int argc, char** argv) {
  auto abort = []() {
    std::cerr << "[reglyph]> Aborting...\n";
    std::abort();
  };
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
  if (data_args.bitmask & getbit(arg_bit::help)) {
    std::cerr 
      << "[reglyph]! Help: \n" 
      << "[reglyph]> -d/--debug: debug\n"
      << "[reglyph]> -h/--help: this message\n"
      << "[reglyph]> -v/--version: version\n"
      << "[reglyph]> -k/--kill-after-write: exit after writing to a file (todo: implement)\n";
    return 0;
  } 
  if (data_args.bitmask & getbit(arg_bit::version)) {
    std::cerr << "[reglyph]> [ver " << ver[0] << "." << ver[1] << "." << ver[2] << "]\n";
    return 0;
  }
  std::cerr << "[reglyph]! boot up stage 2... [creating window!]" << std::flush;

  rg::graph::window_config cfg = {
    .height = WINDOW_HEIGHT,
    .width = WINDOW_WIDTH,
    .handle = "reglyph",

    .hints = {
      .resizable = true,
      .decorated = true,
      .transparent = true,
      .floating = true,
      .maximized = false,
      .samples = 2,
      .refresh_rate = GLFW_DONT_CARE,
    }
  };

  if (!glfwInit()) {
    std::cerr << " [FAIL]\n[reglyph]! GLFW initialization failed!\n";
    abort();
  }
  std::cerr << " [OK!]\n";
  GLFWwindow* window = rg::graph::create_window(cfg);
  glfwMakeContextCurrent(window);
  rg::graph::set_limit(window, WINDOW_WIDTH, WINDOW_HEIGHT, GLFW_DONT_CARE, GLFW_DONT_CARE);

  std::cerr << "[reglyph]! boot up stage 3... [init GLAD!] ";

  if (!gladLoadGL(glfwGetProcAddress)) {
    // failed
    std::cerr << "[FAIL]\n[reglyph]! GLAD init failed!\n";
    abort();
  }


  std::cerr << "[OK!]\n";
  std::cerr << "[reglyph]> Basic boot done! from this moment no boot stages will be displayed\n";

  rg::rend::rend_obj render; // init renderer object
  rg::segs::splitter split;

  // set empty screen color
  glClearColor(0.1059, 0.1059, 0.1059, 1.0);

  int x, y; // state variables

  glfwGetWindowSize(window, &x, &y); // init

  // preemptive Viewport size setting to fix a bug when gl viewport is smaller than window size until resize
  glViewport(0, 0, x, y);

  // resize callback
  glfwSetFramebufferSizeCallback(window, [](GLFWwindow* win, int width, int height) {
      layout_dirty = true;
      needs_update = true;
      glViewport(0, 0, width, height);
      });
  try {
    render.load_fontObj("fonts/JetBrainsMono-SemiBold.ttf", 16);
  } catch (std::exception& e){
    std::cerr << "font load err!" << e.what() << "\n";
    abort();
  }

  rg::uiUtilz::Instance manager(render, (int64_t) 0, (int64_t) 0, rg::rend::vec2<int64_t>(x, y));
  // std settings
  manager.add_element(0, {true, 0.025, rg::segs::direction_t::UP,    {0.2196f, 0.2824f, 0.2824f, 1.0f}, "options"}); // options bar #384848
  manager.add_element(0, {true, 0.05,  rg::segs::direction_t::UP,    {0.2196f, 0.2824f, 0.2824f, 1.0f}, "topbar"});  // top bar     #384848
  manager.add_element(0, {true, 0.2,   rg::segs::direction_t::LEFT,  {0.1647f, 0.2118f, 0.2118f, 1.0f}, "sidebar"}); // sidebar     #2a3636
  manager.add_element(0, {true, 0.05, rg::segs::direction_t::UP,     {0.1722f, 0.2137f, 0.2020f, 1.0f}, "tabs"}); // editor (whatever color it is, idk)
  manager.add_element(0, {true, 0.95,  rg::segs::direction_t::UP,    {0.1020f, 0.1373f, 0.1373f, 1.0f}, "editor"});  // mainseg     #1a2323
  manager.add_element(0, {true, 1,     rg::segs::direction_t::RIGHT, {0.2196f, 0.2824f, 0.2824f, 1.0f}, "statbar"}); // statbar     #384848, any direction, either way consumes everything left.

  glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int w, int h) {
      layout_dirty = true;
      glViewport(0, 0, w, h);
      });
  glfwSetCharCallback(window, [](GLFWwindow*, unsigned int cp) {
      if (cp >= 32 && cp < 127) { pending_input.push_back((char)cp); layout_dirty = true; }
      });
  // delete: bool deletechar = true;

glfwSetKeyCallback(window, [](GLFWwindow*, int key, int, int action, int) {
  if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
  if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) {
    pending_input.push_back('\n');
    layout_dirty = true;
  } else if (key == GLFW_KEY_BACKSPACE) {
    pending_input.push_back('\b');
    layout_dirty = true;
  }
});

  manager.set_editorlabel("editor");
  manager.editor.insert("hello, ", true);   // optional, delete if you don't want the starter text
                                            // delete: std::cout << manager.editor.readline();

  while (!glfwWindowShouldClose(window)) {
if (!pending_input.empty()) {
  for (char c : pending_input) {
    if (c == '\b') manager.editor.backspace();
    else           manager.editor.insert(std::string(1, c), true);
  }
  pending_input.clear();
}
    if (layout_dirty) {
      glfwGetWindowSize(window, &x, &y);
      manager.set_screensize({x, y});
      glClear(GL_COLOR_BUFFER_BIT);
      manager.publish();
      glfwSwapBuffers(window);
      layout_dirty = false;
    }
    glfwWaitEvents();
  }
  std::cerr << "[reglyph]! Bye!\n" << std::flush;
  glfwDestroyWindow(window);
  return 0;
}
