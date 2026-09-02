#include "SceneBatchColor.h"
#include <imgui.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "../Renderer.h"
#include "../IndexBuffer.h"
#include "../VertexArray.h"
#include "../VertexBufferLayout.h"
#include "../Texture.h"

namespace cherno::scene
{

  SceneBatchColor::SceneBatchColor() : m_Translation(glm::vec3(200.0f, 200.0f, 0.0f)),
                                       m_ProjectionMatrix(glm::ortho(0.0f, 640.0f, 0.0f, 480.0f, -1.0f, 1.0f)),
                                       m_ViewMatrix(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)))

  {
    // clang-format off
    const float vertices[] = {
        -50.0f, -50.0f, 0.18f, 0.06f, 0.96f, 1.0f, // index 0
        50.0f, -50.0f, 0.18f, 0.06f, 0.96f, 1.0f, // index 1
        50.0f,  50.0f, 0.18f, 0.06f, 0.96f, 1.0f,  // index 2
        -50.0f,  50.0f, 0.18f, 0.06f, 0.96f, 1.0f,  // index 3
        
        150.0f, -50.0f, 1.0f, 0.93f, 0.24f, 1.0f, // index 4
        250.0f, -50.0f, 1.0f, 0.93f, 0.24f, 1.0f, // index 5
        250.0f,  50.0f, 1.0f, 0.93f, 0.24f, 1.0f,  // index 6
        150.0f,  50.0f, 1.0f, 0.93f, 0.24f, 1.0f, // index 7
    };
    // indices can be chars or shorts, but MUST be unsigned
    const unsigned int indices[] = {
      0,1,2, // bottom right triangle
      2,3,0,  // top left triangle
      
      4,5,6, // bottom right triangle
      6,7,4  // top left triangle
    };
    // clang-format on

    m_VAO = std::make_unique<VertexArray>();

    m_VBO = std::make_unique<VertexBuffer>(vertices, 8 * 6 * sizeof(float));
    VertexBufferLayout layout;
    layout.Push<float>(2); // locations
    layout.Push<float>(4); // rgba color
    m_VAO->Addbuffer(*m_VBO, layout);

    m_IBO = std::make_unique<IndexBuffer>(indices, 12);

    m_Shader = std::make_unique<Shader>("res/shaders/BatchColor.vert", "res/shaders/BatchColor.frag");
    m_Shader->Bind();
  }

  SceneBatchColor::~SceneBatchColor()
  {
  }

  void SceneBatchColor::OnUpdate(float deltaTime)
  {
  }

  void SceneBatchColor::OnRender(GLFWwindow *window)
  {
    GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
    GLCall(glClear(GL_COLOR_BUFFER_BIT));

    // move object based on translation value
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), m_Translation);

    /*
    Multiplication order matters! in OpenGL we work with column major,
    Direct3D and other are row major, we would multiply in reverse order modelMatrix * m_ViewMatrix * m_ProjectionMatrix
    */
    glm::mat4 mvpMatrix = m_ProjectionMatrix * m_ViewMatrix * modelMatrix;
    m_Shader->Bind();
    m_Shader->SetUniformMat4f("u_MVP", mvpMatrix);
    cherno::Draw(*m_VAO, *m_IBO, *m_Shader);
  }

  void SceneBatchColor::OnImGuiRender()
  {
    ImGui::SliderFloat3("Translation", &m_Translation.x, 0.0f, 640.0f);
  }
}
