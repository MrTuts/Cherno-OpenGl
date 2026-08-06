#pragma once

#include "../Renderer.h"

namespace scene
{
  class Scene
  {
  public:
    Scene() {};
    virtual ~Scene() {}

    virtual void OnUpdate(float deltaTime) {}
    virtual void OnRender(Renderer renderer) {}
    virtual void OnImGuiRender() {}
  };
}