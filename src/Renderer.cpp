#include "Renderer.h"

#include <iostream>

void GLClearError()
{
  // loop and clear all errors
  while (glGetError() != GL_NO_ERROR)
    ;
}

bool GLLogCall(const char *function, const char *file, int line)
{
  while (GLenum error = glGetError())
  {
    // Look up the error code inside glad.h
    std::cout << "[OpenGL Error] (0x" << std::hex << error << std::dec << "): " << function << " " << file << ": " << line << std::endl;
    return false;
  }
  return true;
}

void Renderer::Clear() const
{
  GLCall(glClear(GL_COLOR_BUFFER_BIT));
}

void Renderer::Draw(const VertexArray &vao, const IndexBuffer &ibo, const Shader &shader) const
{
  // Draw triangle using legacy API
  /*
  glBegin(GL_TRIANGLES);
  glVertex2f(-0.5f, -0.5f);
  glVertex2f(0.0f, 0.5f);
  glVertex2f(0.5f, -0.5f);
  glEnd();
  */

  // Draw triangle using OpenGL 3.3+ API
  // glBindBuffer(GL_ARRAY_BUFFER, vbo);
  // glDrawArrays(GL_TRIANGLES, 0, 6);

  shader.Bind();
  vao.Bind();
  // no need to also bind vertex buffer (vbo) and specify the attributes layout, that is already linked with the vao
  ibo.Bind();

  GLCall(glDrawElements(GL_TRIANGLES, ibo.GetCount(), GL_UNSIGNED_INT, nullptr));
}
