#version 410 core

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in float a_TexIndex;

// output data from vertex shader to fragment shader
// v stands for varying
out vec4 v_Color;
out vec2 v_TexCoord;
out float v_TexIndex;

uniform mat4 u_MVP; // model view projection matrix

void main() {
  gl_Position = u_MVP * vec4(a_Position, 1.0, 1.0);
  v_Color = a_Color;
  v_TexCoord = a_TexCoord;
  v_TexIndex = a_TexIndex;
}