#include "VertexBuffer.h"
#include "./Renderer.h"

VertexBuffer::VertexBuffer(const void *data, unsigned int size)
{
  GLCall(glGenBuffers(1, &m_RendererID));                            // create a vertex buffer (VBO) and store its ID in buffer
  GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_RendererID));               // tells OpenGL we are working with data of this buffer
  GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW)); // upload data to the buffer. Can be done whenever (but the  buffer has to be bound)
}

VertexBuffer::~VertexBuffer()
{
  GLCall(glDeleteBuffers(1, &m_RendererID));
}

void VertexBuffer::Bind() const
{
  GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_RendererID)); // tells OpenGL we are working with data of this buffer
}

void VertexBuffer::Unbind() const
{
  GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0)); // tells OpenGL we are working with data of this buffer
}