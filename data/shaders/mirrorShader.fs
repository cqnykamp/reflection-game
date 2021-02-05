#version 330 core

out vec4 FragColor;

in vec2 uv;

uniform bool highlighted;

void main() {
  vec3 bordercolor = vec3(0.7, 0.7f, 0.7f);

  if(highlighted) {
    FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
  } else {
    FragColor = vec4(bordercolor, 1.0f);
  }
}
