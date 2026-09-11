#include "Camera.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/transform.hpp"
#include "glm/gtx/euler_angles.hpp"

Camera::Camera() : m_ViewDirection(0.0f, 0.0f, -1.0f),
                   m_Position(0.0f, 0.0f, 0.0f),
                   UP(0.0f, 1.0f, 0.0f),
                   ROTATE_SPEED(0.3f),
                   MOVE_SPEED(0.1f),
                   ZOOM_SPEED(0.1f)
{
  m_StrafeDirection = glm::cross(m_ViewDirection, UP);
}

glm::mat4 Camera::getWorldToViewMatrix() const
{
  return glm::lookAt(m_Position, m_Position + m_ViewDirection, UP);
}

void Camera::mouseRotateStart(const glm::vec2 &newMousePosition)
{
  m_OldMouseRotatePosition = newMousePosition;
}

void Camera::mouseRotateUpdate(const glm::vec2 &newMousePosition)
{
  glm::vec2 mouseDelta = newMousePosition - m_OldMouseRotatePosition;
  // rotate along Y (UP) axis
  m_ViewDirection = glm::mat3(glm::rotate(glm::radians(-mouseDelta.x * ROTATE_SPEED), UP)) * m_ViewDirection;
  // rotate along X' axis
  // cross product gives us vector perpendicular to m_ViewDirection and UP
  m_StrafeDirection = glm::cross(m_ViewDirection, UP);
  m_ViewDirection = glm::mat3(glm::rotate(glm::radians(-mouseDelta.y * ROTATE_SPEED), m_StrafeDirection)) * m_ViewDirection;

  m_OldMouseRotatePosition = newMousePosition;
}

void Camera::mouseMoveStart(const glm::vec2 &newMousePosition)
{
  m_OldMouseMovePosition = newMousePosition;
}

void Camera::mouseMoveUpdate(const glm::vec2 &newMousePosition)
{
  glm::vec2 mouseDelta = (newMousePosition - m_OldMouseMovePosition) * MOVE_SPEED;

  // move along Y (UP) axis
  m_Position += mouseDelta.y * UP;
  // move along X' axis
  glm::vec3 strafeDirection = glm::cross(m_ViewDirection, UP);
  m_Position -= mouseDelta.x * strafeDirection;

  m_OldMouseMovePosition = newMousePosition;
}

void Camera::mouseZoom(const float scrollOffset)
{
  m_Position += ZOOM_SPEED * -scrollOffset * m_ViewDirection;
}

void Camera::moveForward()
{
  m_Position += MOVE_SPEED * m_ViewDirection;
}

void Camera::moveBackward()
{
  m_Position -= MOVE_SPEED * m_ViewDirection;
}

void Camera::strafeLeft()
{
  m_Position -= MOVE_SPEED * m_StrafeDirection;
}

void Camera::strafeRight()
{
  m_Position += MOVE_SPEED * m_StrafeDirection;
}

void Camera::moveUp()
{
  m_Position += MOVE_SPEED * UP;
}

void Camera::moveDowns()
{
  m_Position -= MOVE_SPEED * UP;
}
