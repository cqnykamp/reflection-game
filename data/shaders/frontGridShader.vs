#version 330 core

layout (location = 0) in vec3 aPos;

out vec2 uv;
out vec3 pos;
out float on_tile;

uniform mat4 view;
uniform int xcoord;
uniform int ycoord;
uniform mat3 basis;

uniform int board[36];


void main() {
  uv = vec2(aPos.x, aPos.y);
  pos = basis * vec3(aPos.x + xcoord, aPos.y + ycoord, 1.0f);
  //  vec3 pos = vec3(aPos.x + xcoord, aPos.y + ycoord, 0.0f);


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
  
  gl_Position = view * vec4(pos.x, pos.y, 0.0f, 1.0f);
}
