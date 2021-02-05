#version 330 core

out vec4 FragColor;

in vec2 TexCoord;
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


    //  FragColor = vec4((uv.x + uv.y) / 2, 0.0f, 0.0f, 1.0f);
}
