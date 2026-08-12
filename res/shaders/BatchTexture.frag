#version 330 core

layout(location = 0) out vec4 o_Color;

// receive data from vertex shader
in vec2 v_TexCoord;
in float v_TexIndex;

uniform sampler2D u_Textures[2];

void main() {
  int index = int(v_TexIndex);
  o_Color = texture(u_Textures[index], v_TexCoord);
}