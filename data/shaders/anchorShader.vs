#version 330 core

layout (location = 0) in vec3 aPos;

out vec2 uv;

uniform mat4 model;
uniform mat4 view;
uniform int highlight_key;
uniform float scale;

void main() {
  uv = vec2(aPos.x, aPos.y);

  vec3 pos;
  if(highlight_key == 2) {
    pos = 0.13 * aPos;
    
  } else if(highlight_key == 1) {
    pos = 0.11 * aPos;
    
  } else {
    pos = 0.1 * aPos;
  }
  
  gl_Position = view * model * vec4(pos, 1.0f);
}
