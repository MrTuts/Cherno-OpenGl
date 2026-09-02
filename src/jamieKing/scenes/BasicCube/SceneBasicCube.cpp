#include "SceneBasicCube.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "Renderer.h"
#include "../../primitives/Vertex.h"
#include "../../primitives/ShapeGenerator.h"
#include "../../../common/Shader.h"

namespace jking::scene
{

  SceneBasicCube::SceneBasicCube()
  {
    ShapeData shapeData = ShapeGenerator::makeCube();
    // shapeData = std::make_unique<ShapeData>(ShapeGenerator::makeTriangle());

    GLCall(glGenVertexArrays(1, &m_VAO_ID));
    GLCall(glBindVertexArray(m_VAO_ID));

    GLCall(glGenBuffers(1, &m_VBO_ID));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_VBO_ID));
    GLCall(glBufferData(GL_ARRAY_BUFFER, shapeData.vertexBufferSize(), shapeData.vertices, GL_STATIC_DRAW));

    GLCall(glGenBuffers(1, &m_IBO_ID));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO_ID));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, shapeData.indexBufferSize(), shapeData.indices, GL_STATIC_DRAW));

    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0));

    GLCall(glEnableVertexAttribArray(1));
    GLCall(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(sizeof(float) * 3)));

    m_Shader = std::make_unique<Shader>(RELATIVE_SHADER_PATH("BasicCube.vert"), RELATIVE_SHADER_PATH("BasicCube.frag"));
    m_Shader->Bind();
    shapeData.cleanup();
  }

  void SceneBasicCube::OnUpdate(float deltaTime)
  {
  }

  void SceneBasicCube::OnRender()
  {
    glm::vec3 dominatigColor(1.0f, 0.0f, 0.0f);
    m_Shader->SetUniform3fv("dominatingColor", 1, &dominatigColor[0]);
    GLCall(glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, nullptr));
  }

  void SceneBasicCube::OnImGuiRender()
  {
  }
}