#pragma once

#include <vector>
#include <functional>
#include <iostream>
#include "Scene.h"
#include "Renderer.h"
#include "../../../common/Shader.h"

namespace jking::scene
{
  class SceneColorBuffer : public ::scene::Scene
  {
  public:
    SceneColorBuffer();
    ~SceneColorBuffer() {}

    void OnUpdate(float deltaTime) override;
    void OnRender() override;
    void OnImGuiRender() override;

  private:
    GLuint m_VAO_ID;
    GLuint m_VBO_ID;
    GLuint m_IBO_ID;
    std::unique_ptr<Shader> m_Shader;
    unsigned int m_NumTris;
    double m_LastUpdate;
    bool m_TriangleAdded = false;
  };
}