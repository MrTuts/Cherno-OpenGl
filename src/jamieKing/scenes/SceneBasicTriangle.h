#pragma once

#include <vector>
#include <functional>
#include <iostream>
#include "Scene.h"
#include "Renderer.h"
#include "../../common/Shader.h"

namespace jking::scene
{
  class BasicTriangleScene : public ::scene::Scene
  {
  public:
    BasicTriangleScene();
    ~BasicTriangleScene() {}

    void OnUpdate(float deltaTime) override;
    void OnRender() override;
    void OnImGuiRender() override;

  private:
    GLuint m_VAO_ID;
    GLuint m_VBO_ID;
    GLuint m_IBO_ID;
    std::unique_ptr<Shader> m_Shader;
  };
}