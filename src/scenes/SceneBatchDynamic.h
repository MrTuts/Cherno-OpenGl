#pragma once

#include <memory>
#include "Scene.h"
#include "glm/glm.hpp"
#include "../IndexBuffer.h"
#include "../VertexArray.h"
#include "../VertexBufferLayout.h"
#include "../Texture.h"

namespace scene
{
  class SceneBatchDynamic : public Scene
  {
  public:
    SceneBatchDynamic();
    ~SceneBatchDynamic();

    void OnUpdate(float deltaTime) override;
    void OnRender(Renderer renderer) override;
    void OnImGuiRender() override;

  private:
    unsigned int m_QuadVA;
    unsigned int m_QuadVB;
    unsigned int m_QuadIB;
    float m_QuadPos[2] = {-1, -1};
    std::unique_ptr<Shader> m_Shader;
    std::unique_ptr<Texture> m_TexturePizza;
    std::unique_ptr<Texture> m_TextureBaguette;
    glm::mat4 m_ProjectionMatrix;
    glm::mat4 m_ViewMatrix;
    glm::vec3 m_Translation;
  };
}
