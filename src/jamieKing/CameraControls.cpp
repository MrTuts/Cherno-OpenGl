#include "CameraControls.h"
#include <imgui.h>
#include "backends/imgui_impl_glfw.h"

CameraControls::CameraControls(GLFWwindow *window, Camera &camera) : m_Window(window), m_Camera(camera)
{
}

void CameraControls::Setup()
{
  if (glfwRawMouseMotionSupported())
  {
    glfwSetInputMode(m_Window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
  }

  glfwSetWindowUserPointer(m_Window, this);
  glfwSetScrollCallback(m_Window, ScrollCallback);
}

CameraControls::~CameraControls()
{
  glfwSetInputMode(m_Window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
  if (glfwGetWindowUserPointer(m_Window) == this)
  {
    glfwSetWindowUserPointer(m_Window, nullptr);
  }
  glfwSetScrollCallback(m_Window, ImGui_ImplGlfw_ScrollCallback);
}

void CameraControls::ScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
  ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);

  CameraControls *cameraControls = static_cast<CameraControls *>(glfwGetWindowUserPointer(window));
  if (cameraControls)
  {
    cameraControls->OnScroll(xoffset, yoffset);
  }
}

void CameraControls::OnScroll(double xoffset, double yoffset)
{
  ImGuiIO &io = ImGui::GetIO();
  if (io.WantCaptureMouse)
  {
    return;
  }
  m_Camera.mouseZoom(yoffset);
}

void CameraControls::OnUpdate(float deltaTime)
{
  ImGuiIO &io = ImGui::GetIO();

  // If the mouse is over an ImGui window, let ImGui have it and return early
  if (io.WantCaptureMouse)
  {
    return;
  }

  static bool rotating = false;
  static bool moving = false;
  static glm::vec2 lbStartPos;
  static glm::vec2 rbStartPos;

  double mouseXPos, mouseYPos;
  glfwGetCursorPos(m_Window, &mouseXPos, &mouseYPos);
  glm::vec2 mousePos = glm::vec2(mouseXPos, mouseYPos);

  int lbState = glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_LEFT);
  int rbState = glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_RIGHT);
  if (lbState == GLFW_PRESS && rbState == GLFW_PRESS)
  {
    // move
    if (!moving)
    {
      m_Camera.mouseMoveStart(mousePos);
    }
    moving = true;
    m_Camera.mouseMoveUpdate(mousePos);
  }
  else
  {
    moving = false;
  }

  if (!moving && rbState == GLFW_PRESS)
  {
    if (!rotating)
    {
      m_Camera.mouseRotateStart(mousePos);
    }
    rotating = true;
    m_Camera.mouseRotateUpdate(mousePos);
  }
  else
  {
    rotating = false;
  }

  if (glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_PRESS)
  {
    m_Camera.moveForward();
  }
  if (glfwGetKey(m_Window, GLFW_KEY_S) == GLFW_PRESS)
  {
    m_Camera.moveBackward();
  }
  if (glfwGetKey(m_Window, GLFW_KEY_A) == GLFW_PRESS)
  {
    m_Camera.strafeLeft();
  }
  if (glfwGetKey(m_Window, GLFW_KEY_D) == GLFW_PRESS)
  {
    m_Camera.strafeRight();
  }
}
