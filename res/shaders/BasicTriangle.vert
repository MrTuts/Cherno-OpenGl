#version 410 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec4 a_color;

out vec4 v_Color;
uniform double u_elapsedTime;

void main() {
  gl_Position = vec4(a_position, 1.0, 1.0);
  // v_Color = a_color;

  double t = mod(u_elapsedTime, 3.0);

  int index = int(floor(t));
  int nextIndex = (index + 1) % 3;

  double progress = smoothstep(0.0, 1.0, fract(t));

  mat3 colors = mat3(a_color.rgb,  // RGB
  a_color.brg,  // BRG
  a_color.gbr   // GBR
  );

  vec3 animatedColor = mix(colors[index], colors[nextIndex], float(progress));

  v_Color = vec4(animatedColor, a_color.a);
}