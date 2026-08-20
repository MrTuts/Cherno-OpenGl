#pragma once

#include <Renderer.h>
#include "./VertexArray.h"
#include "./IndexBuffer.h"
#include "../common/Shader.h"

namespace cherno
{
  void Draw(const VertexArray &va, const IndexBuffer &ib, const Shader &shader);
}