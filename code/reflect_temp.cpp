
#include "reflect.h"

#include "gameutil.cpp"

#include <vector>
#include <iostream>
#include <fstream>
#include <string>

int myObjectID;
int obj2;


const float MOUSE_HOVER_SNAP_DIST = 1.0f;
const float MOUSE_DRAG_SNAP_DIST = 0.4f;
const float MOUSE_UNSNAP_DIST = 0.6f;

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

//TODO: use simple array
std::vector<Tile> tiles(25);
std::vector<int> tileIds;


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

const float SLEEP_DURATION = 0.6f; // seconds
bool sleep_active = false;
float sleepStartTime;


int highlights[board_height][board_width];

//
// Window Info
//
float TILE_LENGTH = 0.2f;
TEMPvec3 shift_to_center = {-2.25f, -2.25f, 0.0f};

TEMPmat4 view_inverse;


float deltaTime = 0.0f;
float lastFrame = 0.0f;





int playerObjectID;
int bgObjectID;
int frontGridObjectID;

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
    std::cout << "toVec function failed\n";
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


void setScreenView(int current_screen_width, int current_screen_height) {

  int level_extent = (level_height>level_width ? level_height : level_width);
  float max_tile_length = 1.5f / level_extent;
  max_tile_length = max_tile_length>0.2f ? 0.2f : max_tile_length;

  
  shift_to_center = {-0.5f * (float)level_width, -0.5f* (float)level_height, 0.0f};

  float tile_width, tile_height;
  if(current_screen_width > current_screen_height) {
    tile_height = max_tile_length;
    tile_width = (float)current_screen_height / (float)current_screen_width * max_tile_length;
  } else {
    tile_width = max_tile_length;
    tile_height = (float)current_screen_width / (float)current_screen_height * max_tile_length;
  }

  tile_width *= 1.15f;

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


  int objNum = getObjectCount();

  for(int i=0; i<objNum; i++) {
    gameObject *myObj = getObject(i); //NOTE: assumes internal indexing is contiguous
    myObj->view = view;
  }
    
  //  bgShader->setFloat("tileLength", TILE_LENGTH);
  //frontGridShader->setFloat("tileLength", TILE_LENGTH);
    

}

