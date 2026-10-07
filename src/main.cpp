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
#include <freetype2/ft2build.h>

#define WINDOW_HEIGHT 400
#define WINDOW_WIDTH 600

constexpr std::array<uint16_t, 3> ver = {0, 0, 4};



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

bool layout_dirty = false;

struct shader_pack {
  GLuint id = 0;
  GLint status = 0;
};

shader_pack create_shader(const char* src, GLenum type) {
  shader_pack pack{};
  pack.id = glCreateShader(type);
  glShaderSource(pack.id, 1, &src, NULL);
  glCompileShader(pack.id);
  glGetShaderiv(pack.id, GL_COMPILE_STATUS, &pack.status);
  return pack;
}

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
  std::cerr << "[OK!]\nboot up stage 4... [compile shaders] "; 

  shader_pack vertex = create_shader(SOURCE_vertex_, GL_VERTEX_SHADER);
  if (!vertex.status) {
    std::cerr << "[FAIL]\ncompiling vertex shader fail!\n";
    abort();
  }
  shader_pack fragment = create_shader(SOURCE_fragment_, GL_FRAGMENT_SHADER);
  if (!fragment.status) {
    std::cerr << "[FAIL]\nfailed compilation of fragment shader!\n";
    abort();
  }
  std::cerr << "[OK!]\n";
  std::cerr << "[reglyph]> Basic boot done! from this moment no boot stages will be displayed\n";

  unsigned int shader = glCreateProgram();
  glAttachShader(shader, vertex.id);
  glAttachShader(shader, fragment.id);
  glLinkProgram(shader);

  // linked

  GLint shader_status;
  glGetProgramiv(shader, GL_LINK_STATUS, &shader_status);
  if (!shader_status) {
    char log[1024];
    glGetProgramInfoLog(shader, sizeof(log), nullptr, log);

    std::cerr << "[reglyph]! Failed shader linkage!\n[reglyph]! " << log;
    abort();
  }
  // cleanup from left-overs
  glDeleteShader(vertex.id);
  glDeleteShader(fragment.id);
  glUseProgram(shader);

  rg::rend::rend_obj render; // init renderer object
  
  // set empty screen color
  glClearColor(0.1059, 0.1059, 0.1059, 1.0);

  int x, y; // state variables

  glfwGetWindowSize(window, &x, &y); // init

  struct bar_data {
    bool active;
    rg::rend::vec2<int> size, pos;
    rg::rend::vec4<float> color;
  }; // for segment rendering (yea, the name is bar but its for segments)


  int sidebar_width = 200; // to make them resizable later
  int topbar_height = 50; 

  // std settings

  bar_data sidebar = {
    .active = true,
    .size = {sidebar_width, y},
    .pos = {0, 0},
    .color = {0.1059, 0.1059, 0.1059, 1.0}
  };
  bar_data top_bar = {
    .active = true,
    .size = {x - sidebar_width, topbar_height},
    .pos = {sidebar_width, 0},
    .color = {0.1059, 0.1059, 0.1059, 1.0}
  };
  bar_data main_seg = {
    .active = true,
    .size = {x-sidebar_width, y-topbar_height},
    .pos = {sidebar_width, topbar_height},
    .color = {0.1843, 0.3098, 0.3098, 1.0}
  };

  // preemptive Viewport size setting to fix a bug when gl viewport is smaller than window size until resize
  glViewport(0, 0, x, y);

  // resize callback
  glfwSetFramebufferSizeCallback(window, [](GLFWwindow* win, int width, int height) {
    layout_dirty = true;
    glViewport(0, 0, width, height);
  });

  glfwSwapBuffers(window);
  while (!glfwWindowShouldClose(window)) {
    // wait for events
    glfwPollEvents();
    // cleanup

    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shader);
    
    // if size changed
    if (layout_dirty) {
      glfwGetWindowSize(window, &x, &y);

      top_bar.size = {x - sidebar_width, topbar_height};
      sidebar.size = {sidebar_width, y};
      main_seg.size = {x - sidebar_width, y - topbar_height};
      main_seg.pos = {sidebar_width, topbar_height};
      layout_dirty = false;
      // done
    }

    // render segments
    if (top_bar.active) {
      render.draw_rect(top_bar.pos, top_bar.size, {x, y}, top_bar.color);
    }
    if (sidebar.active) {
      render.draw_rect(sidebar.pos, sidebar.size, {x, y}, sidebar.color);
    }
    if (main_seg.active) {
      render.draw_rect(main_seg.pos, main_seg.size, {x, y}, main_seg.color); 
    }
    // submit
    glfwSwapBuffers(window);
  }
  std::cerr << "[reglyph]! Bye!\n" << std::flush;
  glfwDestroyWindow(window);
  return 0;
}
