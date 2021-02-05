 /**

   Line Level - Great!!
   Dad Likes to plan out before moving
   Dead end level has easier endgame
   Mark starting square permanently
   Last level has easier ending once get to bottom

   Least amount of moves counter / Least amount of moves and # of moves
   Undo button

   Auto level-checker (much later)

**/

#include "gameutil.cpp"

#define GLFW_INCLUDE_NONE
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <cmath>
#include <vector>
#include <fstream>
#include <ctype.h>
#include <string>

#include "Shader.h"
#include "Texture.h"
#include "ObjectData.h"

using namespace std;


const double PI = 3.141592653589793238463;


const float MOUSE_HOVER_SNAP_DIST = 1.0;
const float MOUSE_DRAG_SNAP_DIST = 0.4;
const float MOUSE_UNSNAP_DIST = 0.6;



//
// General Game State
//
bool game_ended;
int active_level = 0;
TEMPimat3 basis;
TEMPimat3 target_basis;


//
// Game Board State
//

const int board_width = 20;
const int board_height = 20;
int board[board_height][board_width];

int level_height;
int level_width;

TEMPivec2 player_pos_original;

struct Tile {
  int xid;
  int yid;
  int type;
};
vector<Tile> tiles(25);


//
// Mirror State
//
enum MirrorState {
  MIRROR_INACTIVE,
  MIRROR_DRAGGABLE,
  MIRROR_LOCKED
};
MirrorState mirrorState;

enum Angle {
  RIGHT = 0,
  UP_RIGHT = 1,
  UP = 2,
  UP_LEFT = 3,
};
Angle mirrorFragmentAngle;
float mirrorFragmentMag;
TEMPivec2 mirrorFragmentAnchor;


//
// Animation State
//
bool is_animation_active = false;
float weight;

const float SLEEP_DURATION = 0.6; // seconds
bool sleep_active = false;
float sleepStartTime;


int highlights[board_height][board_width];


//
// Input State
//

TEMPvec2 mouse_coords = {0.0f, 0.0f};
bool mouse_button_down = false;
bool change_level_button_down = false;
  


//
// Window Info
//
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
int current_screen_width = SCREEN_WIDTH;
int current_screen_height = SCREEN_HEIGHT;

float TILE_LENGTH = 0.2;
TEMPvec3 shift_to_center = {-2.25f, -2.25f, 0.0f};

TEMPmat4 view_inverse;


float deltaTime = 0.0f;
float lastFrame = 0.0f;



// SHADERS
Shader* ourShader;
Shader* playerShader;
Shader* bgShader;
Shader* anchorShader;
Shader* frontGridShader;
Shader* mirrorShader;







//
// Game-specific helper functions
// 
bool isPointInBounds(TEMPvec2 coords) {
  if(coords.x >= 0 && coords.x < board_width && coords.y >= 0 && coords.y < board_height) {
    return true;
  } else {
    return false;
  }
}

bool classifyVector(Angle *angle, TEMPvec2 vec) {
  if(vec.x == 0) {
    *angle = UP;
  } else if(vec.y == 0) {
    *angle = RIGHT;
  } else if(vec.x == vec.y) {
    *angle = UP_RIGHT;
  } else if(vec.x == -1 * vec.y) {
    *angle = UP_LEFT;
  } else {
    //Cannot classify this vector
    return false;
  }
  return true;
}

TEMPvec2 toVec(Angle angle) {
  switch(angle) {
  case RIGHT:
    return {1.0f, 0.0f};
  case UP_RIGHT:
    return {sqrt(2.0f)/2.0f, sqrt(2.0f)/2.0f};
  case UP:
    return {0.0f, 1.0f};
  case UP_LEFT:
    return {-sqrt(2.0f)/2.0f, sqrt(2.0f)/2.0f};
  default:
    cout << "toVec function failed\n";
    return {0.0f,0.0f};
  }
}

