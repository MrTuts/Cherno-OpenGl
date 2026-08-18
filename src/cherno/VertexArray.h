#pragma once

#include "VertexBuffer.h"

// avoid include cycle
class VertexBufferLayout;

class VertexArray
{
private:
  unsigned int m_RendererID;

public:
  VertexArray();
  ~VertexArray();

  void Addbuffer(const VertexBuffer &vb, const VertexBufferLayout &layout);

  void Bind() const;
  void Unbind() const;
};