#include "SceneMenu.h"
#include "Renderer.h"

#include <algorithm>
#include <imgui.h>

namespace scene
{

  SceneMenu::SceneMenu(Scene *&currentScenePtr) : m_CurrentScene(currentScenePtr)
  {
  }

  void SceneMenu::OnRender(GLFWwindow *window)
  {
    GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
  }

  void SceneMenu::OnImGuiRender()
  {
    if (m_Scenes.empty())
    {
      return;
    }
    std::sort(
        m_Scenes.begin(),
        m_Scenes.end(),
        [](const SceneMenuScene &a, const SceneMenuScene &b)
        { return a.parent < b.parent; });
    std::string currentParent;

    for (auto &scene : m_Scenes)
    {
      if (currentParent.compare(scene.parent) != 0)
      {
        currentParent = scene.parent;
        ImGui::TextUnformatted(currentParent.c_str());
      }
      std::string name = scene.parent + ": " + scene.name;
      if (ImGui::Button(name.c_str()))
      {
        m_CurrentScene = scene.create();
      }
    }
  }
}