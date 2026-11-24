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
#include "editor.h"
#define WINDOW_HEIGHT 400
#define WINDOW_WIDTH 600

constexpr std::array<uint16_t, 3> ver = {0, 0, 6};

bool needs_update = false;
int add_char = 0;


const char* SOURCE_vertex_ = 
"#version 330 core\n"
"layout (location = 0) in vec2 posData;\n"
"layout (location = 1) in vec4 Coldata;\n"
"out vec4 col;\n"
"void main() {\n"
"gl_Position=vec4(posData, 0.0, 1.0);\n"
"col = Coldata;\n"
"}";
const char* SOURCE_fragment_ = 
"#version 330 core\n"
"in vec4 col;\n"
"out vec4 FragColor;\n"
"void main() {\n"
"FragColor = col;\n"
"}";

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
  // std::cerr << "[OK!]\nboot up stage 4... [compile shaders] "; 


  std::cerr << "[OK!]\n";
  std::cerr << "[reglyph]> Basic boot done! from this moment no boot stages will be displayed\n";

  rg::rend::rend_obj render; // init renderer object
  rg::segs::splitter split;

  // set empty screen color
  glClearColor(0.1059, 0.1059, 0.1059, 1.0);

  int x, y; // state variables

  glfwGetWindowSize(window, &x, &y); // init
  struct bar_data {
    bool active;
    double part;
    rg::segs::direction_t direction;
    rg::rend::vec4<float> color;
    std::optional<std::string> label = std::nullopt;
  }; // for segment rendering (yea, the name is bar but its for segments)
  std::vector<bar_data> ldata; // layout data

  // std settings
  ldata.push_back({true, 0.025, rg::segs::direction_t::UP,    {0.2196f, 0.2824f, 0.2824f, 1.0f}, "options"}); // options bar #384848
  ldata.push_back({true, 0.05,  rg::segs::direction_t::UP,    {0.2196f, 0.2824f, 0.2824f, 1.0f}, "topbar"});  // top bar     #384848
  ldata.push_back({true, 0.2,   rg::segs::direction_t::LEFT,  {0.1647f, 0.2118f, 0.2118f, 1.0f}, "sidebar"}); // sidebar     #2a3636
  ldata.push_back({true, 0.95,  rg::segs::direction_t::UP,    {0.1020f, 0.1373f, 0.1373f, 1.0f}, "editor"});  // mainseg     #1a2323
  ldata.push_back({true, 1,     rg::segs::direction_t::RIGHT, {0.2196f, 0.2824f, 0.2824f, 1.0f}, "statbar"}); // statbar     #384848, any direction, either way consumes everything left.


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
  glfwSetCharCallback(window, [](GLFWwindow* window, unsigned int codepoint) -> void { 
    needs_update = true;
    add_char = codepoint; 
  });
  
  glfwSwapBuffers(window);
  std::string e;
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    if (layout_dirty) {
      glfwGetWindowSize(window, &x, &y);
      render.set_screensize({x, y});
      render.clear();
      split.screensize({x, y});
      split.scrap();
      for (auto& segment : ldata) {
        if (segment.active)
          split.split(segment.direction, 0, segment.part, segment.color, segment.label);
      }
      split.rend_add(render, 0);

      layout_dirty = false;
    }
    if (needs_update) {
      if (add_char) {
        e += add_char;
        add_char = 0;
      }
      render.clear_from(1);
      auto editor = split.get_info(split.get_ref("editor").value());
      render.add_text({editor->pos[0] + (editor->pos[0] * 15) / 10, editor->pos[1] + (editor->pos[1] * 15) / 10}, 0, e, 1);
      needs_update = false;
    }
    glClear(GL_COLOR_BUFFER_BIT);
    render.submit();
    glfwSwapBuffers(window);
  }
  std::cerr << "[reglyph]! Bye!\n" << std::flush;
  glfwDestroyWindow(window);
  return 0;
}
