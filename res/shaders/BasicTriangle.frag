#version 410 core

layout(location = 0) out vec4 o_color;

in vec4 v_Color;

uniform double u_elapsedTime;

void main() {
  double integerPart = floor(u_elapsedTime);
  double fractionPart = fract(u_elapsedTime);
  double clampedIntegerPart = mod(integerPart, 3);

  // o_color = vec4(v_Color.rgb * fract(u_elapsedTime), v_Color.a);
  o_color = v_Color;
}