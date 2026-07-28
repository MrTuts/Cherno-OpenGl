#include "IndexBuffer.h"
#include "Renderer.h"

IndexBuffer::IndexBuffer(const unsigned int *data, unsigned int count) : m_Count(count)
{
  ASSERT(sizeof(unsigned int) == sizeof(GLuint)); // these should always be the same, but just to be cautious

  GLCall(glGenBuffers(1, &m_RendererID));                                                            // create an index buffer (IBO) and store its ID in buffer
  GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID));                                       // tells OpenGL we are working with data of this buffer
  GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(unsigned int), data, GL_STATIC_DRAW)); // upload data to the buffer. Can be done whenever (but the  buffer has to be bound)
}

IndexBuffer::~IndexBuffer()
{
  GLCall(glDeleteBuffers(1, &m_RendererID));
}

void IndexBuffer::Bind() const
{
  GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID)); // tells OpenGL we are working with data of this buffer
}

void IndexBuffer::Unbind() const
{
  GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)); // tells OpenGL we are working with data of this buffer
}