#version 410 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in float offset;

out vec4 v_Color;

void main() {
  gl_Position = vec4(a_position.x + offset, a_position.y, 0.0, 1.0);
  v_Color = vec4(1.0, 0.0, 0.0, 1.0);
}