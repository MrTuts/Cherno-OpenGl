#pragma once

#include <glad/glad.h>
#include "VertexArray.h"
#include "IndexBuffer.h"
#include "Shader.h"

/* break the code execution when x assertion fails */
#if defined(_WIN32)
#define ASSERT(x) \
  if (!(x))       \
  __debugbreak()
#elif defined(__clang__)
#define ASSERT(x) \
  if (!(x))       \
  __builtin_debugtrap()
#else
#include <csignal>
#define ASSERT(x) \
  if (!(x))       \
  raise(SIGTRAP)
#endif

/* Wrapper to clear gl errors, run x and assert no errors appeared */
#ifdef DEBUG
#define GLCall(x) \
  GLClearError(); \
  x;              \
  ASSERT(GLLogCall(#x, __FILE__, __LINE__)) // #x turns function into its name
#else
#define GLCall(x) x
#endif

void GLClearError();
bool GLLogCall(const char *function, const char *file, int line);

class Renderer
{
public:
  void Clear() const;
  void Draw(const VertexArray &va, const IndexBuffer &ib, const Shader &shader) const;
};