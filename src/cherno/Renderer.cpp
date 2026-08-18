#include "./Renderer.h"

namespace cherno
{
  void Draw(const VertexArray &vao, const IndexBuffer &ibo, const Shader &shader)
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
}
