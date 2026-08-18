#pragma once

#include <iostream>
#include <vector>
#include <functional>

namespace scene
{
  class Scene
  {
  public:
    Scene() {};
    virtual ~Scene() {}

    virtual void OnUpdate(float deltaTime) {}
    virtual void OnRender() {}
    virtual void OnImGuiRender() {}
  };
}