TEMPivec2 nearestAnchor(TEMPvec2 point) {
  TEMPivec2 nearest_anchor = {};
  float dist = 10000;
  for(int yid=0; yid<(level_height+1); yid++) {
    for(int xid=0; xid<(level_width+1); xid++) {
      if(board[yid][xid] == 1) {	
	float current_dist = magnitude(point - TEMPivec2{xid,yid});
	if(current_dist < dist) {
	  nearest_anchor = TEMPivec2{xid,yid};
	  dist = current_dist;
	}
      }
    }
  }

  return nearest_anchor;
}


bool isNearbyAnchor(TEMPvec2 coord, float maxAcceptableDistance) {
  for(int yid=0; yid < (level_height+1); yid++) {
    for(int xid=0; xid < (level_width+1); xid++) {
      if(board[yid][xid] == 1
	 && magnitude(coord - TEMPivec2{xid,yid}) <= maxAcceptableDistance) {
	return true;
      }
    }
  }
  return false;
}

void printBoardState() {
  cout << "Board\n";
  for(int yid=0; yid<board_height; yid++) {
    for(int xid=0; xid<board_width; xid++) {
      cout << board[yid][xid] << " ";
    }
    cout << endl;
  }
  cout << "Tiles\n";
  for(int i=0; i<tiles.size(); i++) {
    printf("(%i, %i) ", tiles[i].xid, tiles[i].yid);
  }
  cout << endl;
}










void reflect_along(TEMPivec2 anchor, Angle angle) {

  TEMPimat3 reflectMatrix;

  switch(angle) {
    
  case RIGHT: {
    reflectMatrix = {
      1,  0, 0,
      0, -1, 2*anchor.y,
      0,  0, 1
    };  
  } break;
    
  case UP_RIGHT: {
    int yinter = anchor.y - anchor.x;
    reflectMatrix = {
      0, 1, -yinter,
      1, 0, yinter,
      0, 0, 1
    };
  } break;

    
  case UP: {
    reflectMatrix = {
      -1, 0, 2*anchor.x,
      0, 1, 0,
      0, 0, 1
    };
    } break;

    
  case UP_LEFT: {
    int yinter = anchor.y + anchor.x;
    reflectMatrix = {
       0, -1, yinter,
      -1,  0, yinter,
       0,  0, 1
    };  
  } break;

  }

  //
  // Transform the space in game state!!
  //
  target_basis = reflectMatrix * target_basis;

  is_animation_active = true;
  weight = 0;
  
}








void setScreenView() {

  int level_extent = (level_height>level_width ? level_height : level_width);
  float max_tile_length = 1.5f / level_extent;
  max_tile_length = max_tile_length>0.2 ? 0.2 : max_tile_length;
  

  shift_to_center = {-0.5f * (float)level_width, -0.5f* (float)level_height, 0.0f};

  float tile_width, tile_height;
  if(current_screen_width > current_screen_height) {
    tile_height = max_tile_length;
    tile_width = (float)current_screen_height / (float)current_screen_width * max_tile_length;
  } else {
    tile_width = max_tile_length;
    tile_height = (float)current_screen_width / (float)current_screen_height * max_tile_length;
  }

  tile_width *= 1.15;

  //  printf("Tile width: %f height %f\n", tile_width, tile_height);

  TEMPvec3 view_scale = {tile_width, tile_height, 1.0f};
    
  // Setup view and view inverse
  TEMPmat4 identity =
    {1,0,0,0,
     0,1,0,0,
     0,0,1,0,
     0,0,0,1};

  TEMPmat4 view = identity;
  view.yy = -1;
  
  view.xx *= view_scale.x;
  view.yy *= view_scale.y;
  view.zz *= view_scale.z;
  

  TEMPmat4 trans = identity;
  trans.xw += shift_to_center.x;
  trans.yw += shift_to_center.y;
  trans.zw += shift_to_center.z;

  TEMPmat4 v_trans = view * trans;
  view = v_trans;
 
 

  //TODO: find a better way to calcuate view_inverse
  view_inverse = identity;
  TEMPmat4 trans2 = identity;
  trans2.xw += shift_to_center.x;
  trans2.yw += shift_to_center.y;
  trans2.zw += shift_to_center.z;

  TEMPmat4 view_inverse_trans = view_inverse * trans2;

  view_inverse = view_inverse_trans;

  view_inverse.xx *= 1 / view_scale.x;
  view_inverse.yy *= 1 / view_scale.y;
  view_inverse.zz *= 1 / view_scale.z;

  view_inverse.yy *= -1;
  view_inverse.yw *= -1;
  
  view_inverse.xw *= -1;
  

  //Update shaders
  ourShader->use();
  ourShader->setMat4("view", view);

  playerShader->use();
  playerShader->setMat4("view", view);

  bgShader->use();
  bgShader->setFloat("tileLength", TILE_LENGTH);
  bgShader->setMat4("view", view);
  
  anchorShader->use();
  anchorShader->setMat4("view", view);

  frontGridShader->use();
  frontGridShader->setMat4("view", view);
  frontGridShader->setFloat("tileLength", TILE_LENGTH);
  
  mirrorShader->use();
  mirrorShader->setMat4("view", view);

}







