#pragma once

#include "Scene.h"
#include "../IndexBuffer.h"
#include "../VertexArray.h"
#include "../VertexBufferLayout.h"
#include "../Texture.h"
#include "glm/glm.hpp"

namespace scene
{
  class ScenePizza : public Scene
  {
  public:
    ScenePizza();
    ~ScenePizza();

    void OnUpdate(float deltaTime) override;
    void OnRender(Renderer renderer) override;
    void OnImGuiRender() override;

  private:
    VertexArray va;
    VertexBuffer vb;
    Shader shader;
    // VertexBufferLayout layout;
    Texture texture;
    glm::vec3 translationA;
    glm::vec3 translationB;
    glm::mat4 projectionMatrix;
    glm::mat4 viewMatrix;
    IndexBuffer ib;
  };
}
