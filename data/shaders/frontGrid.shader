#ifdef VERTEX_PROGRAM

layout (location = 0) in vec3 aPos;

out vec2 uvRaw;
//out vec3 pos;

uniform mat4 view;
uniform mat4 model;
//uniform int xcoord;
//uniform int ycoord;
//uniform mat3 basis;

//uniform int board[36];


void main() {
  uvRaw = vec2(aPos.x, aPos.y);
  //pos = basis * vec3(aPos.x + xcoord, aPos.y + ycoord, 1.0f);
  //pos = model * vec4(aPos, 1.0f);


  /*
  on_tile = 0.0f;  
  if( pos.x >= 0 && pos.x <= 5 && pos.y >= 0 && pos.y <= 5 ) {
    if ( board[int( pos.y * 6 + pos.x )] != 0
	 //	 || board[int( (pos.y - 1)*6 + pos.x )] != 0
	 //	 || board[int( (pos.y - 1)*6 + pos.x-1 )] != 0
	 //	 || board[int( pos.y*6 + pos.x-1 )] != 0) {
	 ) {
      on_tile = 1.0f;
    }
  }
  */

  gl_Position = view * model * vec4(aPos, 1.0f);
  //  gl_Position = vec4(aPos, 1.f);
}


#endif

#ifdef FRAGMENT_PROGRAM


out vec4 FragColor;

in vec2 uvRaw;
//in vec3 pos;


//uniform float tileLength;
//uniform int level_width;
//uniform int level_height;


void main() {

  float tileLength = 1.0f;
  int level_width = 100;
  int level_height = 100;
  

  float fade_length = 3.5;
  float fade_offset = 0.;

  /**
  // Discard if off the board and past fade-out
  if(pos.x < -fade_length-fade_offset
     || pos.y < -fade_length-fade_offset
     || pos.x > level_width+fade_length+fade_offset
     || pos.y > level_height+fade_length+fade_offset) {
    discard;
  }
  **/

  vec2 uv = fract(uvRaw);

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


  /**
  //NOTE: not using fade right now bc it looks better without it
  float fade_on_right = 1. - max( (pos.x - level_width-fade_offset)/fade_length , 0);
  float fade_on_top  = 1. - max( (pos.y - level_height-fade_offset)/fade_length , 0);
  float fade_on_left = 1. - max(-fade_offset-pos.x / fade_length , 0);
  float fade_on_bottom = 1. - max(-fade_offset-pos.y / fade_length, 0);

  float fade = fade_on_right * fade_on_top * fade_on_left * fade_on_bottom;
  **/


  
  FragColor = vec4(1.f, 1.f, 1.f, 0.1f);
  //  FragColor = vec4(pos.x, pos.y, 0., 1.);

  /**
  float corner = step(1.0-thickness, uv.x) * step(1.0-thickness, uv.y)
    + step(1.0-thickness, 1.0-uv.x) * step(1.0-thickness, uv.y)
    + step(1.0-thickness, 1.0-uv.x) * step(1.0-thickness, 1.0-uv.y)
    + step(1.0-thickness, uv.x) * step(1.0-thickness, 1.0-uv.y);
  corner = min(corner, 1.0f);
  **/


  


  
}

#endif
