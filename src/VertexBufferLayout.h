#pragma once

#include <vector>
#include <GLFW/glfw3.h>
#include "Renderer.h"

struct VertexBufferElement
{
  unsigned int type;
  unsigned int count;
  unsigned char normalized;

  static unsigned int GetSizeOfType(unsigned int type)
  {
    switch (type)
    {
    case GL_FLOAT:
      return 4;
    case GL_UNSIGNED_INT:
      return 4;
    case GL_UNSIGNED_BYTE:
      return 1;
    }

    ASSERT(false);
    return 0;
  }
};

template <typename>
inline constexpr bool always_false = false;

class VertexBufferLayout
{
private:
  std::vector<VertexBufferElement> m_Elements;
  unsigned int m_Stride;

public:
  VertexBufferLayout();

  template <typename T>
  void Push(unsigned int count)
  {
    // ChatGPT advice: make the assert condition depend on T, so we do not get compilation error
    static_assert(
        always_false<T>,
        "Unsupported vertex buffer element type");
  }

  inline const std::vector<VertexBufferElement> &GetElements() const { return m_Elements; }
  inline unsigned int GetStride() const { return m_Stride; }
};

template <>
void VertexBufferLayout::Push<float>(unsigned int count);

template <>
void VertexBufferLayout::Push<unsigned int>(unsigned int count);

template <>
void VertexBufferLayout::Push<unsigned char>(unsigned int count);