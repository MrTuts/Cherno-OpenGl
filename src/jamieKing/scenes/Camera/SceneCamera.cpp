#include "SceneCamera.h"
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

  SceneCamera::SceneCamera(GLFWwindow *window) : Scene::Scene(window), m_Rotation(glm::vec3(20.0f, 20.0f, 0.0f)), m_Translation(glm::vec3(0.0f, 0.0f, -3.0f))
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
    GLCall(glEnableVertexAttribArray(1));
    GLCall(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(sizeof(float) * 3)));

    /* Index buffer */
    GLCall(glGenBuffers(1, &m_IBO_ID));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO_ID));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, shapeData.indexBufferSize(), shapeData.indices, GL_STATIC_DRAW));

    /* Transform matrix buffer */
    GLCall(glGenBuffers(1, &m_TMB_ID));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_TMB_ID));
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * 2, nullptr, GL_DYNAMIC_DRAW));

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

    m_Shader = std::make_unique<Shader>(RELATIVE_SHADER_PATH("Camera.vert"), RELATIVE_SHADER_PATH("Camera.frag"));
    m_Shader->Bind();
    m_NumIndices = shapeData.numIndices;
    shapeData.cleanup();
    glEnable(GL_DEPTH_TEST);

    if (glfwRawMouseMotionSupported())
    {
      glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    glfwSetWindowUserPointer(window, this);
    glfwSetScrollCallback(window, ScrollCallback);
  }

  SceneCamera::~SceneCamera()
  {
    glDisable(GL_DEPTH_TEST);
    glfwSetInputMode(m_Window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
    if (glfwGetWindowUserPointer(m_Window) == this)
    {
      glfwSetWindowUserPointer(m_Window, nullptr);
    }
    glfwSetScrollCallback(m_Window, ImGui_ImplGlfw_ScrollCallback);

    GLCall(glDeleteBuffers(1, &m_VBO_ID));
    GLCall(glDeleteBuffers(1, &m_IBO_ID));
    GLCall(glDeleteBuffers(1, &m_TMB_ID));
  }

  void SceneCamera::ScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
  {
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);

    SceneCamera *scene = static_cast<SceneCamera *>(glfwGetWindowUserPointer(window));
    if (scene)
    {
      scene->OnScroll(xoffset, yoffset);
    }
  }

  void SceneCamera::OnScroll(double xoffset, double yoffset)
  {
    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
      return;
    }
    m_Camera.mouseZoom(yoffset);
  }

  void SceneCamera::OnUpdate(float deltaTime)
  {
    ImGuiIO &io = ImGui::GetIO();

    // If the mouse is over an ImGui window, let ImGui have it and return early
    if (io.WantCaptureMouse)
    {
      return;
    }

    static bool rotating = false;
    static bool moving = false;
    static glm::vec2 lbStartPos;
    static glm::vec2 rbStartPos;

    double mouseXPos, mouseYPos;
    glfwGetCursorPos(m_Window, &mouseXPos, &mouseYPos);
    glm::vec2 mousePos = glm::vec2(mouseXPos, mouseYPos);

    int lbState = glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_LEFT);
    int rbState = glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_RIGHT);
    if (lbState == GLFW_PRESS && rbState == GLFW_PRESS)
    {
      // move
      if (!moving)
      {
        m_Camera.mouseMoveStart(mousePos);
      }
      moving = true;
      m_Camera.mouseMoveUpdate(mousePos);
    }
    else
    {
      moving = false;
    }

    if (!moving && rbState == GLFW_PRESS)
    {
      if (!rotating)
      {
        m_Camera.mouseRotateStart(mousePos);
      }
      rotating = true;
      m_Camera.mouseRotateUpdate(mousePos);
    }
    else
    {
      rotating = false;
    }

    if (glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_PRESS)
    {
      m_Camera.moveForward();
    }
    if (glfwGetKey(m_Window, GLFW_KEY_S) == GLFW_PRESS)
    {
      m_Camera.moveBackward();
    }
    if (glfwGetKey(m_Window, GLFW_KEY_A) == GLFW_PRESS)
    {
      m_Camera.strafeLeft();
    }
    if (glfwGetKey(m_Window, GLFW_KEY_D) == GLFW_PRESS)
    {
      m_Camera.strafeRight();
    }
  }

  void SceneCamera::OnRender()
  {
    // define data
    int width, height;
    glfwGetWindowSize(m_Window, &width, &height);
    glm::mat4 projectionMatrix = glm::perspective(glm::radians(60.0f), (static_cast<float>(width) / static_cast<float>(height)), 0.1f, 10.0f);

    // clang-format off
    glm::mat4 fullTransforms[] = {
/* view to projection  |       world to view             |                 model to world                  */
      projectionMatrix * m_Camera.getWorldToViewMatrix() * glm::translate(glm::vec3(-1.0f, 0.0f, -3.0f)) * glm::rotate(glm::radians(36.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
      projectionMatrix * m_Camera.getWorldToViewMatrix() * glm::translate(glm::vec3(1.0f, 0.0f, -3.75f)) * glm::rotate(glm::radians(126.0f), glm::vec3(0.0f, 1.0f, 0.0f))
    };
    // clang-format on

    // send data to GPU
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(fullTransforms), fullTransforms, GL_DYNAMIC_DRAW));

    GLCall(glDrawElementsInstanced(GL_TRIANGLES, m_NumIndices, GL_UNSIGNED_SHORT, nullptr, 2));
  }

  void SceneCamera::OnImGuiRender()
  {
  }
}