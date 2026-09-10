#pragma once

#include "Camera.h"
#include <GLFW/glfw3.h>

class CameraControls
{
private:
  GLFWwindow *m_Window;
  Camera &m_Camera;

public:
  CameraControls(GLFWwindow *window, Camera &camera);
  ~CameraControls();
  void OnUpdate(float deltaTime);
  void Setup();

private:
  void OnScroll(double xoffset, double yoffset);
  static void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset);
};