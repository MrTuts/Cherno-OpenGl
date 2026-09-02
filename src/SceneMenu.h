#pragma once

#include <iostream>
#include <vector>
#include <functional>
#include "Scene.h"

namespace scene
{
  struct SceneMenuScene
  {
    std::string name;
    std::string parent;
    std::function<Scene *()> create;
  };

  class SceneMenu : public Scene
  {
  public:
    SceneMenu(Scene *&currentScenePtr);

    void OnRender(GLFWwindow *window) override;
    void OnImGuiRender() override;

    template <typename T>
    void RegisterScene(const std::string &name, const std::string &parent)
    {
      std::cout << "Registering scene " << name << std::endl;
      m_Scenes.push_back({name, parent, []()
                          { return new T(); }});
      // m_Scenes.push_back(std::make_pair(name, []()
      //                                   { return new T(); }));
    }

    template <typename T>
    void RegisterChernoScene(const std::string &name)
    {
      RegisterScene<T>(name, "Cherno");
    }

    template <typename T>
    void RegisterJKingScene(const std::string &name)
    {
      RegisterScene<T>(name, "JKing");
    }

  private:
    Scene *&m_CurrentScene;
    std::vector<SceneMenuScene> m_Scenes;
  };
}