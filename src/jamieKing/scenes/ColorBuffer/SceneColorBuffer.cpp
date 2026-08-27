#pragma once
#include "SceneColorBuffer.h"
#include <GLFW/glfw3.h>

#include "Renderer.h"
#include "../../../common/Shader.h"

static const float X_DELTA = 0.1f;
static const uint NUM_VERTICES_PER_TRI = 3;
static const uint NUM_FLOAT_PER_VERTEX = 6;
static const uint TRIANGL_BYTE_SIZE = NUM_VERTICES_PER_TRI * NUM_FLOAT_PER_VERTEX * sizeof(float);
static const uint MAX_TRIS = 20;
static uint numTris = 0;
static double lastUpdate = 0;
static bool bufferFilled = false;

static void sendAnotherTriangleToOpenGL()
{
  if (numTris == MAX_TRIS)
  {
    bufferFilled = true;
    numTris = 0;
    return;
  }
  if (bufferFilled)
  {
    numTris++;
    return;
  }

  const GLfloat THIS_TRI_X = -1 + numTris * X_DELTA;
  // clang-format off
    GLfloat thisTri[] = {
      THIS_TRI_X, 1.0f, 0.0f,
      1.0f, 0.0f, 0.0f,

      THIS_TRI_X + X_DELTA, 1.0f, 0.0f,
      1.0f, 0.0f, 0.0f,

      THIS_TRI_X, 0.0f, 0.0f,
      1.0f, 0.0f, 0.0f,
    };
  // clang-format on

  /*
    There are two color buffers, front and back buffer, which switch places.
    Front buffer is what is currently drawn.
    Back buffer is where we push new data.
    In Application.cpp we swap these buffers after each render.
    This means that when we push data here on each frame, part of it is pushed to one buffer, the other part to the second buffer.
    When we do not clear the color buffer, we can see how the buffer swap (though it happens very fast) - each containing part of the data, until they are both filled.
   */
  GLCall(glBufferSubData(GL_ARRAY_BUFFER, numTris * TRIANGL_BYTE_SIZE, TRIANGL_BYTE_SIZE, thisTri));
  numTris++;
}

namespace jking::scene
{

  SceneColorBuffer::SceneColorBuffer()
  {
    m_ControlsBuffer = true;
    bufferFilled = false;
    numTris = 0;

    GLCall(glGenVertexArrays(1, &m_VAO_ID));
    GLCall(glBindVertexArray(m_VAO_ID));

    GLCall(glGenBuffers(1, &m_VBO_ID));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_VBO_ID));
    // prealocate
    GLCall(glBufferData(GL_ARRAY_BUFFER, MAX_TRIS * TRIANGL_BYTE_SIZE, NULL, GL_STATIC_DRAW));

    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0));

    GLCall(glEnableVertexAttribArray(1));
    GLCall(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(sizeof(float) * 3)));

    m_Shader = std::make_unique<Shader>("src/jamieKing/scenes/ColorBuffer/ColorBuffer.vert", "src/jamieKing/scenes/ColorBuffer/ColorBuffer.frag");
    m_Shader->Bind();
  }

  void SceneColorBuffer::OnUpdate(float deltaTime)
  {
    const double time = glfwGetTime();
    if (time - lastUpdate < 0.016)
    {
      // return;
    }
    lastUpdate = time;
    sendAnotherTriangleToOpenGL();
  }

  void SceneColorBuffer::OnRender()
  {
    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    GLCall(glDrawArrays(GL_TRIANGLES, (numTris - 1) * NUM_VERTICES_PER_TRI, NUM_VERTICES_PER_TRI));
  }

  void SceneColorBuffer::OnImGuiRender()
  {
  }
}