#pragma once

#include <GLFW/glfw3.h>
#include <string>
#include <cstdint>

namespace rg::graph {
  struct window_properties {
    bool resizable = true;
    bool decorated = true;
    bool transparent = false;
    bool floating = false;
    bool maximized = false;
    int samples = 2;
    int refresh_rate = GLFW_DONT_CARE;
  };
  struct window_config {
    uint64_t height, width;
    std::string handle;
    window_properties hints;
  };

  static GLFWwindow* create_window(const window_config& cfg) {
    glfwWindowHint(GLFW_RESIZABLE, cfg.hints.resizable);
    glfwWindowHint(GLFW_DECORATED, cfg.hints.decorated);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, cfg.hints.transparent);
    glfwWindowHint(GLFW_FLOATING, cfg.hints.floating);
    glfwWindowHint(GLFW_MAXIMIZED, cfg.hints.maximized);
    glfwWindowHint(GLFW_REFRESH_RATE, cfg.hints.refresh_rate);
    glfwWindowHint(GLFW_SAMPLES, cfg.hints.samples);
    GLFWwindow *window = glfwCreateWindow(cfg.width, cfg.height, cfg.handle.c_str(), nullptr, nullptr);
    if (window == nullptr) {
      return nullptr;
    }
    return window;

  }
}
