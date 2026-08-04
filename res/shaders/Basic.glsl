#shader vertex
#version 330 core

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

#shader fragment
#version 330 core

layout(location = 0) out vec4 color;

// receive data from vertex shader
in vec2 v_TexCoord;

// inputs controlled by our CPU code
uniform vec4 u_Color;
uniform sampler2D u_Texture;

void main() {
	vec4 texColor = texture(u_Texture, v_TexCoord);
	color = texColor;
}