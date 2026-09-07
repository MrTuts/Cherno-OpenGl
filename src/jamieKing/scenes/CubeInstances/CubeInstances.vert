#version 410 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;
layout(location = 2) in mat4 fullTransformMatrix; // 16 floats

out vec4 v_Color;

void main() {
  gl_Position = fullTransformMatrix * vec4(a_position, 1.0);
  v_Color = a_color;
}