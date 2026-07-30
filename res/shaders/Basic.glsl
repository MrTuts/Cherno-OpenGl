#shader vertex
#version 330 core

layout(location = 0) in vec4 position;  // location 0 corelates with 0 at glVertexAttribPointer(0,...)

void main() {
	gl_Position = position;
}

#shader fragment
#version 330 core

layout(location = 0) out vec4 color;

uniform vec4 u_Color;

void main() {
	color = u_Color;
}