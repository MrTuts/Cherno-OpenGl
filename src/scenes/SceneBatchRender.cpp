#include "SceneBatchRender.h"
#include <imgui.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "../Renderer.h"
#include "../IndexBuffer.h"
#include "../VertexArray.h"
#include "../VertexBufferLayout.h"
#include "../Texture.h"

namespace scene
{

  SceneBatchRender::SceneBatchRender() : m_Translation(glm::vec3(200.0f, 200.0f, 0.0f)),
                                         m_ProjectionMatrix(glm::ortho(0.0f, 640.0f, 0.0f, 480.0f, -1.0f, 1.0f)),
                                         m_ViewMatrix(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)))

  {
    // clang-format off
    /* 
      The rectangle we want to draw
      3 ---- 2
      |    |
      |    |
      0 ---- 1
    */
    static const float vertices[] = {
        -50.0f, -50.0f, 0.0f, 0.0f, // index 0
        50.0f, -50.0f, 1.0f, 0.0f, // index 1
        50.0f,  50.0f, 1.0f, 1.0f,  // index 2
        -50.0f,  50.0f, 0.0f, 1.0f,  // index 3
        
        150.0f, -50.0f, 0.0f, 0.0f, // index 4
        250.0f, -50.0f, 1.0f, 0.0f, // index 5
        250.0f,  50.0f, 1.0f, 1.0f,  // index 6
        150.0f,  50.0f, 0.0f, 1.0f  // index 7
    };
    // indices can be chars or shorts, but MUST be unsigned
    static const unsigned int indices[] = {
      0,1,2, // bottom right triangle
      2,3,0,  // top left triangle
      
      4,5,6, // bottom right triangle
      6,7,4  // top left triangle
    };
    // clang-format on

    // When using core profile, we need to create vertex array buffer
    // With compatibility profile, there is one vao created that stores everything. Since it stores everything,
    // all attrib layouts need to be re-specified every time together with the array buffer
    // --
    // This allows us to bind the array buffer and set a layout to it (vertex attributes)
    // VertexBuffer vb{vertices, 4 * 4 * sizeof(float)};

    m_VAO = std::make_unique<VertexArray>();

    m_VBO = std::make_unique<VertexBuffer>(vertices, 8 * 4 * sizeof(float));
    VertexBufferLayout layout;
    layout.Push<float>(2); // locations
    layout.Push<float>(2); // texture coords
    m_VAO->Addbuffer(*m_VBO, layout);

    m_IBO = std::make_unique<IndexBuffer>(indices, 12);

    m_Shader = std::make_unique<Shader>("res/shaders/Basic.vert", "res/shaders/Basic.frag");
    // with shaders in one file
    // Shader shader{"res/shaders/Basic.glsl"};
    m_Shader->Bind();
    m_Shader->SetUniform4f("u_Color", 0.8f, 0.3f, 0.8f, 1.0f);

    m_Texture = std::make_unique<Texture>("res/textures/pizza.png");
    m_Shader->SetUniform1i("u_Texture", 0); // 0 is the slot this texture is bound to
  }

  SceneBatchRender::~SceneBatchRender()
  {
  }

  void SceneBatchRender::OnUpdate(float deltaTime)
  {
  }

  void SceneBatchRender::OnRender(Renderer renderer)
  {
    GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
    GLCall(glClear(GL_COLOR_BUFFER_BIT));

    m_Texture->Bind();
    {
      // move object based on translation value
      glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), m_Translation);

      /*
      Multiplication order matters! in OpenGL we work with column major,
      Direct3D and other are row major, we would multiply in reverse order modelMatrix * m_ViewMatrix * m_ProjectionMatrix
      */
      glm::mat4 mvpMatrix = m_ProjectionMatrix * m_ViewMatrix * modelMatrix;
      m_Shader->Bind();
      m_Shader->SetUniformMat4f("u_MVP", mvpMatrix);
      renderer.Draw(*m_VAO, *m_IBO, *m_Shader);
    }
  }

  void SceneBatchRender::OnImGuiRender()
  {
    ImGui::SliderFloat3("Translation", &m_Translation.x, 0.0f, 640.0f);
  }
}
