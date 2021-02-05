#version 330 core

layout (location = 0) in vec3 aPos;

out vec2 uv;

uniform mat4 view;
uniform mat4 model;

void main() {
  uv = vec2(aPos.x, aPos.y);
  gl_Position = view * model * vec4(aPos, 1.0f);
}
