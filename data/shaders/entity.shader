#ifdef VERTEX_PROGRAM

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;

out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;

void main() {
  gl_Position = view * model * vec4(aPos, 1.0);
  TexCoord = aTex;

}


#endif

#ifdef FRAGMENT_PROGRAM


out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D ourTexture;

void main() {

  FragColor = texture(ourTexture, TexCoord);
  // FragColor = vec4(TexCoord.x, TexCoord.y, 0, 1);
		
}

#endif