void loadLevel(int level_num) {

  for(int yid=0; yid<board_height; yid++) {
    for(int xid=0; xid<board_width; xid++) {
      board[yid][xid] = 0;
      highlights[yid][xid] = 0;
    }
  }
  tiles.clear();
  tileIds.clear();

  player_pos_original = {0,0};

  level_height = 0;
  level_width = 0;


  //IMPORTANT!!
  clearAllObjects();




        
  float background_vertices[] = {
    -50.0f, -50.0f, 0.0f, 
    -50.0f, 50.0f, 0.0f,  
    50.0f, 50.0f, 0.0f,   
    50.0f, -50.0f, 0.0f,  
  };
  unsigned int background_indices[] = {
    0, 1, 2,
    2, 3, 0
  };
  //Texture water_texture = Texture("textures/water.jpg");
  int bgID = createNewRenderObject(background_vertices, 12, background_indices, 6, "shaders/background.shader");
  gameObject *bgObj = getObject(bgID);
  bgObj->model = identity4;
  
  std::ifstream level_file;
  level_file.open("levels", std::ios::in);

  // If error opening file, quit
  if(!level_file) {
    std::cout << "File not opened successfully!\n";
    level_file.close();
    return;
  }

  //Read from file
    
  int yid = 0;
  int blocks_passed = 0;
  bool prev_line_has_int = false;

  std::string line;
  while(getline(level_file, line)) {
    
    bool line_has_int = false;
    std::vector<int> ints_on_this_line;
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

	for(int i=0; i<ints_on_this_line.size(); i++) {
	  int this_int = ints_on_this_line[i];

	  struct Tile tile;
	  tile.xid = i;
	  tile.yid = yid;
	  
	  if(this_int == 1) { // Regular tile
	    tile.type = 1;
	    tiles.push_back(tile);
	    int tileID = createNewRenderObject(vertices, 12, indices, 6, "shaders/shader.shader");
	    tileIds.push_back(tileID);
	    gameObject *tileObj = getObject(tileID);
	    tileObj->model = TEMPmat4 {
	      1.f, 0.f, 0.f, (float)tile.xid,
	      0.f, 1.f, 0.f, (float)tile.yid,
	      0.f, 0.f, 1.f, 0.f,
	      0.f, 0.f, 0.f, 1.f
	    };


	    

	  } else if(this_int == 2) { // Goal tile
	    tile.type = 2;
	    tiles.push_back(tile);
	    int tileID = createNewRenderObject(vertices, 12, indices, 6, "shaders/shader.shader");
	    tileIds.push_back(tileID);
	    gameObject *tileObj = getObject(tileID);
	    tileObj->model = TEMPmat4 {
	      1.f, 0.f, 0.f, (float)tile.xid,
	      0.f, 1.f, 0.f, (float)tile.yid,
	      0.f, 0.f, 1.f, 0.f,
	      0.f, 0.f, 0.f, 1.f
	    };
	    


	  } else if(this_int == 3) { // Player tile
	    
	    tile.type = 1; // regular tile from board's perspective
	    tiles.push_back(tile);
	    player_pos_original = {i, yid};
	    int tileID = createNewRenderObject(vertices, 12, indices, 6, "shaders/shader.shader");
	    tileIds.push_back(tileID);
	    gameObject *tileObj = getObject(tileID);
	    tileObj->model = TEMPmat4 {
	      1.f, 0.f, 0.f, (float)tile.xid,
	      0.f, 1.f, 0.f, (float)tile.yid,
	      0.f, 0.f, 1.f, 0.f,
	      0.f, 0.f, 0.f, 1.f
	    };
	    
	    
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
  int playerID = createNewRenderObject(player_vertices, 12, player_indices, 6, "shaders/player.shader");
  gameObject *playerObj = getObject(playerID);
  playerObj->model = TEMPmat4 {
    1.f, 0.f, 0.f, (float) player_pos_original.x,
    0.f, 1.f, 0.f, (float) player_pos_original.y,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f
  };



  // FRONT GRID VAO

  float front_grid_vertices[] = {
    0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    1.0f, 0.0f, 0.0f
  };
  unsigned int front_grid_indices[] = {
    0, 1, 2,
    2, 3, 0
  };


      
    //frontGridShader->setMat3("basis", animation_basis);
    TEMPmat3 identity =
      {1, 0, 0,
       0, 1, 0,
       0, 0, 1};
    //frontGridShader->setMat3("basis", identity);
    //frontGridShader->setInt("level_width", level_width);
    //    frontGridShader->setInt("level_height", level_height);	

    for(int i = -10; i<15; i++) {
      for(int j = -10; j<15; j++) {
	int frontGridID = createNewRenderObject(front_grid_vertices, 12, front_grid_indices, 6, "shaders/frontGridShader.shader");
	gameObject *frontGridObj = getObject(frontGridID);
	frontGridObj->model = TEMPmat4 {
	  1.f, 0.f, 0.f, (float)i,
	  0.f, 1.f, 0.f, (float)j,
	  0.f, 0.f, 1.f, 0.f,
	  0.f, 0.f, 0.f, 1.f
	};

      }
    }




  int frontGridID = createNewRenderObject(front_grid_vertices, 12, front_grid_indices, 6, "shaders/frontGridShader.shader");
  gameObject *frontGridObj = getObject(frontGridID);
  frontGridObj->model = identity4;
  


  //cout << "level height "<< level_height << " width "<< level_width << endl;
  
  
  //cout << "=== Loading new level ===" << endl;

  //Reset flags
  game_ended = false;

  mirrorState = MIRROR_INACTIVE;

  
  sleep_active = false;


  //Setup basis
  target_basis = TEMPimat3();
  basis = TEMPimat3();




  //Update shaders

  TEMPmat4 TEMPmodel = identity4;
  TEMPmodel.xw = (float) player_pos_original.x;
  TEMPmodel.yw = (float) player_pos_original.y;

  gameObject *player = getObject(playerObjectID);
  player->model = TEMPmodel;

  /*
  bgShader->use();
  bgShader->setFloat("tileLength", TILE_LENGTH);
  frontGridShader->use();
  frontGridShader->setFloat("tileLength", TILE_LENGTH);
  */
}





void onPlayerMovementFinished(float time) {

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
  int supporting_tile_id = -100;

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
    std::cout << "GOAL NOT SUPPORTED! Undoing...\n";

    //Start sleep counter
    sleepStartTime = time;
    sleep_active = true;
    

  } else {
    mirrorState = MIRROR_INACTIVE;

    // Check if level complete  
    if(tiles[supporting_tile_id].type == 2) {
      std::cout << "LEVEL COMPLETE!\n";
    }
  }
}


void onSleepTimedOut() {
  sleep_active = false;
  reflect_along(mirrorFragmentAnchor, mirrorFragmentAngle);
}














void gameUpdateAndRender(gameInput input, gameMemory *memory) {
  
  controllerInput controller = input.controllers[0];
  float xrel = (float) (controller.mouseX - 0.5f*input.screenWidth)
    / (float) (0.5f*input.screenWidth);
  float yrel = (float) -1.0f * (controller.mouseY-0.5f*input.screenHeight)
    / (float) (0.5f*input.screenHeight);


  if(!memory->isInitialized) {

    loadLevel(active_level);
    //NOTE: also do this whenever load new level
    setScreenView(input.screenWidth, input.screenHeight);


    /*
    for(int i=0; i<tileIds.size(); i++) {
      gameObject *tileObj = getObject(tileIds[i]);
      tileObj->model = identity4;
      tileObj->view = identity4;
    }
    */


    /**
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

  createNewRenderObject(anchor_vertices, 12, anchor_indices, 6, "shaders/anchorShader.shader");  
    **/
    

#if 0
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
  createNewRenderObject(mirror_vertices, 12, mirror_indices, 6, "shaders/mirrorShader.shader");

  

    /**
    float vertices[] = {
      -0.5f, -0.5f, 0.0f,
      -0.5f, 0.5f, 0.0f,
      0.5f, 0.5f, 0.0f,
      0.5f, -0.5f, 0.0f,	  
    };

    unsigned int indices[] = {
      0, 1, 2,
      2, 3, 0
    };

    myObjectID = createNewRenderObject(vertices, 12, indices, 6,
				       "shaders/testShader.shader");

    obj2 = createNewRenderObject(vertices, 12, indices, 6,
				 "shaders/testShader.shader");
    **/

#endif


    
    memory->isInitialized = true;
  }


  if(controller.rKey.transitionCount && controller.rKey.endedDown) {
    //reset level
    std::cout << "RESETTING LEVEL: " << active_level << std::endl;
    loadLevel(active_level);
    setScreenView(input.screenWidth, input.screenHeight);
	
  }

  if(controller.leftArrow.transitionCount && controller.leftArrow.endedDown) {
    if(active_level > 0) {
      active_level -= 1;
      //cout << "LOADING LEVEL: " << active_level << endl;
      loadLevel(active_level);
      setScreenView(input.screenWidth, input.screenHeight);
    } else {
      //cout << "That's already the first level\n";
    }
  }

  if(controller.rightArrow.transitionCount && controller.rightArrow.endedDown) {
    active_level += 1;
    //cout << "LOADING LEVEL: " << active_level << endl;
    loadLevel(active_level);
    setScreenView(input.screenWidth, input.screenHeight);
  }
  

#if 0
  TEMPvec4 coord_rel = {xrel, yrel, 0.0f, 1.0f};
  TEMPvec4 coord_orig = view_inverse * coord_rel;
 
  TEMPvec2 mouse_coords = TEMPvec2(coord_orig.x, coord_orig.y);


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




  

   
  if(controller.mouseLeft.transitionCount && controller.mouseLeft.endedDown) {
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
    
       
  } else if(controller.mouseLeft.transitionCount && !controller.mouseLeft.endedDown) {
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


  uint32 currentFrame = input.currentTime;//TODO: get time
  //  deltaTime = currentFrame - lastFrame;
  //lastFrame = currentFrame;

  if(sleep_active && currentFrame >= sleepStartTime + SLEEP_DURATION) {
    onSleepTimedOut();
  }

  //gameObject bgObj = getObject(bgObjecttID);
  //bgShader->setFloat("time", glfwGetTime());


  for(int i=0; i<tiles.size(); i++) {

    TEMPmat4 TEMPmodel = identity4;
    //TODO: imat4
    TEMPmodel.xw = (float) tiles[i].xid;
    TEMPmodel.yw = (float) tiles[i].yid;
      
    gameObject *tileObj = getObject(tileIds[i]);
    tileObj->model = TEMPmodel;

    bool is_goal = false;
    if(tiles[i].type == 2) {
      is_goal = true;
    }

    
    //ourShader->setBool("is_goal", is_goal);
    
    //floor_tiles_data.draw();
  }
  



  /**
  TEMPmat4 model = {
    1.f, 0.f, 0.f, xrel * 0.5f,
    0.f, 1.f, 0.f, yrel * 0.5f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f
  };

  gameObject *myObject = getObject(myObjectID);

  myObject->model = model;


  TEMPmat4 model2 = {
    xrel * 0.5f, 0.f, 0.f, 0.f,
    0.f, yrel * 0.5f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f
  };

  gameObject *myObject2 = getObject(obj2);
  myObject2->model = model2;
  **/

  #endif
}
