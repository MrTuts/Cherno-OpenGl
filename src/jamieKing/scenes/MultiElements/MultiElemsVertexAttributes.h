#pragma once

#include <vector>
#include <functional>
#include <iostream>
#include "Scene.h"
#include "Renderer.h"
#include "Camera.h"
#include "../../CameraControls.h"
#include "../../../common/Shader.h"
#include "../../primitives/ShapeData.h"
#include <glm/glm.hpp>

namespace jking::scene
{
  class MultiElemsVertexAttributes : public ::scene::Scene
  {
  public:
    MultiElemsVertexAttributes(GLFWwindow *window);
    ~MultiElemsVertexAttributes();

    void OnUpdate(float deltaTime) override;
    void OnRender() override;
    void OnImGuiRender() override;

  private:
    GLuint m_VAO_ID;
    GLuint m_VBO_Cube_ID;
    GLuint m_VBO_Arrow_ID;
    GLuint m_IBO_Cube_ID;
    GLuint m_IBO_Arrow_ID;
    std::unique_ptr<Shader> m_Shader;
    unsigned int m_CubeNumIndices;
    unsigned int m_ArrowNumIndices;
    glm::vec3 m_Rotation;
    glm::vec3 m_Translation;
    Camera m_Camera;
    CameraControls m_CameraControls;
  };
}