void loadLevel(int level_num) {

  for(int yid=0; yid<board_height; yid++) {
    for(int xid=0; xid<board_width; xid++) {
      board[yid][xid] = 0;
      highlights[yid][xid] = 0;
    }
  }
  tiles.clear();

  player_pos_original = {0,0};

  level_height = 0;
  level_width = 0;
  
  ifstream level_file;
  level_file.open("levels", ios::in);

  // If error opening file, quit
  if(!level_file) {
    cout << "File not opened successfully!\n";
    level_file.close();
    return;
  }

  //Read from file
    
  int yid = 0;
  int blocks_passed = 0;
  bool prev_line_has_int = false;

  string line;
  while(getline(level_file, line)) {
    
    bool line_has_int = false;
    vector<int> ints_on_this_line;
    for(int i=0; i<line.length(); i++) {
      char ch = line.at(i);
      if(isdigit(ch)) {
	line_has_int = true;
	ints_on_this_line.push_back(ch - '0');
      }
    }

    //cout << line << " (has ints: "<<(bool)line_has_int<<")\n";

    if(line_has_int) {
      if(blocks_passed == 2*level_num) {

	for(int i=0; i<ints_on_this_line.size(); i++) {
	  int this_int = ints_on_this_line[i];

	  struct Tile tile;
	  tile.xid = i;
	  tile.yid = yid;
	  
	  if(this_int == 1) { // Regular tile
	    tile.type = 1;
	    tiles.push_back(tile);
	  } else if(this_int == 2) { // Goal tile
	    tile.type = 2;
	    tiles.push_back(tile);
	  } else if(this_int == 3) { // Player tile
	    
	    tile.type = 1; // regular tile from board's perspective
	    tiles.push_back(tile);
	    player_pos_original = {i, yid};
	  }


	  if(this_int != 0) {
	    level_height = ((yid+1) > level_height) ? (yid+1) : level_height;
	    level_width = ((i+1) > level_width) ? (i+1) : level_width;
	  }
	}
	yid++;


	
      } else if(blocks_passed == 2*level_num + 1) {
	for(int i=0; i<ints_on_this_line.size(); i++) {
	  int this_int = ints_on_this_line[i];
	  board[yid][i] = this_int;

	  if(this_int != 0) {
	    level_height = (yid>level_height) ? yid : level_height;
	    level_width = (i>level_width) ? i : level_width;
	  }


	}
	yid++;

      }

    } else {
      if(prev_line_has_int) {
	blocks_passed += 1;
	yid = 0;

	//cout << "[BLOCKS PASSED: "<<blocks_passed<<"]\n";
      }
    }

    prev_line_has_int = line_has_int;
        
  }
  level_file.close();


  //cout << "level height "<< level_height << " width "<< level_width << endl;
  
  
  //cout << "=== Loading new level ===" << endl;

  //Reset flags
  game_ended = false;

  mirrorState = MIRROR_INACTIVE;

  
  sleep_active = false;


  //Setup basis
  target_basis = TEMPimat3();
  basis = TEMPimat3();


  setScreenView();

  //Update shaders

  playerShader->use();
  TEMPmat4 TEMPmodel = identity4;
  TEMPmodel.xw = player_pos_original.x;
  TEMPmodel.yw = player_pos_original.y;
  
  playerShader->setMat4("model", TEMPmodel);


  bgShader->use();
  bgShader->setFloat("tileLength", TILE_LENGTH);

  frontGridShader->use();
  frontGridShader->setFloat("tileLength", TILE_LENGTH);

}







