#include "VertexArray.h"
#include "Renderer.h"

VertexArray::VertexArray()
{
  GLCall(glGenVertexArrays(1, &m_RendererID));
}

VertexArray::~VertexArray()
{
  GLCall(glDeleteVertexArrays(1, &m_RendererID));
}

void VertexArray::Addbuffer(const VertexBuffer &vb, const VertexBufferLayout &layout)
{
  Bind();
  vb.Bind();

  const auto &elements = layout.GetElements();
  size_t offset = 0;
  for (size_t i = 0; i < elements.size(); i++)
  {
    const auto &element = elements[i];
    GLCall(glEnableVertexAttribArray(i));
    // specify how the data is laid out in the buffer.
    // This code technically links the array buffer with vao
    // params
    // i is the index of the attribute,
    // element.count is the number of components,
    // element.type is the type of each component,
    // element.normalized is whether we want to normalize the data (transforming 0-255 to 0-1),
    // layout.GetStride() is the stride (the distance between consecutive attributes)
    // offset is the offset (the starting point of the first attribute).
    GLCall(glVertexAttribPointer(i, element.count, element.type, element.normalized, layout.GetStride(), reinterpret_cast<const void *>(offset)));
    offset += element.count * VertexBufferElement::GetSizeOfType(element.type);
  }
}

void VertexArray::Bind() const
{
  GLCall(glBindVertexArray(m_RendererID));
}

void VertexArray::Unbind() const
{
  GLCall(glBindVertexArray(0));
}
