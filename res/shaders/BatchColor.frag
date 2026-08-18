#version 410 core

layout(location = 0) out vec4 o_Color;

// receive data from vertex shader
in vec4 v_Color;

void main() {
  o_Color = v_Color;
}