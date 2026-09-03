#include "SceneBasicCube.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp> // for glm::perspective
#include <imgui.h>

#include "Renderer.h"
#include "../../primitives/Vertex.h"
#include "../../primitives/ShapeGenerator.h"
#include "../../../common/Shader.h"

namespace jking::scene
{

  SceneBasicCube::SceneBasicCube() : m_Rotation(glm::vec3(20.0f, 20.0f, 0.0f)), m_Translation(glm::vec3(0.0f, 0.0f, -3.0f))
  {
    ShapeData shapeData = ShapeGenerator::makeCube();

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
  }

  void SceneBasicCube::OnRender(GLFWwindow *window)
  {
    glm::vec3 dominatigColor(1.0f, 0.0f, 0.0f);

    int width, height;
    glfwGetWindowSize(window, &width, &height);

    /* it's faster to calculate matrices here and pass one final matrix into the shader */

    /* this is more obvious code how the matrices are multiplied, but need several intermediate matrices */
    // glm::mat4 modelTranslationMatrix = glm::translate(glm::mat4(1.0f), m_Translation);
    // glm::mat4 modelRotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    // modelRotationMatrix = glm::rotate(modelRotationMatrix, glm::radians(m_Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    // modelRotationMatrix = glm::rotate(modelRotationMatrix, glm::radians(m_Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    // glm::mat4 projectionMatrix = glm::perspective(glm::radians(60.0f), static_cast<float>(width / height), 0.1f, 10.0f);
    // glm::mat4 modelTransformMatrix = modelTranslationMatrix * modelRotationMatrix;
    // glm::mat4 fullTransformMatrix = projectionMatrix * modelTransformMatrix;

    /* less obvious how the matrices are multiplied, but we have single matrix on which we apply the transformations. The order of transformations is reversed */
    // 3. project to perspective
    glm::mat4 fullTransformMatrix = glm::perspective(glm::radians(60.0f), static_cast<float>(width / height), 0.1f, 10.0f);
    // 2. translate
    fullTransformMatrix = glm::translate(fullTransformMatrix, m_Translation);
    // 1. rotate
    fullTransformMatrix = glm::rotate(fullTransformMatrix, glm::radians(m_Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    fullTransformMatrix = glm::rotate(fullTransformMatrix, glm::radians(m_Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    fullTransformMatrix = glm::rotate(fullTransformMatrix, glm::radians(m_Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    m_Shader->SetUniformMat4f("fullTransformMatrix", fullTransformMatrix);
    GLCall(glDrawElements(GL_TRIANGLES, m_NumIndices, GL_UNSIGNED_SHORT, nullptr));
  }

  void SceneBasicCube::OnImGuiRender()
  {
    ImGui::SliderFloat3("Rotation", &m_Rotation.x, 0.0f, 360.0f);
    ImGui::SliderFloat3("Translation", &m_Translation.x, -15.0f, 11.0f);
  }
}