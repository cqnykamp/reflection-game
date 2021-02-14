#ifdef VERTEX_PROGRAM

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 uv;
//out vec2 TexCoord;

uniform mat4 view;

void main() {
  uv = vec2(aPos.x, aPos.y);
  //TexCoord = aTexCoord;
  
  gl_Position = view * vec4(aPos, 1.0f);
}




#endif

#ifdef FRAGMENT_PROGRAM


out vec4 FragColor;

in vec2 uv;
//in vec2 TexCoord;

uniform float tileLength;
uniform sampler2D ourTexture;
uniform float time;

int subdivs_per_tile = 6;

void main() {
  //vec2 tileduv = fract(uv / vec2(tileLength));
  vec2 tileduv = fract(uv);

  vec2 outer_pos = floor(uv / vec2(tileLength));
  ivec2 inner_pos = ivec2(floor(subdivs_per_tile * tileduv));

  float not_border = (1.0-step(0.99f, tileduv.x))
    * (1.0-step(0.99, tileduv.y))
    * (1.0-step(0.99, 1.0-tileduv.x))
    * (1.0-step(0.99, 1.0-tileduv.y));

  vec3 bordercolor = vec3(0.5, 0.5f, 0.5f);
  //vec3 bgcolor = vec3(0.093f, 0.086, 0.328);
  vec3 bgcolor = vec3(0.05f, 0.05f, 0.2f);
  
  vec3 color = mix(bgcolor, bordercolor, 1.0-not_border);


  if(not_border == 0.0f) {
    //draw border
    FragColor = vec4(bordercolor, 1.0f);
  
  } else if( (inner_pos.x+inner_pos.y) % 2 == 0) {
    FragColor = vec4(0.13f, 0.05f, 0.15f, 1.0f);
  } else {
    FragColor = vec4(0.1f, 0.05f, 0.1f, 1.0f);
  }

  
  //  FragColor = vec4(uv, 0.0f, 1.0f);
   FragColor = vec4(0.192 * .2  ,0.663 *.2 ,0.722 * .3, 1.0);

  /**
  	waves_uv_offset.x = cos(TIME * time_scale.x + (tiled_uvs.x - tiled_uvs.y) * offset_scale.x);
	waves_uv_offset.y = sin(TIME * time_scale.y + (tiled_uvs.x + tiled_uvs.y) * offset_scale.y);
  **/

  vec2 uv_offset;
  uv_offset.x = 0.2 * cos(0.3 * time + 0.01f * (uv.x - uv.y));
  uv_offset.y = 0.2 * sin(0.3 * time + 0.01f * (uv.x + uv.y));
  
  //FragColor = texture(ourTexture, 0.5 * (uv + uv_offset));
  //  FragColor = vec4(TexCoord.x, TexCoord.y, 0.0f, 1.0f);
  
}


#endif
