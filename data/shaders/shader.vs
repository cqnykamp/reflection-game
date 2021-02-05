#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;
out vec2 uv;

uniform mat4 model;
uniform mat4 view;

void main() {
  TexCoord = aTexCoord;
  uv = vec2(aPos.x, aPos.y);
  gl_Position = view * model  * vec4(aPos, 1.0);

}
