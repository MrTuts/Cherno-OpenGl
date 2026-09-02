#pragma once

#include <vector>
#include <functional>
#include <iostream>
#include "Scene.h"
#include "Renderer.h"
#include "../../../common/Shader.h"

namespace jking::scene
{
  class SceneBasicTriangle : public ::scene::Scene
  {
  public:
    SceneBasicTriangle();
    ~SceneBasicTriangle() {}

    void OnUpdate(float deltaTime) override;
    void OnRender(GLFWwindow *window) override;
    void OnImGuiRender() override;

  private:
    GLuint m_VAO_ID;
    GLuint m_VBO_ID;
    GLuint m_IBO_ID;
    std::unique_ptr<Shader> m_Shader;
  };
}