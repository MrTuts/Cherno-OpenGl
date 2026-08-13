#include "ScenePizza.h"
#include <imgui.h>
#include "glm/gtc/matrix_transform.hpp"

// clang-format off
    /* 
      The rectangle we want to draw
      3 ---- 2
      |    |
      |    |
      0 ---- 1
*/
const float vertices[] = {
    -50.0f, -50.0f, 0.0f, 0.0f, // index 0
    50.0f, -50.0f, 1.0f, 0.0f, // index 1
    50.0f,  50.0f, 1.0f, 1.0f,  // index 2
    -50.0f,  50.0f, 0.0f, 1.0f  // index 3
};
// indices can be chars or shorts, but MUST be unsigned
const unsigned int indices[] = {
  0,1,2, // bottom right triangle
  2,3,0  // top left triangle
};
// clang-format on

namespace scene
{

  ScenePizza::ScenePizza() : va(VertexArray()),
                             translationA(glm::vec3(200.0f, 200.0f, 0.0f)),
                             translationB(glm::vec3(400.0f, 200.0f, 0.0f)),
                             shader(Shader{"res/shaders/Basic.vert", "res/shaders/Basic.frag"}),
                             // 6 = 6 indices
                             ib(IndexBuffer{indices, 6}),
                             vb(VertexBuffer(vertices, 4 * 4 * sizeof(float))),
                             texture(Texture("res/textures/pizza.png"))
  {

    // When using core profile, we need to create vertex array buffer
    // With compatibility profile, there is one vao created that stores everything. Since it stores everything,
    // all attrib layouts need to be re-specified every time together with the array buffer
    // --
    // This allows us to bind the array buffer and set a layout to it (vertex attributes)
    // VertexBuffer vb{vertices, 4 * 4 * sizeof(float)};

    VertexBufferLayout layout;
    layout.Push<float>(2); // locations
    layout.Push<float>(2); // texture coords
    va.Addbuffer(vb, layout);

    // with shaders in one file
    // Shader shader{"res/shaders/Basic.glsl"};
    shader.Bind();
    shader.SetUniform4f("u_Color", 0.8f, 0.3f, 0.8f, 1.0f);

    texture.Bind();                      // by default binds at texture slot 0
    shader.SetUniform1i("u_Texture", 0); // 0 is the slot this texture is bound to

    /* MVP
      view matrix - matrix for the view of the camera, position, scale, other.
      model matrix - matrix for the vertex we are drawing; transformation of the model
      projection matrix - converting the space we work with (e.g. 0, 640) into (-1, 1)
    */
    // orthographic projection - things do not get smaller with their distance to the camera (no perspective)
    projectionMatrix = glm::ortho(0.0f, 640.0f, 0.0f, 480.0f, -1.0f, 1.0f);
    viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
  }

  ScenePizza::~ScenePizza()
  {
  }

  void ScenePizza::OnUpdate(float deltaTime)
  {
  }

  void ScenePizza::OnRender(Renderer renderer)
  {
    texture.Bind();
    /* Rendering multiple objects
        Here we render the same object twice by changing the uniform, we call the Draw function twice. This is good for e.g. rendering
        3D objects.
        Another method is batch rendering, where instead we pass all the vertices to GPU and render it in only one Draw function.
        This batch rendering is better for e.g. rendering many tiles on screen, rendering text
      */
    {
      // move object based on translation value
      glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), translationA);
      /*
      Multiplication order matters! in OpenGL we work with column major,
      Direct3D and other are row major, we would multiply in reverse order modelMatrix * viewMatrix * projectionMatrix
      */
      glm::mat4 mvpMatrix = projectionMatrix * viewMatrix * modelMatrix;
      shader.Bind();
      shader.SetUniformMat4f("u_MVP", mvpMatrix);
      renderer.Draw(va, ib, shader);
    }

    /* Rendering same object second time with different model matrix */
    {
      glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), translationB);
      glm::mat4 mvpMatrix = projectionMatrix * viewMatrix * modelMatrix;
      shader.Bind();
      shader.SetUniformMat4f("u_MVP", mvpMatrix);
      renderer.Draw(va, ib, shader);
    }
  }

  void ScenePizza::OnImGuiRender()
  {
    ImGui::SliderFloat3("Translation A", &translationA.x, 0.0f, 640.0f);
    ImGui::SliderFloat3("Translation B", &translationB.x, 0.0f, 640.0f);
  }
}