void onPlayerMovementFinished() {

  // Update game state
  basis = target_basis;

  TEMPivec2 player_lower_left = {10000, 10000};
  for(int yshift=0; yshift<2; yshift++) {
    for(int xshift=0; xshift<2; xshift++) {
      TEMPivec3 rawCorner = TEMPivec3{player_pos_original.x + xshift, player_pos_original.y + yshift, 1};
      TEMPivec3 corner = basis * rawCorner;
      /**
      corner.x += basis.xz;
      corner.y += basis.yz;
      **/

      if(corner.x < player_lower_left.x || corner.y < player_lower_left.y) {
	player_lower_left = {corner.x, corner.y};
      }
    }

  }
  
  //printf("Lower left corner: (%f, %f)\n", player_lower_left.x, player_lower_left.y);


  // Check if player is supported
  bool is_player_supported = true;
  int supporting_tile_id;

  //TODO: should boundary still be considered board_width(or height) - 1?
  if(player_lower_left.y < 0 || player_lower_left.y > board_height-1 || player_lower_left.x < 0 || player_lower_left.x > board_width-1) {
    // off the board
    is_player_supported = false;
  } else {

    is_player_supported = false;
    for(int i=0; i<tiles.size(); i++) {
      if(tiles[i].xid == player_lower_left.x && tiles[i].yid == player_lower_left.y) {
	is_player_supported = true;
	supporting_tile_id = i;
	break;
      }
    }

        
  }

  if(!is_player_supported) {
    cout << "GOAL NOT SUPPORTED! Undoing...\n";

    //Start sleep counter
    sleepStartTime = glfwGetTime();
    sleep_active = true;
    

  } else {
    mirrorState = MIRROR_INACTIVE;

    // Check if level complete  
    if(tiles[supporting_tile_id].type == 2) {
      cout << "LEVEL COMPLETE!\n";
    }
  }
}


void onSleepTimedOut() {
  sleep_active = false;
  reflect_along(mirrorFragmentAnchor, mirrorFragmentAngle);
}








void error_callback(int error, const char* description) {
  fprintf(stderr, "Error: %s\n", description);
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
  glViewport(0,0, width, height);
  current_screen_width = width;
  current_screen_height = height;

  setScreenView();
  
}



void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  
  if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }

  if(key == GLFW_KEY_R && action == GLFW_PRESS) {
    //reset level
    cout << "RESETTING LEVEL: " << active_level << endl;
    loadLevel(active_level);
  }

  if(key == GLFW_KEY_1 && action == GLFW_PRESS) {
    if(active_level > 0) {
      active_level -= 1;
      //cout << "LOADING LEVEL: " << active_level << endl;
      loadLevel(active_level);
    } else {
      //cout << "That's already the first level\n";
    }
  }


  if(key == GLFW_KEY_2 && action == GLFW_PRESS) {
    active_level += 1;
    //cout << "LOADING LEVEL: " << active_level << endl;
    loadLevel(active_level);
  }
}



