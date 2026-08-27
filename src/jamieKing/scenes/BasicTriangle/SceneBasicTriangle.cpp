#pragma once
#include "SceneBasicTriangle.h"
#include <GLFW/glfw3.h>

#include "Renderer.h"
#include "../../../common/Shader.h"

namespace jking::scene
{
  SceneBasicTriangle::SceneBasicTriangle()
  {
    // clang-format off
    GLfloat vertices[] = {
      -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
      1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
      0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
      
      1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
      -1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
    };

    GLushort indices[] = {
      0,1,2,
      2,3,4
    };
    // clang-format on

    GLCall(glGenVertexArrays(1, &m_VAO_ID));
    GLCall(glBindVertexArray(m_VAO_ID));

    GLCall(glGenBuffers(1, &m_VBO_ID));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_VBO_ID));
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));

    GLCall(glGenBuffers(1, &m_IBO_ID));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO_ID));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW));

    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0));

    GLCall(glEnableVertexAttribArray(1));
    GLCall(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(sizeof(float) * 2)));

    m_Shader = std::make_unique<Shader>("src/jamieKing/scenes/BasicTriangle/BasicTriangle.vert", "src/jamieKing/scenes/BasicTriangle/BasicTriangle.frag");
    m_Shader->Bind();
    m_Shader->SetUniform1d("u_elapsedTime", glfwGetTime());
  }

  void SceneBasicTriangle::OnUpdate(float deltaTime)
  {
    m_Shader->SetUniform1d("u_elapsedTime", glfwGetTime());
  }

  void SceneBasicTriangle::OnRender()
  {

    GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr));
  }

  void SceneBasicTriangle::OnImGuiRender()
  {
  }
}