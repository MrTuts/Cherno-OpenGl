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
  class MultiElemsSingleArrayBuffer : public ::scene::Scene
  {
  public:
    MultiElemsSingleArrayBuffer(GLFWwindow *window);
    ~MultiElemsSingleArrayBuffer();

    void OnUpdate(float deltaTime) override;
    void OnRender() override;
    void OnImGuiRender() override;

  private:
    GLuint m_VAO_Cube_ID;
    GLuint m_VAO_Arrow_ID;
    GLuint m_VBO_ID;
    std::unique_ptr<Shader> m_Shader;
    unsigned int m_CubeNumIndices;
    unsigned int m_ArrowNumIndices;
    unsigned int m_ArrowIndexDataByteOffset;
    unsigned int m_CubeIndexDataByteOffset;
    glm::vec3 m_Rotation;
    glm::vec3 m_Translation;
    Camera m_Camera;
    CameraControls m_CameraControls;
  };
}