#ifdef VERTEX_PROGRAM


layout (location = 0) in vec3 aPos;

out vec2 uv;

uniform mat4 view;
uniform mat4 model;

void main() {
  uv = vec2(aPos.x, aPos.y);
  gl_Position = view * model * vec4(aPos, 1.0f);
}


#endif

#ifdef FRAGMENT_PROGRAM


out vec4 FragColor;

in vec2 uv;

// uniform bool highlighted;

void main() {
  bool highlighted = true;
  
  vec3 bordercolor = vec3(0.7, 0.7f, 0.7f);

  if(highlighted) {
    FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
  } else {
    FragColor = vec4(bordercolor, 1.0f);
  }
}


#endif
