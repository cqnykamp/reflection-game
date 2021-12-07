#ifdef VERTEX_PROGRAM

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

//out vec2 uv;
uniform mat4 view;
uniform mat4 model;

void main() {

  //NOTE: does not use view or model currently
  gl_Position = vec4(aPos, 1.0f);
  TexCoord = aTexCoord;
}


#endif

#ifdef FRAGMENT_PROGRAM

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D ourTexture;

void main() {

  FragColor = texture(ourTexture, TexCoord);
  // FragColor = vec4(0.192 * .2  ,0.663 *.2 ,0.722 * .3, 1.0);

}


#endif
