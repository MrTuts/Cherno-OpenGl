#include "Scene.h"

#include <imgui.h>

namespace scene
{

  SceneMenu::SceneMenu(Scene *&currentScenePtr) : m_CurrentScene(currentScenePtr)
  {
  }

  void SceneMenu::OnRender(Renderer renderer)
  {
    GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
  }

  void SceneMenu::OnImGuiRender()
  {
    for (auto &scene : m_Scenes)
    {
      if (ImGui::Button(scene.first.c_str()))
      {
        m_CurrentScene = scene.second();
      }
    }
  }
}