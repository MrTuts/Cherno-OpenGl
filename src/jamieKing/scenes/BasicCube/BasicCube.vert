#version 410 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;

uniform vec3 dominatingColor;

out vec4 v_Color;

void main() {
  gl_Position = vec4(a_position, 1.0);
  v_Color = vec4(dominatingColor, 1.0);
}