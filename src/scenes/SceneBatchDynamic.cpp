#include "SceneBatchDynamic.h"
#include <imgui.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "../Renderer.h"
#include "../IndexBuffer.h"
#include "../VertexArray.h"
#include "../VertexBufferLayout.h"
#include "../Texture.h"

namespace
{
  struct Vec4
  {
    float x, y, z, w;
  };

  struct Vec2
  {
    float x, y;
  };

  struct Vertex
  {
    Vec2 Position;
    Vec4 Color;
    Vec2 TexCoords;
    float TexID;
  };
}

static std::array<Vertex, 4> CreateQuad(float x, float y, float texID)
{
  float size = 200.0f;

  Vertex v0;
  v0.Position = {x, y};
  v0.Color = {0.18f, 0.06f, 0.96f, 1.0f};
  v0.TexCoords = {0.0f, 0.0f};
  v0.TexID = texID;

  Vertex v1;
  v1.Position = {x + size, y};
  v1.Color = {0.18f, 0.06f, 0.96f, 1.0f};
  v1.TexCoords = {1.0f, 0.0f};
  v1.TexID = texID;

  Vertex v2;
  v2.Position = {x + size, y + size};
  v2.Color = {0.18f, 0.06f, 0.96f, 1.0f};
  v2.TexCoords = {1.0f, 1.0f};
  v2.TexID = texID;

  Vertex v3;
  v3.Position = {x, y + size};
  v3.Color = {0.18f, 0.06f, 0.96f, 1.0f};
  v3.TexCoords = {0.0f, 1.0f};
  v3.TexID = texID;

  return {v0, v1, v2, v3};
}

namespace scene
{

  /*
    We render two quads with different textures. We load two textures in two slots and in the vertex we specify texture coordinates and slot index
    In the fragment shader we than use these values to render the correct texture on the quad.
    This approach has limitation of number of textures we can draw in single batch, since the number of available slots is quite low and varies between different GPUs.
    We can use texture atlases instead to overcome this limitation (one file containing multiple textures, we specify the texture position).
   */
  SceneBatchDynamic::SceneBatchDynamic() : m_Translation(glm::vec3(200.0f, 200.0f, 0.0f)),
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
    
    // indices can be chars or shorts, but MUST be unsigned
    const unsigned int indices[] = {
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

    GLCall(glGenVertexArrays(1, &m_QuadVA));
    GLCall(glBindVertexArray(m_QuadVA));

    // create dynamic vertex buffer
    GLCall(glGenBuffers(1, &m_QuadVB));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_QuadVB));
    // we pass nullptr to glBufferData to create an empty buffer of the specified size,
    // which we can fill later with glBufferSubData or by mapping the buffer
    // We allocate space for 1000 of vertices.
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 1000, nullptr, GL_DYNAMIC_DRAW));

    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *)offsetof(Vertex, Position)));
    GLCall(glEnableVertexAttribArray(1));
    GLCall(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *)offsetof(Vertex, Color)));
    GLCall(glEnableVertexAttribArray(2));
    GLCall(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *)offsetof(Vertex, TexCoords)));
    GLCall(glEnableVertexAttribArray(3));
    GLCall(glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *)offsetof(Vertex, TexID)));

    GLCall(glGenBuffers(1, &m_QuadIB));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_QuadIB));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW)); // upload data to the buffer. Can be done whenever (but the  buffer has to be bound)

    m_Shader = std::make_unique<Shader>("res/shaders/BatchTexture.vert", "res/shaders/BatchTexture.frag");
    // with shaders in one file
    // Shader shader{"res/shaders/Basic.glsl"};
    m_Shader->Bind();

    // load two textures, bind them each to separate slot
    m_TexturePizza = std::make_unique<Texture>("res/textures/pizza.png");
    m_TexturePizza->Bind(0);
    m_TextureBaguette = std::make_unique<Texture>("res/textures/bageta.png");
    m_TextureBaguette->Bind(1);
  }

  SceneBatchDynamic::~SceneBatchDynamic()
  {
  }

  void SceneBatchDynamic::OnUpdate(float deltaTime)
  {
  }

  void SceneBatchDynamic::OnRender(Renderer renderer)
  {
    GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
    GLCall(glClear(GL_COLOR_BUFFER_BIT));

    // Set dynamic vertex buffer data
    auto q0 = CreateQuad(m_QuadPos[0] * 50, m_QuadPos[1] * 50, 0);
    auto q1 = CreateQuad(200, -50, 1);
    Vertex vertices[8];
    memcpy(
        vertices, q0.data(), q0.size() * sizeof(Vertex));
    memcpy(
        vertices + q0.size(), q1.data(), q1.size() * sizeof(Vertex));

    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_QuadVB));
    GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices));

    // set dynamic vertex buffer
    m_Shader->Bind();
    m_TexturePizza->Bind(0);
    m_TextureBaguette->Bind(1);

    // move object based on translation value
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), m_Translation);

    /*
    Multiplication order matters! in OpenGL we work with column major,
    Direct3D and other are row major, we would multiply in reverse order modelMatrix * m_ViewMatrix * m_ProjectionMatrix
    */
    glm::mat4 mvpMatrix = m_ProjectionMatrix * m_ViewMatrix * modelMatrix;

    m_Shader->SetUniformMat4f("u_MVP", mvpMatrix);
    int samplers[2] = {0, 1};
    m_Shader->SetUniform1iv("u_Textures", 2, samplers);

    GLCall(glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, nullptr));
  }

  void SceneBatchDynamic::OnImGuiRender()
  {
    ImGui::DragFloat2("Quad Position", m_QuadPos, 0.1f);
  }
}
