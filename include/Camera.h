#pragma once

#include "glm/glm.hpp"

class Camera
{
private:
  glm::vec3 m_Position;
  glm::vec3 m_ViewDirection;
  glm::vec2 m_OldMouseRotatePosition;
  glm::vec2 m_OldMouseMovePosition;
  glm::vec3 m_StrafeDirection;
  const glm::vec3 UP;
  const float ROTATE_SPEED;
  const float MOVE_SPEED;
  const float ZOOM_SPEED;

public:
  Camera();

  glm::mat4 getWorldToViewMatrix() const;
  void mouseRotateStart(const glm::vec2 &newMousePosition);
  void mouseRotateUpdate(const glm::vec2 &newMousePosition);
  void mouseMoveStart(const glm::vec2 &newMousePosition);
  void mouseMoveUpdate(const glm::vec2 &newMousePosition);
  void mouseZoom(const float scrollOffset);
  void moveForward();
  void moveBackward();
  void strafeLeft();
  void strafeRight();
  void moveUp();
  void moveDowns();
};