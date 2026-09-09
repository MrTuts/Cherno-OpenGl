#pragma once

#include <vector>
#include <functional>
#include <iostream>
#include "Scene.h"
#include "Renderer.h"
#include "Camera.h"
#include "../../../common/Shader.h"
#include "../../primitives/ShapeData.h"
#include <glm/glm.hpp>

namespace jking::scene
{
  class SceneCamera : public ::scene::Scene
  {
  public:
    SceneCamera(GLFWwindow *window);
    ~SceneCamera();

    void OnUpdate(float deltaTime) override;
    void OnRender() override;
    void OnImGuiRender() override;
    void OnScroll(double xoffset, double yoffset);

  private:
    static void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset);

    GLuint m_VAO_ID;
    GLuint m_TMB_ID; // transformation matrix buffer
    GLuint m_VBO_ID;
    GLuint m_IBO_ID;
    std::unique_ptr<Shader> m_Shader;
    unsigned int m_NumIndices;
    glm::vec3 m_Rotation;
    glm::vec3 m_Translation;
    Camera m_Camera;

    // std::unique_ptr<ShapeData> shapeData;
  };
}