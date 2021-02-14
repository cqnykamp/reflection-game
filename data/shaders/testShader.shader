#ifdef VERTEX_PROGRAM

layout (location = 0) in vec3 aPos;

uniform mat4 model;

void main() {

  gl_Position = vec4(aPos, 1.0f);
  gl_Position = model * vec4(aPos, 1.0f);

}


#endif

#ifdef FRAGMENT_PROGRAM

out vec4 FragColor;

void main() {
  FragColor = vec4(0., 0., 1., 1.);

}

#endif
