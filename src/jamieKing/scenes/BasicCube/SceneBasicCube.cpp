#include "SceneBasicCube.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp> // for glm::perspective

#include "Renderer.h"
#include "../../primitives/Vertex.h"
#include "../../primitives/ShapeGenerator.h"
#include "../../../common/Shader.h"

namespace jking::scene
{

  SceneBasicCube::SceneBasicCube() : m_ModelTransformMatrix(glm::mat4(1.0f))
  {
    ShapeData shapeData = ShapeGenerator::makeCube();
    m_ModelTransformMatrix = glm::translate(m_ModelTransformMatrix, glm::vec3(0.0f, 0.0f, -4.0f));

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
    m_NumIndices = shapeData.numIndices;
    shapeData.cleanup();
    glEnable(GL_DEPTH_TEST);
  }

  SceneBasicCube::~SceneBasicCube()
  {
    glDisable(GL_DEPTH_TEST);
  }

  void SceneBasicCube::OnUpdate(float deltaTime)
  {
    m_ModelTransformMatrix = glm::rotate(
        m_ModelTransformMatrix,
        glm::radians(deltaTime * 20.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    m_ModelTransformMatrix = glm::rotate(
        m_ModelTransformMatrix,
        glm::radians(deltaTime * 20.0f),
        glm::vec3(0.0f, 0.0f, 1.0f));
  }

  void SceneBasicCube::OnRender(GLFWwindow *window)
  {
    glm::vec3 dominatigColor(1.0f, 0.0f, 0.0f);

    int width, height;
    glfwGetWindowSize(window, &width, &height);
    glm::mat4 projectionMatrix = glm::perspective(glm::radians(60.0f), static_cast<float>(width / height), 0.1f, 10.0f);

    m_Shader->SetUniformMat4f("modelTransformMatrix", m_ModelTransformMatrix);
    m_Shader->SetUniformMat4f("projectionMatrix", projectionMatrix);
    GLCall(glDrawElements(GL_TRIANGLES, m_NumIndices, GL_UNSIGNED_SHORT, nullptr));
  }

  void SceneBasicCube::OnImGuiRender()
  {
  }
}