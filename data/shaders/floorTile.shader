#ifdef VERTEX_PROGRAM


layout (location = 0) in vec3 aPos;

out vec2 uv;

uniform mat4 model;
uniform mat4 view;

void main() {
  uv = vec2(aPos.x, aPos.y);
  gl_Position = view * model * vec4(aPos, 1.0);

}


#endif

#ifdef FRAGMENT_PROGRAM


out vec4 FragColor;

in vec2 uv;

uniform int highlight_key;

int squares_per_size = 4;

void main() {
  
  vec4 uvColor = vec4(uv, 0.0f, 1.0f);

  vec2 tiled_uv = squares_per_size * uv;
  ivec2 tile_pos = ivec2(floor(tiled_uv));

  float borderwidth = 0.05;
  float not_border = (1.0-step(1.-borderwidth, uv.x))
    * (1.0-step(1.-borderwidth, uv.y))
    * (1.0-step(1.-borderwidth, 1.0-uv.x))
    * (1.0-step(1.-borderwidth, 1.0-uv.y));

  float diagwidth = 0.03;
  float not_diag = step(diagwidth, abs(uv.x-uv.y))
    * step(diagwidth, abs(uv.x + uv.y - 1.));

  
  if(highlight_key == 1) { // is goal
   
    //    FragColor = vec4(0.961,0.745,0.255, 1.0f); //gold
    FragColor = vec4(0.145 *1.5 ,0.502 * 1.5 ,0.224 * 1.5, 1.0); //green
    
    /**
    if( (tile_pos.x+tile_pos.y) % 2 == 0) {
      FragColor = vec4(0.2f, 0.7f, 0.5f, 1.0f);
    } else {
      FragColor = vec4(0.2f, 0.3f, 0.6f, 1.0f);
    }
    **/


  } else {  

    /**
    if( (tile_pos.x+tile_pos.y) % 2 == 0) {
      FragColor = vec4(0.1f, 0.2f, 0.3f, 1.0f);
    } else {
      FragColor = vec4(0.2f, 0.3f, 0.4f, 1.0f);
    }
    **/
    FragColor = vec4(0.192 * .6  ,0.663 *.6 ,0.722 * .75, 1.0);

  }
		
}

#endif
