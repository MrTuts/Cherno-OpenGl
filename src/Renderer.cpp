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
