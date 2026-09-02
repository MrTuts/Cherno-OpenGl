#pragma once

#include <memory>
#include "Scene.h"
#include "glm/glm.hpp"
#include "../IndexBuffer.h"
#include "../VertexArray.h"
#include "../VertexBufferLayout.h"
#include "../Texture.h"

namespace cherno::scene
{
  class SceneBatchColor : public ::scene::Scene
  {
  public:
    SceneBatchColor();
    ~SceneBatchColor();

    void OnUpdate(float deltaTime) override;
    void OnRender(GLFWwindow *window) override;
    void OnImGuiRender() override;

  private:
    std::unique_ptr<VertexArray> m_VAO;
    std::unique_ptr<VertexBuffer> m_VBO;
    std::unique_ptr<Shader> m_Shader;
    std::unique_ptr<IndexBuffer> m_IBO;
    glm::mat4 m_ProjectionMatrix;
    glm::mat4 m_ViewMatrix;
    glm::vec3 m_Translation;
  };
}
