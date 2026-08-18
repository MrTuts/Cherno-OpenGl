#pragma once

#include <iostream>
#include <vector>
#include <functional>
#include "Scene.h"

namespace scene
{
  class SceneMenu : public Scene
  {
  public:
    SceneMenu(Scene *&currentScenePtr);

    void OnRender() override;
    void OnImGuiRender() override;

    template <typename T>
    void RegisterScene(const std::string &name)
    {
      std::cout << "Registering scene " << name << std::endl;
      m_Scenes.push_back(std::make_pair(name, []()
                                        { return new T(); }));
    }

  private:
    Scene *&m_CurrentScene;
    std::vector<std::pair<std::string, std::function<Scene *()>>> m_Scenes;
  };
}