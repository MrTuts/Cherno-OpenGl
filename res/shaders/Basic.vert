#version 330 core

layout(location = 0) in vec4 position;  // location 0 corelates with 0 at glVertexAttribPointer(0,...)

void main() {
  gl_Position = position;
}