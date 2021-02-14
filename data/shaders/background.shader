#ifdef VERTEX_PROGRAM

layout (location = 0) in vec3 aPos;

//out vec2 uv;
uniform mat4 view;
uniform mat4 model;

void main() {
  //uv = vec2(aPos.x, aPos.y);

  //NOTE: does not use view or model currently
  gl_Position = vec4(aPos, 1.0f);
}


#endif

#ifdef FRAGMENT_PROGRAM


out vec4 FragColor;

void main() {

   FragColor = vec4(0.192 * .2  ,0.663 *.2 ,0.722 * .3, 1.0);
  
}


#endif
