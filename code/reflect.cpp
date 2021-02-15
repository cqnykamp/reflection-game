//TODO: use aliases for game state structs on some functions


#include "reflect.h"

#include "gameutil.cpp"

#include <vector>
#include <iostream>
#include <fstream>
#include <string>

#define MOUSE_HOVER_SNAP_DIST  1.0f
#define MOUSE_DRAG_SNAP_DIST 0.4f
#define MOUSE_UNSNAP_DIST 0.6f

#define BOARD_WIDTH (20)
#define BOARD_HEIGHT (20)


struct GameState {
  bool game_ended;
  int levelCount;
  int active_level;
};

struct Tile {
  int xid;
  int yid;
  int type;
};

struct LevelInfo {
  int level_height;
  int level_width;
  ivec2 player_pos_original;
  int board[BOARD_HEIGHT][BOARD_WIDTH];

};

//
// Mirror State
//
enum MirrorState {
  MIRROR_INACTIVE,
  MIRROR_DRAGGABLE,
  MIRROR_LOCKED
};


enum Angle {
  RIGHT = 0,
  UP_RIGHT = 1,
  UP = 2,
  UP_LEFT = 3,
};




#define SLEEP_DURATION 600 // milliseconds
uint32 sleepStartTime;

struct LevelState {
  imat3 basis;
  imat3 target_basis;
  MirrorState mirrorState;
  Angle mirrorFragmentAngle;
  float mirrorFragmentMag;
  ivec2 mirrorFragmentAnchor;

  float weight;
  bool is_animation_active;
  bool sleep_active = false;
};


//
// Game Board State
//


//TODO: use simple array
std::vector<Tile> tiles(25);
std::vector<int> tileIds;




//
// Window Info
//
float TILE_LENGTH = 0.2f;
vec3 shift_to_center = {-2.25f, -2.25f, 0.0f};
float lastFrame = 0.0f;




//
// Game-specific helper functions
// 
bool isPointInBounds(vec2 coords) {
  if(coords.x >= 0 && coords.x < BOARD_WIDTH && coords.y >= 0 && coords.y < BOARD_HEIGHT) {
    return true;
  } else {
    return false;
  }
}

