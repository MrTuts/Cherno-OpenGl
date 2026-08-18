#version 410 core

layout(location = 0) in vec4 position;  // location 0 corelates with 0 at glVertexAttribPointer(0,...)
layout(location = 1) in vec2 texCoord;  // location 1 corelates with 1 at glVertexAttribPointer(1,...)

// output data from vertex shader to fragment shader
// v stands for varying
out vec2 v_TexCoord;

uniform mat4 u_MVP; // model view projection matrix

void main() {
  gl_Position = u_MVP * position;
  v_TexCoord = texCoord;
}