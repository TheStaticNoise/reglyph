#include "vecx.h"
#include "rend.h"
#include "segs.h"
#include "graph.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <optional>
#include <print>

bool re = false;

int main() {
  std::cerr << "test unit 1: [splitter]\n";
  int c;
  if (!(c = glfwInit())) {
    std::cerr << "[unrelated] glfw init error " << c << "\n";
    return -1;
  }
  rg::graph::window_config cfg = {
    .height = 800,
    .width = 500,
    .handle = "test unit",
  };
  GLFWwindow* window = rg::graph::create_window(cfg);
  rg::graph::set_limit(window, cfg.width, cfg.height, -1, -1);
  if (window == nullptr) {
    std::cerr << "[unrelated] glfw window error" << c << "\n";
    return -1;
  }
  glfwMakeContextCurrent(window);
  if (!(c = gladLoadGL(glfwGetProcAddress))) {
    std::cerr << "[unrelated] glad init error " << c << "\n";
    std::abort();
    return -1;
  }
  rg::rend::rend_obj render;
  rg::segs::splitter mng;

  int x, y;
  render.clear();
  glfwGetWindowSize(window, &x, &y);
  render.set_screensize({x, y});
  mng.screensize({x, y});
  glfwSetFramebufferSizeCallback(window, [](GLFWwindow* win, int width, int height) {
      glViewport(0, 0, width, height);
      re = true;
      });
  glfwSwapBuffers(window);
  glViewport(0, 0, x, y);


  auto redraw_layout = [](rg::rend::rend_obj& render, rg::segs::splitter& mng) {
    mng.scrap();
    render.clear();
    mng.split(rg::segs::direction_t::UP, 0, 0.1, {0.1f, 0.0f, 0.5f, 1.0f}, std::nullopt);
    mng.split(rg::segs::direction_t::UP, 0, 0.1, {0.0f, 0.0f, 1.0f, 1.0f}, std::nullopt);
    mng.split(rg::segs::direction_t::DOWN, 0, 0.1, {1.0f, 1.0f, 0.0f, 1.0f}, std::nullopt);
    mng.split(rg::segs::direction_t::DOWN, 0, 0.1, {1.0f, 1.0f, 1.0f, 1.0f}, std::nullopt);
    mng.split(rg::segs::direction_t::UP, 0, 0.01, {1.0f, 0.0f, 0.0f, 1.0f}, std::nullopt);
    mng.split(rg::segs::direction_t::LEFT, 0, 0.2, {1.0f, 0.5f, 1.0f, 1.0f}, std::nullopt);
    mng.split(rg::segs::direction_t::LEFT, 0, 0.2, {1.0f, 0.5f, 0.0f, 1.0f}, std::nullopt);
    mng.split(rg::segs::direction_t::RIGHT, 0, 0.2, {1.0f, 0.5f, 1.0f, 1.0f}, std::nullopt);
    mng.split(rg::segs::direction_t::RIGHT, 0, 0.2, {1.0f, 0.5f, 0.0f, 1.0f}, std::nullopt);
    mng.rend_add(render, 0); 
    render.submit(); 
  };
  redraw_layout(render, mng);

  while (!glfwWindowShouldClose(window)) {
    if (re) {
      glfwGetWindowSize(window, &x, &y);
      mng.screensize({x, y});

      redraw_layout(render, mng);
      render.set_screensize({x, y});
      re = false;
    }
    glfwPollEvents();
    render.submit();
    glfwSwapBuffers(window);

  }
}
