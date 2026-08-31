#pragma once

#include <vector>
#include <functional>
#include <iostream>
#include "Scene.h"
#include "Renderer.h"
#include "../../../common/Shader.h"
#include "../../primitives/ShapeData.h"

namespace jking::scene
{
  class SceneBasicCube : public ::scene::Scene
  {
  public:
    SceneBasicCube();
    ~SceneBasicCube() {}

    void OnUpdate(float deltaTime) override;
    void OnRender() override;
    void OnImGuiRender() override;

  private:
    GLuint m_VAO_ID;
    GLuint m_VBO_ID;
    GLuint m_IBO_ID;
    std::unique_ptr<Shader> m_Shader;
    // std::unique_ptr<ShapeData> shapeData;
  };
}