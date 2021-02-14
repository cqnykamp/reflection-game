#ifdef VERTEX_PROGRAM

layout (location = 0) in vec3 aPos;

out vec2 uv;

uniform mat4 model;
//uniform mat3 basis;
uniform mat4 view;


void main() {
  uv = vec2(aPos.x, aPos.y);

  vec4 pos_orig = model * vec4(aPos.x, aPos.y, 0.0f, 1.0f);
  pos_orig.z = 1.0f; // Tiled coord system assumes level on z=1

  vec3 pos = pos_orig.xyz;
  //vec3 pos = basis * pos_orig.xyz;
  
  gl_Position = view * vec4(pos.x, pos.y, 0.0f, 1.0f);

}


#endif

#ifdef FRAGMENT_PROGRAM

out vec4 FragColor;

in vec2 uv;


void main() {

  float tiled_diag = 6 * (uv.x + uv.y);
  int diag_pos = int(floor(tiled_diag));

  /**
  if(diag_pos % 2 == 0) {
    FragColor = vec4(1.0f, 0.9f, 0.0f, 0.6f);
  } else {
    FragColor = vec4(1.0f, 1.0f, 0.0f, 0.6f);
  }
  **/

  //FragColor = vec4(0.965f, 0.165f, 0.0f, 1.0f);

  

  //  FragColor = vec4(0.192 ,0.663 ,0.722, 1.0); // light blue
  FragColor = vec4(.902, 0., .702 , 1.);
  FragColor = vec4(uv.x, uv.y, 0., 1.);


    //  FragColor = vec4((uv.x + uv.y) / 2, 0.0f, 0.0f, 1.0f);
}

#endif