void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {

  if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    //
    // Left Mouse Button Pressed
    //
    
    if(mirrorState == MIRROR_INACTIVE) {
      //Check if mouse pointer nearby anchor
      //If so, set mirror to MIRROR_DRAGGABLE

      if(isNearbyAnchor(mouse_coords, MOUSE_HOVER_SNAP_DIST)) {
	mirrorState = MIRROR_DRAGGABLE;

	mirrorFragmentAnchor = nearestAnchor(mouse_coords);
	mirrorFragmentAngle = RIGHT;
	mirrorFragmentMag = 0;
	/**printf("Nearest anchor is (%f, %f) to mouse coords (%f, %f)\n",
	       mirrorFragmentAnchor.x,
	       mirrorFragmentAnchor.y,
	       mouse_coords.x,
	       mouse_coords.y
	       );**/
      }
    }
    
       
  } else if(button == GLFW_MOUSE_BUTTON_LEFT && action==GLFW_RELEASE) {
    //
    // Left Mouse Button Released
    //
    
    if(mirrorState == MIRROR_DRAGGABLE) {

      bool passesThroughAnchor = false;

      for(int yid=0; yid < (level_height+1); yid++) {
	for(int xid=0; xid < (level_width+1); xid++) {
	  if(board[yid][xid] == 1) {

	    
	    TEMPivec2 point = {xid, yid};
	    TEMPivec2 coord1 = mirrorFragmentAnchor;
	    //TODO: find more accurate way of representing coord2?
	    TEMPvec2 coord2 = mirrorFragmentAnchor + mirrorFragmentMag * toVec(mirrorFragmentAngle);
	          
	    Angle current_to_coord;
	    TEMPvec2 pointToCoord = point - coord1;
	    bool pointToCoordValid = classifyVector(&current_to_coord, point - coord1);
	          	      
	    if(pointToCoordValid) {
	      float pointToCoordLength = magnitude(point-coord1);
	      float pointCoordDot = dot(point-coord1, coord2-coord1);
	          	    
	      if(mirrorFragmentAngle == current_to_coord
		 && abs(mirrorFragmentMag) > pointToCoordLength
		 && pointCoordDot > 0) {
	          	           
		passesThroughAnchor = true;
		break;
	      }
	    }
	  }
	}
      }
      	    
            
      if(passesThroughAnchor) {
	mirrorState = MIRROR_LOCKED;
	reflect_along(mirrorFragmentAnchor, mirrorFragmentAngle);
	
      } else {
	mirrorState = MIRROR_INACTIVE;
      }
      
    }
  }
}



void update_state_from_input(double xpos, double ypos) {

  //
  // Convert mouse into game coords
  // Set mouse_coords
  //
  
  float xrel = (float) (xpos-0.5*current_screen_width) / (float) (0.5*current_screen_width);
  float yrel = (float) -1.0f * (ypos-0.5*current_screen_height) / (float) (0.5*current_screen_height); 
  TEMPvec4 coord_rel = {xrel, yrel, 0.0f, 1.0f};
  TEMPvec4 coord_orig = view_inverse * coord_rel;
 
  mouse_coords = TEMPvec2(coord_orig.x, coord_orig.y);
  //printf("Mouse coords (%f, %f)\n", mouse_coords.x, mouse_coords.y);

  



  if(mirrorState == MIRROR_DRAGGABLE) {

    TEMPvec2 d = mouse_coords - mirrorFragmentAnchor;
    float theta  = atan2(d.y, d.x);
    float mag = magnitude(d);

    //
    // Calculate which mirror angle is closest to mouse,
    // aka, set mirrorFragmentAngle
    //
    if( (theta >= -PI/8 && theta <= PI/8) ||
	(theta >= 7*PI/8 || theta <= -7*PI/8)) {
      mirrorFragmentAngle = RIGHT;
	
    } else if((theta >= 3*PI/8 && theta <= 5*PI/8) ||
	      (theta <= -3*PI/8 && theta >= -5*PI/8)) {
      mirrorFragmentAngle = UP;

      //Diags
    } else if((theta > PI/8 && theta < 3*PI/8) ||
	      (theta  < -5*PI/8 && theta > -7*PI/8)) {
      mirrorFragmentAngle = UP_RIGHT;
	
    } else if((theta > 5*PI/8 && theta < 7*PI/8) ||
	      (theta < -PI/8 && theta > -3*PI/8)) {
      mirrorFragmentAngle = UP_LEFT;
    }

    //
    // Calculate fragment magnitude
    //
    TEMPvec2 mouseToFragmentEnd;
    switch(mirrorFragmentAngle) {
    case 0:
      mouseToFragmentEnd = TEMPvec2{d.x, 0.0f};
      break;
    case 1:
      mouseToFragmentEnd = d + normalize(TEMPvec2{1.,-1}) * mag*sin(theta - PI/4);
      break;
    case 2:
      mouseToFragmentEnd = TEMPvec2{0.0f, d.y};
      break;
    case 3:
      mouseToFragmentEnd = d + normalize(TEMPvec2(-1.0f,-1.0f)) * mag*sin(3*PI/4 - theta);
      break;
    }

    if(theta >= 7*PI/8 || theta < -PI/8) {
      mirrorFragmentMag = -1 * magnitude(mouseToFragmentEnd);
    } else {
      mirrorFragmentMag = magnitude(mouseToFragmentEnd);
    }
    
  }
  
}



