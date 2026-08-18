#version 410 core

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec4 a_Color;

// output data from vertex shader to fragment shader
// v stands for varying
out vec4 v_Color;

uniform mat4 u_MVP; // model view projection matrix

void main() {
  gl_Position = u_MVP * vec4(a_Position, 1.0, 1.0);
  v_Color = a_Color;
}