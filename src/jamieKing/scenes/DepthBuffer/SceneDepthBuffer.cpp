#pragma once
#include "SceneDepthBuffer.h"
#include <GLFW/glfw3.h>

#include "Renderer.h"
#include "../../../common/Shader.h"

namespace jking::scene
{
  SceneDepthBuffer::SceneDepthBuffer()
  {
    const float RED_TRIANGLE_Z = 0.5f;
    const float BLUE_TRIANGLE_Z = -0.5f;
    // clang-format off
    GLfloat vertices[] = {
      -1.0f, -1.0f, RED_TRIANGLE_Z, 1.0f, 0.0f, 0.0f, 1.0f,
      1.0f, -1.0f, RED_TRIANGLE_Z, 1.0f, 0.0f, 0.0f, 1.0f,
      0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
      
      0.0f, -1.0f, BLUE_TRIANGLE_Z, 0.0f, 0.0f, 1.0f, 1.0f,
      1.0f, 1.0f, BLUE_TRIANGLE_Z, 0.0f, 0.0f, 1.0f, 1.0f,
      0.0f, 1.0f, BLUE_TRIANGLE_Z, 0.0f, 0.0f, 1.0f, 1.0f,
    };

    GLushort indices[] = {
      0,1,2,
      3,4,5
    };
    // clang-format on

    /*
      Enable depth buffer
      This comes with some performance overhead
      Depth buffer is 2D array of numbers, the size of the array is the same as the size of pixel space,
      meaning every pixel has it's depth associated with it.
      When we output color in fragment shader, we run depth test, which evaluates whether the new fragment is closer to the camera,
      hence if it should be used or discarded. The comparison is if(newDepth<prevDepth), so in case of equal value, prev is used
      Depth buffer works with "z" axis, where +1 is furthest away from camera and -1 closes to camera.
      "z" must be between 1 > z >= -1, everything outside is not rendered (fails the depth test)
      "z" value is interpolated between vertices like everything else
    */
    GLCall(glEnable(GL_DEPTH_TEST));

    GLCall(glGenVertexArrays(1, &m_VAO_ID));
    GLCall(glBindVertexArray(m_VAO_ID));

    GLCall(glGenBuffers(1, &m_VBO_ID));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_VBO_ID));
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));

    GLCall(glGenBuffers(1, &m_IBO_ID));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO_ID));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW));

    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), 0));

    GLCall(glEnableVertexAttribArray(1));
    GLCall(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *)(sizeof(float) * 3)));

    m_Shader = std::make_unique<Shader>("src/jamieKing/scenes/DepthBuffer/DepthBuffer.vert", "src/jamieKing/scenes/DepthBuffer/DepthBuffer.frag");
    m_Shader->Bind();
    m_Shader->SetUniform1d("u_elapsedTime", glfwGetTime());
  }

  SceneDepthBuffer::~SceneDepthBuffer()
  {
    GLCall(glDisable(GL_DEPTH_TEST));
  }

  void SceneDepthBuffer::OnUpdate(float deltaTime)
  {
    m_Shader->SetUniform1d("u_elapsedTime", glfwGetTime());
  }

  void SceneDepthBuffer::OnRender()
  {
    // clear the depth buffer array, every value is set to 1.0 (furthest away)
    glClear(GL_DEPTH_BUFFER_BIT);
    GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr));
  }

  void SceneDepthBuffer::OnImGuiRender()
  {
  }
}