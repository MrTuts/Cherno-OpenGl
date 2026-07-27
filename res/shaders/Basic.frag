#version 330 core

layout(location = 0) out vec4 color;

uniform vec4 u_Color; // uniform values are prefixed with u_

void main() {
  color = u_Color;
}