#ifdef VERTEX_PROGRAM

layout (location = 0) in vec4 vertex; //2 for pos, 2 for tex

out vec2 texCoords;

uniform mat4 projection;

void main() {
  gl_Position = projection * vec4(vertex.xy, 0.f, 1.f);
  texCoords = vertex.zw;
}

#endif


#ifdef FRAGMENT_PROGRAM

in vec2 texCoords;
out vec4 color;

uniform sampler2D text;
uniform vec3 textColor;

void main() {
  vec4 sampled = vec4(1.f, 1.f, 1.f, texture(text, texCoords).r);
  color = vec4(textColor, 1.f) * sampled;
  //color = vec4(1.f, 0.f, 0.f, 1.f);
}


#endif
