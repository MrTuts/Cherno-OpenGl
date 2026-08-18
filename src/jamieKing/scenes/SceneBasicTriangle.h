#pragma once

#include <vector>
#include <functional>
#include <iostream>
#include "Scene.h"

namespace jking::scene
{
  class BasicTriangleScene : public ::scene::Scene
  {
  public:
    BasicTriangleScene() {}
    ~BasicTriangleScene() {}

    void OnRender() override;
    void OnImGuiRender() override;

  private:
  };
}