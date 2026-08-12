#version 330 core

layout(location = 0) out vec4 o_Color;

// receive data from vertex shader
in vec2 v_TexCoord;
in float v_TexIndex;

uniform sampler2D u_Textures[2];

void main() {
  int index = int(v_TexIndex);

  // this would be optimal code (no branching), but only works on OpenGL 4+
  // o_Color = texture(u_Textures[index], v_TexCoord); 

  // ugly solution with code branching, which slows down the execution
  if(index == 0)
    o_Color = texture(u_Textures[0], v_TexCoord);
  else if(index == 1)
    o_Color = texture(u_Textures[1], v_TexCoord);
}