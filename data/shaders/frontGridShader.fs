#version 330 core

out vec4 FragColor;

in vec2 uv;
in vec3 pos;
in float on_tile;


uniform float tileLength;
uniform mat4 view_inverse;
uniform mat3 basis;

uniform int level_width;
uniform int level_height;


void main() {

  float fade_length = 3.5;
  float fade_offset = 0.;

  // Discard if off the board and past fade-out
  if(pos.x < -fade_length-fade_offset
     || pos.y < -fade_length-fade_offset
     || pos.x > level_width+fade_length+fade_offset
     || pos.y > level_height+fade_length+fade_offset) {
    discard;
  }


  float borderwidth = 0.05;
  float not_border = (1.0-step(1.-borderwidth, uv.x))
    * (1.0-step(1.-borderwidth, uv.y))
    * (1.0-step(1.-borderwidth, 1.0-uv.x))
    * (1.0-step(1.-borderwidth, 1.0-uv.y));

  float diagwidth = 0.05;
  float not_diag = step(diagwidth, abs(uv.x-uv.y))
    * step(diagwidth, abs(uv.x + uv.y - 1.));


  if(not_border ==1. && not_diag==1.) {
    discard;
  }

  
  //NOTE: not using fade right now bc it looks better without it
  float fade_on_right = 1. - max( (pos.x - level_width-fade_offset)/fade_length , 0);
  float fade_on_top  = 1. - max( (pos.y - level_height-fade_offset)/fade_length , 0);
  float fade_on_left = 1. - max(-fade_offset-pos.x / fade_length , 0);
  float fade_on_bottom = 1. - max(-fade_offset-pos.y / fade_length, 0);

  float fade = fade_on_right * fade_on_top * fade_on_left * fade_on_bottom;



  
  FragColor = vec4(1., 1., 1., .1);
  //  FragColor = vec4(pos.x, pos.y, 0., 1.);

  /**
  float corner = step(1.0-thickness, uv.x) * step(1.0-thickness, uv.y)
    + step(1.0-thickness, 1.0-uv.x) * step(1.0-thickness, uv.y)
    + step(1.0-thickness, 1.0-uv.x) * step(1.0-thickness, 1.0-uv.y)
    + step(1.0-thickness, uv.x) * step(1.0-thickness, 1.0-uv.y);
  corner = min(corner, 1.0f);
  **/


  


  
}
