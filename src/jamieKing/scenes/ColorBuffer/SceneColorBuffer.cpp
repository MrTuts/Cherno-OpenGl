#include "SceneColorBuffer.h"
#include <GLFW/glfw3.h>

#include "Renderer.h"
#include "../../../common/Shader.h"

static const float X_DELTA = 0.1f;
static const unsigned int NUM_VERTICES_PER_TRI = 3;
static const unsigned int NUM_FLOAT_PER_VERTEX = 6;
static const unsigned int TRIANGL_BYTE_SIZE = NUM_VERTICES_PER_TRI * NUM_FLOAT_PER_VERTEX * sizeof(float);
static const unsigned int MAX_TRIS = 20;

namespace jking::scene
{

  SceneColorBuffer::SceneColorBuffer()
  {
    m_ControlsBuffer = true;

    GLCall(glGenVertexArrays(1, &m_VAO_ID));
    GLCall(glBindVertexArray(m_VAO_ID));

    GLCall(glGenBuffers(1, &m_VBO_ID));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_VBO_ID));
    // reserve VBO space without uploading data; triangles are written incrementally via glBufferSubData
    GLCall(glBufferData(GL_ARRAY_BUFFER, MAX_TRIS * TRIANGL_BYTE_SIZE, NULL, GL_STATIC_DRAW));

    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0));

    GLCall(glEnableVertexAttribArray(1));
    GLCall(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(sizeof(float) * 3)));

    m_Shader = std::make_unique<Shader>(RELATIVE_SHADER_PATH("ColorBuffer.vert"), RELATIVE_SHADER_PATH("ColorBuffer.frag"));
    m_Shader->Bind();
  }

  void SceneColorBuffer::OnUpdate(float deltaTime)
  {
    const double time = glfwGetTime();
    if (time - m_LastUpdate < 0.5)
      return;
    m_LastUpdate = time;

    if (m_NumTris == MAX_TRIS)
    {
      m_NumTris = 0;
    }

    const GLfloat THIS_TRI_X = -1 + m_NumTris * X_DELTA;
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
    // glBufferSubData writes to the VBO (shared GPU memory) — NOT to the back color buffer.
    // The draw call in OnRender is what writes pixels into the back color buffer.
    GLCall(glBufferSubData(GL_ARRAY_BUFFER, m_NumTris * TRIANGL_BYTE_SIZE, TRIANGL_BYTE_SIZE, thisTri));
    m_NumTris++;
    m_TriangleAdded = true;
  }

  void SceneColorBuffer::OnRender(GLFWwindow *window)
  {
    // No clear — stale pixels remain, so each physical buffer accumulates its own set of triangles.
    // GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    // Only draw on the single frame a triangle was added; this ensures the triangle's pixels land
    // in exactly one physical color buffer. Without this guard, OnRender fires every frame and
    // stamps the same triangle into both back buffers before the next addition, preventing flickering.
    if (!m_TriangleAdded)
      return;
    m_TriangleAdded = false;
    GLCall(glDrawArrays(GL_TRIANGLES, (m_NumTris - 1) * NUM_VERTICES_PER_TRI, NUM_VERTICES_PER_TRI));
  }

  void SceneColorBuffer::OnImGuiRender()
  {
  }
}