int main() {  
  
  glfwSetErrorCallback(error_callback);
  // glfw: initialize and configure
  if(!glfwInit()) {
    return 1;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  // glfw window creation
  GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "LearnOpenGL", NULL, NULL);
  if(window == NULL) {
    cout << "Failed to create GLFW window" << endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);  

  //Input callbacks
  glfwSetKeyCallback(window, key_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);  
  // glad: load all OpenGL function pointers
  gladLoadGL(glfwGetProcAddress);
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  //glEnable(GL_DEPTH_TEST);  

  ourShader = new Shader("shaders/shader.vs", "shaders/shader.fs");
  //playerShader = new Shader("shaders/playerShader.vs", "shaders/playerShader.fs");
  playerShader = new Shader("shaders/player.shader");
  bgShader = new Shader("shaders/bgshader.vs", "shaders/bgshader.fs");
  anchorShader = new Shader("shaders/anchorShader.vs", "shaders/anchorShader.fs");
  frontGridShader = new Shader("shaders/frontGridShader.vs", "shaders/frontGridShader.fs");
  mirrorShader = new Shader("shaders/mirrorShader.vs", "shaders/mirrorShader.fs");


  
  // set up vertex data (and buffer(s)) and configure vertex attributes

  float anchor_vertices[] = {
    -1.0f, -1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
  };
  unsigned int anchor_indices[] = {
    0, 1, 2,
    2, 3, 0
  };
  ObjectData anchor_data = ObjectData(12, 6, anchor_vertices, anchor_indices);


  
  float background_vertices[] = {
    -50.0f, -50.0f, 0.0f, 0.0f, 0.0f,
    -50.0f, 50.0f, 0.0f,  0.0f, 1.0f,
    50.0f, 50.0f, 0.0f,   1.0f, 1.0f,
    50.0f, -50.0f, 0.0f,  1.0f, 0.0f,
  };
  unsigned int background_indices[] = {
    0, 1, 2,
    2, 3, 0
  };
  Texture water_texture = Texture("textures/water.jpg");
  ObjectData background_data = ObjectData(20, 6, background_vertices, background_indices, true);


  
  // Floor tiles

  float vertices[] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
  };  
  unsigned int indices[] = {
    0, 1, 2,
    2, 3, 0
  };
  ObjectData floor_tiles_data = ObjectData(12, 6, vertices, indices);


  
  // PLAYER VAO
  
  float player_vertices[] = {
    0.15f, 0.15f, 0.0f,
    0.85f, 0.15f, 0.0f,
    0.85f, 0.85f, 0.0f,
    0.15f, 0.85f, 0.0f,
  };
  unsigned int player_indices[] = {
    0, 1, 2,
    2, 3, 0
  };
  ObjectData player_data = ObjectData(12, 6, player_vertices, player_indices);

  

  // FRONT GRID VAO

  float front_grid_vertices[] = {
    0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
  };
  unsigned int front_grid_indices[] = {
    0, 1, 2,
    2, 3, 0
  };
  ObjectData front_grid_data = ObjectData(12, 6, front_grid_vertices, front_grid_indices);



  // MIRROR VAO


  float mirror_vertices[] = {
    0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
  };
  unsigned int mirror_indices[] = {
    0, 1, 2,
    2, 3, 0
  };
  ObjectData mirror_data = ObjectData(12, 6, mirror_vertices, mirror_indices);  


  
  // TEXTURES
  /**
     unsigned int texture1;
     glGenTextures(1, &texture1);
     glBindTexture(GL_TEXTURE_2D, texture1);

     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);  

     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  
  
     int width, height, nrChannels;
     unsigned char *data = stbi_load("textures\\container.jpg", &width, &height, &nrChannels, 0);
     stbi_set_flip_vertically_on_load(true);
     if(data) {
     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
     glGenerateMipmap(GL_TEXTURE_2D);

     } else {
     cout << "Failed to load texture 1" << endl;

     }

     stbi_image_free(data);
  

     // texture 2
     unsigned int texture2;
     glGenTextures(1, &texture2);
     glBindTexture(GL_TEXTURE_2D, texture2);

     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);  

     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  


     unsigned char *data2 = stbi_load("textures\\awesomeface.png", &width, &height, &nrChannels, 0);
     if(data) {
     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data2);
     glGenerateMipmap(GL_TEXTURE_2D);

     } else {
     cout << "Failed to load second texture" << endl;
     }

     stbi_image_free(data2);
  **/

  
  loadLevel(active_level);

  int counter = 0;
  
  
  // RENDER LOOP
  
  while(!glfwWindowShouldClose(window)){

    // Calculate delta time
    float currentFrame = glfwGetTime();

    /**
       counter++;
       if(floor(currentFrame) != floor(lastFrame)) {
       cout << "FPS: " << counter << endl;
       counter = 0;
       }
    **/
    
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    //printf("Frame duration: %f ms\n", 1000*deltaTime);

    if(sleep_active && currentFrame >= sleepStartTime + SLEEP_DURATION) {
      onSleepTimedOut();
    }


    //Update state from input
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos); // poll mouse position
    update_state_from_input(xpos, ypos);

    // Background color
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);    

    
    //draw background
    background_data.use();
    bgShader->use();
    bgShader->setFloat("time", glfwGetTime());
    water_texture.bind();
    background_data.draw();


    
    //Textures
    /**
       glActiveTexture(GL_TEXTURE0);
       glBindTexture(GL_TEXTURE_2D, texture1);
       glActiveTexture(GL_TEXTURE1);
       glBindTexture(GL_TEXTURE_2D, texture2);
    **/


    //Draw floor tiles
    floor_tiles_data.use();
    ourShader->use();

    for(int i=0; i<tiles.size(); i++) {

      TEMPmat4 TEMPmodel = identity4;
      TEMPmodel.xw = tiles[i].xid;
      TEMPmodel.yw = tiles[i].yid;
      
      ourShader->setMat4("model", TEMPmodel);

      bool is_goal = false;
      if(tiles[i].type == 2) {
	is_goal = true;
      }
      
      ourShader->setBool("is_goal", is_goal);
      floor_tiles_data.draw();
    }
    
    // Animation basis

    TEMPmat3 animation_basis = target_basis;
    if(is_animation_active) {
      //cout << "weight: "<< weight << endl;
      weight += 2.0f * deltaTime;

      if(weight >= 1) {
	is_animation_active = false;

	//cout << "animation over\n";
	onPlayerMovementFinished();

	
       } else {
	animation_basis  = basis * TEMPmat3() * (1.0-weight)
	  + target_basis * TEMPmat3() * (weight);
      }

    }

    //Draw the player
    player_data.use();
    playerShader->use();
    playerShader->setMat3("basis", animation_basis);
    player_data.draw();



    //Draw front grid

    front_grid_data.use();
    frontGridShader->use();
    
    //frontGridShader->setMat3("basis", animation_basis);
    TEMPmat3 identity =
      {1, 0, 0,
       0, 1, 0,
       0, 0, 1};
    frontGridShader->setMat3("basis", identity);
    frontGridShader->setInt("level_width", level_width);
    frontGridShader->setInt("level_height", level_height);	

    for(int i = -10; i<15; i++) {
      for(int j = -10; j<15; j++) {
	frontGridShader->setInt("xcoord", i);
	frontGridShader->setInt("ycoord", j);
	front_grid_data.draw();
      }
    }





    //Draw the anchors
    anchor_data.use();
    anchorShader->use();

    //anchorShader->setFloat("scale", 1.);

    for(int yid=0; yid<board_height; yid++) {
      for(int xid=0; xid<board_width; xid++) {
	if(board[yid][xid] != 0) {

	  int highlight_key = 0;

	  if(mirrorState==MIRROR_INACTIVE) {
	    TEMPivec2 myVec = {xid,yid};
	    if(nearestAnchor(mouse_coords) == myVec
	       && magnitude(mouse_coords - myVec) < MOUSE_HOVER_SNAP_DIST) {
	      highlight_key = 1;
	    }
	  }

	  if(mirrorState==MIRROR_DRAGGABLE || mirrorState==MIRROR_LOCKED) {
	    bool isTheAnchor = (TEMPivec2{xid,yid} == mirrorFragmentAnchor);
 	    bool passesThroughLine = false;
	    
	    TEMPivec2 point = {xid, yid};
	    TEMPvec2 coord1 = mirrorFragmentAnchor;
	    //TODO: find more accurate way of representing coord2?
	    TEMPvec2 coord2 = mirrorFragmentAnchor + mirrorFragmentMag * toVec(mirrorFragmentAngle);

	    Angle current_to_coord;
	    bool pointToCoordValid = classifyVector(&current_to_coord, point - coord1);
	      
	    if(pointToCoordValid) {
	      float pointToCoordLength = magnitude(point-coord1);
	      float pointCoordDot = dot(point-coord1, coord2-coord1);
	    
	      if(mirrorFragmentAngle == current_to_coord
		 && abs(mirrorFragmentMag) > pointToCoordLength
		 && pointCoordDot > 0) {
	           
		passesThroughLine = true;
	      }
	    
							    
	      if(isTheAnchor || passesThroughLine) {
		highlight_key = 1;	      
	      }
	    }
	  }

	  
	  TEMPmat4 TEMPmodel = identity4;
	  TEMPmodel.xw = xid;
	  TEMPmodel.yw = yid;
	  anchorShader->setMat4("model", TEMPmodel);	  
	  anchorShader->setInt("highlight_key", highlight_key);
	  
	  anchor_data.draw();
	    
	}
      }
    }



  
    //Draw the mirror

    if(mirrorState==MIRROR_DRAGGABLE || mirrorState==MIRROR_LOCKED) {
      TEMPvec2 diff = mirrorFragmentMag * toVec(mirrorFragmentAngle);
      
	float x1, y1, x2, y2;
	TEMPvec2 thickness_offset_dir;

	thickness_offset_dir = TEMPvec2(diff.y, -1 * diff.x);
	thickness_offset_dir = normalize(thickness_offset_dir);
	x1 = mirrorFragmentAnchor.x;
	y1 = mirrorFragmentAnchor.y;
	x2 = x1 + diff.x;
	y2 = y1 + diff.y;	
	// printf("Mirror segment: (%i, %i) to (%i, %i) Thickness dir: (%f, %f)\n", x1, y1, x2, y2, thickness_offset_dir.x, thickness_offset_dir.y);
      
	float thickness = 0.05;
	mirror_vertices[0] = x1 + thickness_offset_dir.x * thickness;
	mirror_vertices[1] = y1 + thickness_offset_dir.y * thickness;
      
	mirror_vertices[3] = x1 - thickness_offset_dir.x * thickness;
	mirror_vertices[4] = y1 - thickness_offset_dir.y * thickness;
      
	mirror_vertices[6] = x2 - thickness_offset_dir.x * thickness;
	mirror_vertices[7] = y2 - thickness_offset_dir.y * thickness;

	mirror_vertices[9] = x2 + thickness_offset_dir.x * thickness;
	mirror_vertices[10] = y2 + thickness_offset_dir.y * thickness;

	mirror_data.use();
	mirrorShader->use();

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(mirror_vertices), mirror_vertices);
	/**
	   cout << "Mirror vertices\n";
	   for(int i=0; i<12; i++) {
	   cout << mirror_vertices[i] << endl;
	   }
	**/

	mirrorShader->setBool("highlighted", true);
	mirrorShader->setMat4("model", identity4);
	mirror_data.draw();
    }
    

    glBindVertexArray(0);
    
    // Swap buffers and poll IO
    glfwSwapBuffers(window);
    glfwPollEvents();
    
  }

  /**
  delete ourShader;
  delete bgShader;
  delete anchorShader;
  delete frontGridShader;
  delete mirrorShader;
  **/

  glfwTerminate();
  return 0;
}
