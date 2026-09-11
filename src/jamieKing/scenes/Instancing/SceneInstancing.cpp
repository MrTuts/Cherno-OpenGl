#include "SceneInstancing.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp> // for glm::perspective
#include <imgui.h>

#include "Renderer.h"
#include "../../primitives/Vertex.h"
#include "../../primitives/ShapeGenerator.h"
#include "../../../common/Shader.h"

namespace jking::scene
{

  SceneInstancing::SceneInstancing()
  {

    // clang-format off
    GLfloat vertices[] = {
      -1.0f, 0.0f,
      -1.0f, 1.0f,
      -0.9f, 0.0f
    };
    GLushort indices[] = {
      0,1,2
    };

    GLfloat offsets[] ={
     0.0f, 0.5f, 1.0f, 1.2f, 1.6f
    };
    // clang-format on

    GLCall(glGenVertexArrays(1, &m_VAO_ID));
    GLCall(glBindVertexArray(m_VAO_ID));

    /* Vertex buffer */
    GLCall(glGenBuffers(1, &m_VBO_ID));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_VBO_ID));
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));
    // Define attributes for this vertex buffer
    GLCall(glEnableVertexAttribArray(0));
    // we don't have to specify offset/stride, because we only have one attribute for this buffer
    GLCall(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0));

    /* Offset buffer */
    // Buffer for offset data
    GLCall(glGenBuffers(1, &m_OffsetBO_ID));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_OffsetBO_ID));
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(offsets), offsets, GL_STATIC_DRAW));
    // Define attributes for this offset buffer
    GLCall(glEnableVertexAttribArray(1));
    // even thought this is a second attribute, we don't need to specify offset/stride, because it's a single attribute for this buffer
    GLCall(glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, 0));
    // For instance rendering. For attribute 1, we want divisor to be one
    /*
      When we draw using `glDrawElementsInstanced`, we divide each instance index by the divisor,
      the result is the index of data (from offsets) that will be use for the instance draw.
      For 5 instances, it is
      0/1 -> 0 (offset 0.0)
      1/1 -> 1 (offset 0.5)
      2/1 -> 2 (offset 1.0)
      3/1 -> 3 (..)
      4/1 -> 4 (..)
      ----
      for divisor 2
      0/2 -> 0 (offset 0.0)
      1/2 -> 0 (offset 0.0)
      2/2 -> 1 (offset 0.5)
      3/2 -> 1 (offset 0.5)
      4/2 -> 2 (offset 1.0)
      We would reuse same offset of two triangles
    */
    GLCall(glVertexAttribDivisor(1, 1));

    /* index buffer */
    GLCall(glGenBuffers(1, &m_IBO_ID));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO_ID));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW));

    m_Shader = std::make_unique<Shader>(RELATIVE_SHADER_PATH("Instancing.vert"), RELATIVE_SHADER_PATH("Instancing.frag"));
    m_Shader->Bind();
  }

  SceneInstancing::~SceneInstancing()
  {
    GLCall(glDeleteBuffers(1, &m_VBO_ID));
    GLCall(glDeleteBuffers(1, &m_OffsetBO_ID));
    GLCall(glDeleteBuffers(1, &m_IBO_ID));
  }

  void SceneInstancing::OnUpdate(float deltaTime)
  {
  }

  void SceneInstancing::OnRender()
  {
    // draws x (5 - last arg) of instances
    GLCall(glDrawElementsInstanced(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, nullptr, 5));
  }

  void SceneInstancing::OnImGuiRender()
  {
  }
}