#include "SceneBasicTriangle.h"

#include "Renderer.h"

namespace jking::scene
{
  void BasicTriangleScene::OnRender()
  {
    float vertices[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        0.0f, 1.0f};
    // GLCall(glGenBuffers()
    GLuint bufferId;
    GLCall(glGenBuffers(1, &bufferId));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, bufferId));
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));
  }

  void BasicTriangleScene::OnImGuiRender()
  {
  }
}