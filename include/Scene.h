#pragma once

#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <functional>

namespace scene
{
  class Scene
  {
  protected:
    bool m_ControlsBuffer;
    GLFWwindow *m_Window;

  public:
    Scene() : m_ControlsBuffer(false), m_Window(nullptr) {};
    Scene(GLFWwindow *window) : m_ControlsBuffer(false), m_Window(window) {};
    virtual ~Scene() {}

    virtual void OnUpdate(float deltaTime) {}
    virtual void OnRender() {}
    virtual void OnImGuiRender() {}

    inline bool controlsBuffer()
    {
      return m_ControlsBuffer;
    }
  };
}