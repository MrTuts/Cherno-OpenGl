#include "MultiElemsVertexArrayBuffer.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp> // for glm::perspective
#include <imgui.h>
#include "backends/imgui_impl_glfw.h"

#include "Renderer.h"
#include "../../primitives/Vertex.h"
#include "../../primitives/ShapeGenerator.h"
#include "../../../common/Shader.h"

namespace jking::scene
{

  MultiElemsVertexArrayBuffer::MultiElemsVertexArrayBuffer(GLFWwindow *window) : Scene::Scene(window), m_CameraControls(window, m_Camera), m_Rotation(glm::vec3(20.0f, 20.0f, 0.0f)), m_Translation(glm::vec3(0.0f, 0.0f, -3.0f))
  {
    /* Cubes */
    ShapeData shapeData = ShapeGenerator::makeCube();

    GLCall(glGenVertexArrays(1, &m_VAO_Cube_ID));
    GLCall(glBindVertexArray(m_VAO_Cube_ID));

    /* Vertex buffer */
    GLCall(glGenBuffers(1, &m_VBO_ID));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_VBO_ID));
    GLCall(glBufferData(GL_ARRAY_BUFFER, shapeData.vertexBufferSize(), shapeData.vertices, GL_STATIC_DRAW));
    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0));
    GLCall(glEnableVertexAttribArray(1));
    GLCall(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(sizeof(float) * 3)));

    /* Index buffer */
    GLCall(glGenBuffers(1, &m_IBO_ID));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO_ID));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, shapeData.indexBufferSize(), shapeData.indices, GL_STATIC_DRAW));
    m_CubeNumIndices = shapeData.numIndices;
    shapeData.cleanup();

    /* Arrow */
    shapeData = ShapeGenerator::makeArrow();

    GLCall(glGenVertexArrays(1, &m_VAO_Arrow_ID));
    GLCall(glBindVertexArray(m_VAO_Arrow_ID));

    /* Vertex buffer */
    GLCall(glGenBuffers(1, &m_VBO_ID));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_VBO_ID));
    GLCall(glBufferData(GL_ARRAY_BUFFER, shapeData.vertexBufferSize(), shapeData.vertices, GL_STATIC_DRAW));
    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0));
    GLCall(glEnableVertexAttribArray(1));
    GLCall(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(sizeof(float) * 3)));

    /* Index buffer */
    GLCall(glGenBuffers(1, &m_IBO_ID));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO_ID));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, shapeData.indexBufferSize(), shapeData.indices, GL_STATIC_DRAW));
    m_ArrowNumIndices = shapeData.numIndices;
    shapeData.cleanup();

    /* Other */
    m_Shader = std::make_unique<Shader>(RELATIVE_SHADER_PATH("Shader.vert"), RELATIVE_SHADER_PATH("Shader.frag"));
    m_Shader->Bind();
    glEnable(GL_DEPTH_TEST);
    m_CameraControls.Setup();
  }

  MultiElemsVertexArrayBuffer::~MultiElemsVertexArrayBuffer()
  {
    glDisable(GL_DEPTH_TEST);
  }

  void MultiElemsVertexArrayBuffer::OnUpdate(float deltaTime)
  {
    m_CameraControls.OnUpdate(deltaTime);
  }

  void MultiElemsVertexArrayBuffer::OnRender()
  {
    // define data
    int width, height;
    glfwGetWindowSize(m_Window, &width, &height);
    glm::mat4 viewToProjectionMatrix = glm::perspective(glm::radians(60.0f), (static_cast<float>(width) / static_cast<float>(height)), 0.1f, 10.0f);
    glm::mat4 worldToViewMatrix = m_Camera.getWorldToViewMatrix();
    glm::mat4 worldToProjectionMatrix = viewToProjectionMatrix * m_Camera.getWorldToViewMatrix();

    glm::mat4 elementModelToWorld = glm::translate(glm::vec3(-1.0f, 0.0f, -3.0f)) * glm::rotate(glm::radians(36.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    GLCall(glBindVertexArray(m_VAO_Cube_ID));
    m_Shader->SetUniformMat4f("fullTransformMatrix", worldToProjectionMatrix * elementModelToWorld);
    GLCall(glDrawElements(GL_TRIANGLES, m_CubeNumIndices, GL_UNSIGNED_SHORT, nullptr));

    elementModelToWorld = glm::translate(glm::vec3(1.0f, 0.0f, -3.75f)) * glm::rotate(glm::radians(126.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_Shader->SetUniformMat4f("fullTransformMatrix", worldToProjectionMatrix * elementModelToWorld);
    GLCall(glDrawElements(GL_TRIANGLES, m_CubeNumIndices, GL_UNSIGNED_SHORT, nullptr));

    GLCall(glBindVertexArray(m_VAO_Arrow_ID));
    elementModelToWorld = glm::translate(glm::vec3(0.0f, 0.0f, -3.0f));
    m_Shader->SetUniformMat4f("fullTransformMatrix", worldToProjectionMatrix * elementModelToWorld);
    GLCall(glDrawElements(GL_TRIANGLES, m_ArrowNumIndices, GL_UNSIGNED_SHORT, nullptr));
  }

  void MultiElemsVertexArrayBuffer::OnImGuiRender()
  {
  }
}