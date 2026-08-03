#version 330 core

layout(location = 0) in vec4 position;  // location 0 corelates with 0 at glVertexAttribPointer(0,...)
layout(location = 1) in vec2 texCoord;  // location 1 corelates with 1 at glVertexAttribPointer(1,...)

// output data from vertex shader to fragment shader
// v stands for varying
out vec2 v_TexCoord;

void main() {
  gl_Position = position;
  v_TexCoord = texCoord;
}