bool classifyVector(Angle *angle, vec2 vec) {
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

vec2 toVec(Angle angle) {
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

ivec2 nearestAnchor(LevelInfo *levelInfo, vec2 point) {
  ivec2 nearest_anchor = {};
  float dist = 10000;
  for(int yid=0; yid<(levelInfo->level_height+1); yid++) {
    for(int xid=0; xid<(levelInfo->level_width+1); xid++) {
      if(levelInfo->board[yid][xid] == 1) {	
	float current_dist = magnitude(point - ivec2{xid,yid});
	if(current_dist < dist) {
	  nearest_anchor = ivec2{xid,yid};
	  dist = current_dist;
	}
      }
    }
  }

  return nearest_anchor;
}

bool isNearbyAnchor(LevelInfo *levelInfo, vec2 coord, float maxAcceptableDistance) {
  for(int yid=0; yid < (levelInfo->level_height+1); yid++) {
    for(int xid=0; xid < (levelInfo->level_width+1); xid++) {
      if(levelInfo->board[yid][xid] == 1
	 && magnitude(coord - ivec2{xid,yid}) <= maxAcceptableDistance) {
	return true;
      }
    }
  }
  return false;
}

void reflect_along(LevelState *state, ivec2 anchor, Angle angle) {

  imat3 reflectMatrix;

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
  state->target_basis = reflectMatrix * state->target_basis;

  state->is_animation_active = true;
  state->weight = 0;
  
}


struct viewCalculations {
  mat4 view;
  mat4 view_inverse;
};

viewCalculations setScreenView(LevelInfo *levelInfo, int current_screen_width, int current_screen_height) {

  int level_extent = (levelInfo->level_height > levelInfo->level_width ?
		      levelInfo->level_height : levelInfo->level_width);
  float max_tile_length = 1.5f / level_extent;
  max_tile_length = max_tile_length>0.2f ? 0.2f : max_tile_length;

  
  shift_to_center = {-0.5f * (float)levelInfo->level_width, -0.5f* (float)levelInfo->level_height, 0.0f};

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

  vec3 view_scale = {tile_width, tile_height, 1.0f};
    
  // Setup view and view inverse
  mat4 identity =
    {1,0,0,0,
     0,1,0,0,
     0,0,1,0,
     0,0,0,1};

  mat4 view = identity;
  view.yy = -1;
  
  view.xx *= view_scale.x;
  view.yy *= view_scale.y;
  view.zz *= view_scale.z;
  

  mat4 trans = identity;
  trans.xw += shift_to_center.x;
  trans.yw += shift_to_center.y;
  trans.zw += shift_to_center.z;

  mat4 v_trans = view * trans;
  view = v_trans;
 
 
  //TODO: find a better way to calcuate view_inverse
  mat4 view_inverse = identity;
  mat4 trans2 = identity;
  trans2.xw += shift_to_center.x;
  trans2.yw += shift_to_center.y;
  trans2.zw += shift_to_center.z;

  mat4 view_inverse_trans = view_inverse * trans2;

  view_inverse = view_inverse_trans;

  view_inverse.xx *= 1 / view_scale.x;
  view_inverse.yy *= 1 / view_scale.y;
  view_inverse.zz *= 1 / view_scale.z;

  view_inverse.yy *= -1;
  view_inverse.yw *= -1;
  
  view_inverse.xw *= -1;


  viewCalculations result = {view, view_inverse};
  return result;

}

void loadLevel(GameState *gameState, LevelInfo *levelInfo, LevelState *state, int level_num) {

  for(int yid=0; yid<BOARD_HEIGHT; yid++) {
    for(int xid=0; xid<BOARD_WIDTH; xid++) {
      levelInfo->board[yid][xid] = 0;
    }
  }
  tiles.clear();

  levelInfo->player_pos_original = {0,0};
  levelInfo->level_height = 0;
  levelInfo->level_width = 0;

  LoadedLevel levelData;
  loadLevelFromFile(level_num, &levelData);

  int xid=0;
  int yid=0;
  for(int i=0; i < levelData.pointDataCount; i++) {
    if(levelData.pointData[i] == '\n') {
      xid=0;
      yid++;
      
    } else {
      int thisVal = (int) (levelData.pointData[i] - '0');
      
      levelInfo->board[yid][xid] = thisVal;

      if(thisVal != 0) {
	if(yid > levelInfo->level_height) {
	  levelInfo->level_height = yid;
	}
	if(xid > levelInfo->level_width) {
	  levelInfo->level_width = xid;
	}
      }
      xid++;
    }
  }


  xid = 0;
  yid = 0;
  for(int i=0; i < levelData.tileDataCount; i++) {
    if(levelData.tileData[i] == '\n') {
      xid=0;
      yid++;
    } else {
      int thisVal = (int) (levelData.tileData[i] - '0');

      if(thisVal == 1) { // Regular tile
	tiles.push_back(Tile {xid, yid, 1});
      } else if(thisVal == 2) { // Goal tile
	tiles.push_back(Tile {xid, yid, 2});
      } else if(thisVal == 3) { // Player tile
	tiles.push_back(Tile {xid, yid, 1}); //regular tile from board's perspective
	levelInfo->player_pos_original = {i, yid};
      }

      if(thisVal != 0) {
	if(yid+1 > levelInfo->level_height) {
	  levelInfo->level_height = yid+1;
	}
	if(xid+1 > levelInfo->level_width) {
	  levelInfo->level_width = xid+1;
	}
      }

      xid++;
    }
  }

#if 0    
  std::fstream level_file;
  level_file.open("levels", std::ios::in);

  // If error opening file, quit
  if(!level_file.is_open()) {
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
	
	for(int i=0; i<ints_on_this_line.size(); i++) {
	  int this_int = ints_on_this_line[i];

	  struct Tile tile;
	  tile.xid = i;
	  tile.yid = yid;
	  
	  if(this_int == 1) { // Regular tile
	    tile.type = 1;
	    tiles.push_back(tile);
	    //	    int tileID = createNewRenderObject(vertices, 12, indices, 6, "shaders/shader.shader");

	  } else if(this_int == 2) { // Goal tile
	    tile.type = 2;
	    tiles.push_back(tile);

	  } else if(this_int == 3) { // Player tile
	    
	    tile.type = 1; // regular tile from board's perspective
	    tiles.push_back(tile);
	    levelInfo->player_pos_original = {i, yid};
	    
	  }


	  if(this_int != 0) {
	    levelInfo->level_height = ((yid+1) > levelInfo->level_height) ? (yid+1) : levelInfo->level_height;
	    levelInfo->level_width = ((i+1) > levelInfo->level_width) ? (i+1) : levelInfo->level_width;
	  }
	}
	yid++;


	
      } else if(blocks_passed == 2*level_num + 1) {
	for(int i=0; i<ints_on_this_line.size(); i++) {
	  int this_int = ints_on_this_line[i];
	  levelInfo->board[yid][i] = this_int;

	  if(this_int != 0) {
	    levelInfo->level_height = (yid>levelInfo->level_height) ? yid : levelInfo->level_height;
	    levelInfo->level_width = (i>levelInfo->level_width) ? i : levelInfo->level_width;
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
#endif

  assert(levelInfo->level_height < BOARD_HEIGHT);
  assert(levelInfo->level_width < BOARD_WIDTH);

  
  //Reset flags
  gameState->game_ended = false;
  state->mirrorState = MIRROR_INACTIVE;
  state->sleep_active = false;

  //Setup basis

  //TODO: should this be identity?
  state->target_basis = imat3();
  state->basis = imat3();

}





void onPlayerMovementFinished(LevelInfo *levelInfo, LevelState *state, uint32 time) {

  // Update game state
  state->basis = state->target_basis;

  ivec2 player_lower_left = {10000, 10000};
  for(int yshift=0; yshift<2; yshift++) {
    for(int xshift=0; xshift<2; xshift++) {
      ivec3 rawCorner = ivec3{levelInfo->player_pos_original.x + xshift, levelInfo->player_pos_original.y + yshift, 1};
      ivec3 corner = state->basis * rawCorner;

      if(corner.x < player_lower_left.x || corner.y < player_lower_left.y) {
	player_lower_left = {corner.x, corner.y};
      }
    }

  }
  
  //printf("Lower left corner: (%f, %f)\n", player_lower_left.x, player_lower_left.y);

  // Check if player is supported
  bool is_player_supported = true;
  int supporting_tile_id = -100;

  //TODO: should boundary still be considered BOARD_WIDTH(or height) - 1?
  if(player_lower_left.y < 0 || player_lower_left.y > BOARD_HEIGHT-1 || player_lower_left.x < 0 || player_lower_left.x > BOARD_WIDTH-1) {
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
    state->sleep_active = true;
    

  } else {
    state->mirrorState = MIRROR_INACTIVE;

    // Check if level complete  
    if(tiles[supporting_tile_id].type == 2) {
      std::cout << "LEVEL COMPLETE!\n";
    }
  }
}




void gameUpdateAndRender(gameInput input, gameMemory *memoryInfo, RenderMemoryInfo *renderMemoryInfo) {

  controllerInput controller = input.controllers[0];

  /**
  std::cout << "Game state size:  "<< sizeof(GameState) << "\n";
  std::cout << "Level info size:  "<< sizeof(LevelInfo) << "\n";
  std::cout << "Level state size: "<< sizeof(LevelState) << "\n";
  */

  GameState *gameState = (GameState *) (memoryInfo->permanentStorage);
  LevelInfo *levelInfo = (LevelInfo *) (gameState + 1);
  LevelState *state = (LevelState *) (levelInfo + 1);

  //Make sure that our memory layout is contiguous
  assert( (char*)gameState == (char*) memoryInfo->permanentStorage);
  assert( (char*)gameState + sizeof(GameState) == (char*)levelInfo);
  assert( (char*)levelInfo + sizeof(LevelInfo) == (char*)state);

  //Make sure that we do not overrun our alloted space
  assert( (char*)state+sizeof(LevelState) - (char*)memoryInfo->permanentStorage < memoryInfo->permanentStorageSize);
	  
  
  if(!memoryInfo->isInitialized) {
    loadLevel(gameState, levelInfo, state, 0);
    memoryInfo->isInitialized = true;
  }

  
  

  
  if(controller.rKey.transitionCount && controller.rKey.endedDown) {
    //reset level
    std::cout << "RESETTING LEVEL: " << gameState->active_level << std::endl;
    loadLevel(gameState, levelInfo, state, gameState->active_level);	
  }
  if(controller.leftArrow.transitionCount && controller.leftArrow.endedDown) {
    if(gameState->active_level > 0) {
      //cout << "LOADING LEVEL: " << active_level << endl;
      gameState->active_level += 1;
      loadLevel(gameState, levelInfo, state, gameState->active_level);
    } else {
      //cout << "That's already the first level\n";
    }
  }
  if(controller.rightArrow.transitionCount && controller.rightArrow.endedDown) {
    gameState->active_level += 1;
    //cout << "LOADING LEVEL: " << active_level << endl;
    loadLevel(gameState, levelInfo, state, gameState->active_level);
  }
  
  
  viewCalculations viewResult = setScreenView(levelInfo, input.screenWidth, input.screenHeight);

  float xrel = (float) (controller.mouseX - 0.5f*input.screenWidth)
    / (float) (0.5f*input.screenWidth);
  float yrel = (float) -1.0f * (controller.mouseY-0.5f*input.screenHeight)
    / (float) (0.5f*input.screenHeight);

  vec4 coord_rel = {xrel, yrel, 0.0f, 1.0f};
  vec4 coord_orig = viewResult.view_inverse * coord_rel; 
  vec2 mouse_coords = vec2(coord_orig.x, coord_orig.y);


  // Animation basis
  if(state->sleep_active && input.currentTime >= sleepStartTime + SLEEP_DURATION) {
    state->sleep_active = false;
    reflect_along(state, state->mirrorFragmentAnchor, state->mirrorFragmentAngle);

  }
  mat3 animation_basis = state->target_basis;
  if(state->is_animation_active) {
    state->weight += 0.0025f * input.deltaTime;

    if(state->weight >= 1) {
      state->is_animation_active = false;
      onPlayerMovementFinished(levelInfo, state, input.currentTime);
	
    } else {
      //Lerp basis
      animation_basis  = state->basis * mat3() * (1.0f-state->weight)
	+ state->target_basis * mat3() * (state->weight);
    }

  }

  //Background
  renderObject *renderMemory = renderMemoryInfo->memory;
  *renderMemory = renderObject {BACKGROUND, identity4, viewResult.view};
  renderMemory++;
  
  
  //Floor tiles
  for(int i=0; i<tiles.size(); i++) {
    mat4 model = identity4;
    model.xw = (float) tiles[i].xid;
    model.yw = (float) tiles[i].yid;

    *renderMemory = renderObject {FLOOR_TILE, model, viewResult.view};
    if(tiles[i].type == 2) {
      renderMemory->highlight_key = 1;
    } else {
      renderMemory->highlight_key = 0;
    }
    renderMemory++;
  }


  //Player  
  mat4 model = identity4;
  model.xw = (float) levelInfo->player_pos_original.x;
  model.yw = (float) levelInfo->player_pos_original.y;
  *renderMemory = renderObject {PLAYER, model, viewResult.view, animation_basis};
  renderMemory++;


  //Anchors

  for(int yid=0; yid<BOARD_HEIGHT; yid++) {
    for(int xid=0; xid<BOARD_WIDTH; xid++) {
      if(levelInfo->board[yid][xid] != 0) {

	int highlight_key = 0;

	if(state->mirrorState==MIRROR_INACTIVE) {
	  ivec2 myVec = {xid,yid};
	  if(nearestAnchor(levelInfo, mouse_coords) == myVec
	     && magnitude(mouse_coords - myVec) < MOUSE_HOVER_SNAP_DIST) {
	    highlight_key = 1;
	  }
	}
	if(state->mirrorState==MIRROR_DRAGGABLE || state->mirrorState==MIRROR_LOCKED) {
	  bool isTheAnchor = (ivec2{xid,yid} == state->mirrorFragmentAnchor);
	  bool passesThroughLine = false;
	    
	  ivec2 point = {xid, yid};
	  vec2 coord1 = state->mirrorFragmentAnchor;
	  //TODO: find more accurate way of representing coord2?
	  vec2 coord2 = state->mirrorFragmentAnchor + state->mirrorFragmentMag * toVec(state->mirrorFragmentAngle);

	  Angle current_to_coord;
	  bool pointToCoordValid = classifyVector(&current_to_coord, point - coord1);
	      
	  if(pointToCoordValid) {
	    float pointToCoordLength = magnitude(point-coord1);
	    float pointCoordDot = dot(point-coord1, coord2-coord1);
	    
	    if(state->mirrorFragmentAngle == current_to_coord
	       && abs(state->mirrorFragmentMag) > pointToCoordLength
	       && pointCoordDot > 0) {
	           
	      passesThroughLine = true;
	    }	    
							    
	    if(isTheAnchor || passesThroughLine) {
	      highlight_key = 1;	      
	    }
	  }
	}
	  
	mat4 anchorModel = identity4;
	anchorModel.xw = (float)xid;
	anchorModel.yw = (float)yid;

	*renderMemory = renderObject{ANCHOR, anchorModel, viewResult.view};
	renderMemory->highlight_key = highlight_key;
	renderMemory++;
	    
      }
    }
  }
  


  
  //Mirror

  if(state->mirrorState == MIRROR_DRAGGABLE) {

    vec2 d = mouse_coords - state->mirrorFragmentAnchor;
    float theta  = atan2(d.y, d.x);
    float mag = magnitude(d);

    //
    // Calculate which mirror angle is closest to mouse,
    // aka, set mirrorFragmentAngle
    //
    if( (theta >= -PI/8 && theta <= PI/8) ||
	(theta >= 7*PI/8 || theta <= -7*PI/8)) {
      state->mirrorFragmentAngle = RIGHT;
	
    } else if((theta >= 3*PI/8 && theta <= 5*PI/8) ||
	      (theta <= -3*PI/8 && theta >= -5*PI/8)) {
      state->mirrorFragmentAngle = UP;

      //Diags
    } else if((theta > PI/8 && theta < 3*PI/8) ||
	      (theta  < -5*PI/8 && theta > -7*PI/8)) {
      state->mirrorFragmentAngle = UP_RIGHT;
	
    } else if((theta > 5*PI/8 && theta < 7*PI/8) ||
	      (theta < -PI/8 && theta > -3*PI/8)) {
      state->mirrorFragmentAngle = UP_LEFT;
    }

    //
    // Calculate fragment magnitude
    //
    vec2 mouseToFragmentEnd;
    switch(state->mirrorFragmentAngle) {
    case 0:
      mouseToFragmentEnd = vec2{d.x, 0.0f};
      break;
    case 1:
      mouseToFragmentEnd = d + normalize(vec2{1.,-1}) * mag*sin(theta - PI/4);
      break;
    case 2:
      mouseToFragmentEnd = vec2{0.0f, d.y};
      break;
    case 3:
      mouseToFragmentEnd = d + normalize(vec2(-1.0f,-1.0f)) * mag*sin(3*PI/4 - theta);
      break;
    }

    if(theta >= 7*PI/8 || theta < -PI/8) {
      state->mirrorFragmentMag = -1 * magnitude(mouseToFragmentEnd);
    } else {
      state->mirrorFragmentMag = magnitude(mouseToFragmentEnd);
    }
    
  }

   
  if(controller.mouseLeft.transitionCount && controller.mouseLeft.endedDown) {
    //
    // Left Mouse Button Pressed
    //
    
    if(state->mirrorState == MIRROR_INACTIVE) {
      //Check if mouse pointer nearby anchor
      //If so, set mirror to MIRROR_DRAGGABLE

      if(isNearbyAnchor(levelInfo, mouse_coords, MOUSE_HOVER_SNAP_DIST)) {
	state->mirrorState = MIRROR_DRAGGABLE;

	state->mirrorFragmentAnchor = nearestAnchor(levelInfo, mouse_coords);
	state->mirrorFragmentAngle = RIGHT;
	state->mirrorFragmentMag = 0;
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
    
    if(state->mirrorState == MIRROR_DRAGGABLE) {

      bool passesThroughAnchor = false;

      for(int yid=0; yid < (levelInfo->level_height+1); yid++) {
	for(int xid=0; xid < (levelInfo->level_width+1); xid++) {
	  if(levelInfo->board[yid][xid] == 1) {

	    ivec2 point = {xid, yid};
	    ivec2 coord1 = state->mirrorFragmentAnchor;
	    //TODO: find more accurate way of representing coord2?
	    vec2 coord2 = state->mirrorFragmentAnchor + state->mirrorFragmentMag * toVec(state->mirrorFragmentAngle);
	          
	    Angle current_to_coord;
	    vec2 pointToCoord = point - coord1;
	    bool pointToCoordValid = classifyVector(&current_to_coord, point - coord1);
	          	      
	    if(pointToCoordValid) {
	      float pointToCoordLength = magnitude(point-coord1);
	      float pointCoordDot = dot(point-coord1, coord2-coord1);
	          	    
	      if(state->mirrorFragmentAngle == current_to_coord
		 && abs(state->mirrorFragmentMag) > pointToCoordLength
		 && pointCoordDot > 0) {
	          	           
		passesThroughAnchor = true;
		break;
	      }
	    }
	  }
	}
      }
      	    
            
      if(passesThroughAnchor) {
	state->mirrorState = MIRROR_LOCKED;
	reflect_along(state, state->mirrorFragmentAnchor, state->mirrorFragmentAngle);
	
      } else {
	state->mirrorState = MIRROR_INACTIVE;
      }
      
    }
  }

  
  if(state->mirrorState==MIRROR_DRAGGABLE || state->mirrorState==MIRROR_LOCKED) {
    vec2 diff = state->mirrorFragmentMag * toVec(state->mirrorFragmentAngle);
      
    float x1, y1, x2, y2;
    vec2 thickness_offset_dir;

    thickness_offset_dir = vec2(diff.y, -1 * diff.x);
    thickness_offset_dir = normalize(thickness_offset_dir);
    x1 = (float) state->mirrorFragmentAnchor.x;
    y1 = (float) state->mirrorFragmentAnchor.y;
    x2 = x1 + diff.x;
    y2 = y1 + diff.y;	
    // printf("Mirror segment: (%i, %i) to (%i, %i) Thickness dir: (%f, %f)\n", x1, y1, x2, y2, thickness_offset_dir.x, thickness_offset_dir.y);
      
    float thickness = 0.05f;

    float mirror_vertices[12];
    
    mirror_vertices[0] = x1 + thickness_offset_dir.x * thickness;
    mirror_vertices[1] = y1 + thickness_offset_dir.y * thickness;
    mirror_vertices[2] = 0;
      
    mirror_vertices[3] = x1 - thickness_offset_dir.x * thickness;
    mirror_vertices[4] = y1 - thickness_offset_dir.y * thickness;
    mirror_vertices[5] = 0;    
      
    mirror_vertices[6] = x2 - thickness_offset_dir.x * thickness;
    mirror_vertices[7] = y2 - thickness_offset_dir.y * thickness;
    mirror_vertices[8] = 0;    

    mirror_vertices[9] = x2 + thickness_offset_dir.x * thickness;
    mirror_vertices[10] = y2 + thickness_offset_dir.y * thickness;
    mirror_vertices[11] = 0;    


    updateRenderContextVertices(MIRROR, mirror_vertices, 12);
    *renderMemory = renderObject{MIRROR, identity4, viewResult.view};
    renderMemory++;
    
  }


  //Front Grid
  *renderMemory = renderObject{FRONT_GRID, identity4, viewResult.view};
  renderMemory++;

  //Make sure that we didn't overuse memory in our render buffer
  assert(renderMemory - renderMemoryInfo->memory < renderMemoryInfo->count);

  /**
  std::cout << "Mirror state: " << mirrorState << "\n";
  std::cout << "Current time: " << input.currentTime << "\n";
  std::cout << "Weight: "<< weight << "\n";
  **/

}
