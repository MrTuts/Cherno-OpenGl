#pragma once

#include "Scene.h"

namespace cherno::scene
{
  class SceneClearColor : public ::scene::Scene
  {
  public:
    SceneClearColor();
    ~SceneClearColor();

    void OnUpdate(float deltaTime) override;
    void OnRender(GLFWwindow *window) override;
    void OnImGuiRender() override;

  private:
    float m_ClearColor[4];
  };
}
