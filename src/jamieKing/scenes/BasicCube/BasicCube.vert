#version 410 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;

uniform mat4 fullTransformMatrix;

out vec4 v_Color;

void main() {
  gl_Position = fullTransformMatrix * vec4(a_position, 1.0);
  v_Color = a_color;
}