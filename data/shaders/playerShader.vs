#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 uv;
out vec2 TexCoord;

uniform mat4 model;
uniform mat3 basis;
uniform mat4 view;


void main() {
  TexCoord = aTexCoord;
  uv = vec2(aPos.x, aPos.y);

  vec4 pos_orig = model * vec4(aPos.x, aPos.y, 0.0f, 1.0f);
  pos_orig.z = 1.0f; // Tiled coord system assumes level on z=1
  vec3 pos = basis * pos_orig.xyz;
  
  gl_Position = view * vec4(pos.x, pos.y, 0.0f, 1.0f);

}
