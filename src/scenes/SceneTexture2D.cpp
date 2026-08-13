#include "SceneTexture2D.h"
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

  SceneTexture2D::SceneTexture2D() : m_Translation(glm::vec3(320.0f, 240.0f, 0.0f)),
                                     /* MVP
                                      view matrix - matrix for the view of the camera, position, scale, other.
                                      model matrix - matrix for the vertex we are drawing; transformation of the model
                                      projection matrix - converting the space we work with (e.g. 0, 640) into (-1, 1)
                                     */
                                     // orthographic projection - things do not get smaller with their distance to the camera (no perspective)
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
    const float vertices[] = {
        -320.0f, -240.0f, 0.0f, 0.0f, // index 0
        320.0f, -240.0f, 1.0f, 0.0f, // index 1
        320.0f,  240.0f, 1.0f, 1.0f,  // index 2
        -320.0f,  240.0f, 0.0f, 1.0f  // index 3
    };
    // indices can be chars or shorts, but MUST be unsigned
    const unsigned int indices[] = {
      0,1,2, // bottom right triangle
      2,3,0  // top left triangle
    };
    // clang-format on

    // When using core profile, we need to create vertex array buffer
    // With compatibility profile, there is one vao created that stores everything. Since it stores everything,
    // all attrib layouts need to be re-specified every time together with the array buffer
    // --
    // This allows us to bind the array buffer and set a layout to it (vertex attributes)
    // VertexBuffer vb{vertices, 4 * 4 * sizeof(float)};

    m_VAO = std::make_unique<VertexArray>();

    m_VBO = std::make_unique<VertexBuffer>(vertices, 4 * 4 * sizeof(float));
    VertexBufferLayout layout;
    layout.Push<float>(2); // locations
    layout.Push<float>(2); // texture coords
    m_VAO->Addbuffer(*m_VBO, layout);

    m_IBO = std::make_unique<IndexBuffer>(indices, 6);

    m_Shader = std::make_unique<Shader>("res/shaders/Basic.vert", "res/shaders/Basic.frag");
    // with shaders in one file
    // Shader shader{"res/shaders/Basic.glsl"};
    m_Shader->Bind();
    m_Shader->SetUniform4f("u_Color", 0.8f, 0.3f, 0.8f, 1.0f);

    m_Texture = std::make_unique<Texture>("res/textures/pizza.png");
    m_Shader->SetUniform1i("u_Texture", 0); // 0 is the slot this texture is bound to
  }

  SceneTexture2D::~SceneTexture2D()
  {
  }

  void SceneTexture2D::OnUpdate(float deltaTime)
  {
  }

  void SceneTexture2D::OnRender(Renderer renderer)
  {
    GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
    GLCall(glClear(GL_COLOR_BUFFER_BIT));

    m_Texture->Bind();
    /* Rendering multiple objects
        Here we render the same object twice by changing the uniform, we call the Draw function twice. This is good for e.g. rendering
        3D objects.
        Another method is batch rendering, where instead we pass all the vertices to GPU and render it in only one Draw function.
        This batch rendering is better for e.g. rendering many tiles on screen, rendering text
      */
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

  void SceneTexture2D::OnImGuiRender()
  {
    ImGui::SliderFloat3("Translation A", &m_Translation.x, 0.0f, 640.0f);
  }
}
