#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 uv;
out vec2 TexCoord;

uniform mat4 view;

void main() {
  uv = vec2(aPos.x, aPos.y);
  TexCoord = aTexCoord;
  
  gl_Position = view * vec4(aPos, 1.0f);
}
