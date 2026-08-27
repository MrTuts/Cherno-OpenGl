#pragma once

#include <iostream>
#include <vector>
#include <functional>

namespace scene
{
  class Scene
  {
  protected:
    bool m_ControlsBuffer;

  public:
    Scene() : m_ControlsBuffer(false) {};
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