#include "SceneCubeInstances.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp> // for glm::perspective
#include <imgui.h>

#include "Renderer.h"
#include "../../primitives/Vertex.h"
#include "../../primitives/ShapeGenerator.h"
#include "../../../common/Shader.h"

namespace jking::scene
{

  SceneCubeInstances::SceneCubeInstances(GLFWwindow *window) : Scene::Scene(window), m_Rotation(glm::vec3(20.0f, 20.0f, 0.0f)), m_Translation(glm::vec3(0.0f, 0.0f, -3.0f)), m_FOV(60.0f), m_Near(0.1f), m_Far(10.0f)
  {
    ShapeData shapeData = ShapeGenerator::makeCube();

    GLCall(glGenVertexArrays(1, &m_VAO_ID));
    GLCall(glBindVertexArray(m_VAO_ID));

    /* Vertex buffer */
    GLCall(glGenBuffers(1, &m_VBO_ID));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_VBO_ID));
    GLCall(glBufferData(GL_ARRAY_BUFFER, shapeData.vertexBufferSize(), shapeData.vertices, GL_STATIC_DRAW));
    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0));
    /*
      We can specify the shader attribute also like this:
      glVertexAttrib3f(1, 0, 1, 0);
      This would in this case set color for every vertex to green (instead of it varying per vertex).
      In this case we would NOT call glEnableVertexAttribArray(1)
    */
    GLCall(glEnableVertexAttribArray(1));
    GLCall(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(sizeof(float) * 3)));

    /* Index buffer */
    GLCall(glGenBuffers(1, &m_IBO_ID));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO_ID));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, shapeData.indexBufferSize(), shapeData.indices, GL_STATIC_DRAW));

    /* Transform matrix buffer */
    GLCall(glGenBuffers(1, &m_TMB_ID));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_TMB_ID));
    // define data
    int width, height;
    glfwGetWindowSize(m_Window, &width, &height);
    glm::mat4 projectionMatrix = glm::perspective(glm::radians(m_FOV), (static_cast<float>(width) / static_cast<float>(height)), m_Near, m_Far);
    glm::mat4 fullTransforms[] = {
        projectionMatrix * glm::translate(glm::vec3(-1.0f, 0.0f, -3.0f)) * glm::rotate(glm::radians(36.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        projectionMatrix * glm::translate(glm::vec3(1.0f, 0.0f, -3.75f)) * glm::rotate(glm::radians(126.0f), glm::vec3(0.0f, 1.0f, 0.0f))};
    // send data to GPU
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(fullTransforms), fullTransforms, GL_STATIC_DRAW));
    GLCall(glEnableVertexAttribArray(2));
    GLCall(glEnableVertexAttribArray(3));
    GLCall(glEnableVertexAttribArray(4));
    GLCall(glEnableVertexAttribArray(5));
    // attributes are send in as groups of 4, so we need to divide our matrix (16 floats) into 4 groups of 4 floats
    // the vertex shader still receives all data in layout=2, because it is defined as mat4
    GLCall(glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *)(sizeof(float) * 0)));
    GLCall(glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *)(sizeof(float) * 4)));
    GLCall(glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *)(sizeof(float) * 8)));
    GLCall(glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *)(sizeof(float) * 12)));
    // See SceneInstancing
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);

    m_Shader = std::make_unique<Shader>(RELATIVE_SHADER_PATH("CubeInstances.vert"), RELATIVE_SHADER_PATH("CubeInstances.frag"));
    m_Shader->Bind();
    m_NumIndices = shapeData.numIndices;
    shapeData.cleanup();
    glEnable(GL_DEPTH_TEST);
  }

  SceneCubeInstances::~SceneCubeInstances()
  {
    glDisable(GL_DEPTH_TEST);
  }

  void SceneCubeInstances::OnUpdate(float deltaTime)
  {
  }

  void SceneCubeInstances::OnRender()
  {
    GLCall(glDrawElementsInstanced(GL_TRIANGLES, m_NumIndices, GL_UNSIGNED_SHORT, nullptr, 2));
  }

  void SceneCubeInstances::OnImGuiRender()
  {
  }
}