#pragma once

#include <vector>
#include <functional>
#include <iostream>
#include "Scene.h"
#include "Renderer.h"
#include "../../../common/Shader.h"
#include "../../primitives/ShapeData.h"
#include <glm/glm.hpp>

namespace jking::scene
{
  class SceneCubeInstances : public ::scene::Scene
  {
  public:
    SceneCubeInstances(GLFWwindow *window);
    ~SceneCubeInstances();

    void OnUpdate(float deltaTime) override;
    void OnRender() override;
    void OnImGuiRender() override;

  private:
    GLuint m_VAO_ID;
    GLuint m_TMB_ID; // transformation matrix buffer
    GLuint m_VBO_ID;
    GLuint m_IBO_ID;
    std::unique_ptr<Shader> m_Shader;
    unsigned int m_NumIndices;
    glm::vec3 m_Rotation;
    glm::vec3 m_Translation;
    float m_FOV;
    float m_Near;
    float m_Far;
    // std::unique_ptr<ShapeData> shapeData;
  };
}