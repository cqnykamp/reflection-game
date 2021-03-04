//TODO: use aliases for game state structs on some functions

#include "reflect.h"
#include "gameutil.cpp"

#include <iostream>
#include <fstream>
#include <string>

#define MOUSE_HOVER_SNAP_DIST  1.0f
#define MOUSE_DRAG_SNAP_DIST 0.4f
#define MOUSE_UNSNAP_DIST 0.6f

#define BOARD_WIDTH (20)
#define BOARD_HEIGHT (20)
#define MAX_TILES (256)

#define SLEEP_DURATION 600 // milliseconds

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

//TODO: store board and tiles as bitfields?
struct LevelInfo {
  int level_height;
  int level_width;
  ivec2 player_pos_original;
  int board[BOARD_HEIGHT][BOARD_WIDTH];

  Tile tiles[MAX_TILES];
  int activeTiles;

  bool hasSecondPlayer;
  ivec2 secondPlayerStartPos;
};


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


struct LevelState {
  imat3 basis;
  imat3 target_basis;
  MirrorState mirrorState;
  Angle mirrorFragmentAngle;
  float mirrorFragmentMag;
  ivec2 mirrorFragmentAnchor;

  float weight;
  float mirrorTimeElapsed;
  uint32 sleepStartTime;
  bool is_animation_active;
  bool sleep_active = false;


  mat3 hexBasis;
  int hexMirrorAngleId;
  
};


float customMod(float num, float base) {
  float result = num;
  if(result < 0) {
    while(result + base <= base) {
      result += base;
    }
  } else {
    while(result >= base) {
      result -= base;
    }
  }
  return result;
}


void * claimMemory(void **existingMemoryBuffer, int bytes) {
  void *myNewMemory = *existingMemoryBuffer;
  *existingMemoryBuffer = (void*)((char*)(*existingMemoryBuffer) + bytes);
  return myNewMemory;
}


vec2 squareToHexGrid(vec2 squareCoords) {
  float hexY = squareCoords.y; // * sqrt(0.75f);

  vec2 newCoords = vec2 {
    squareCoords.x + 0.5f * abs( customMod(hexY - 1.f, 2.f) - 1.f),
    hexY * sqrt(0.75f)
  };
  return newCoords;
}


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

ivec2 nearestAnchor(LevelInfo *levelInfo, vec2 point, bool hexMode) {
  ivec2 nearest_anchor = {};
  float dist = 10000;
  for(int yid=0; yid<(levelInfo->level_height+1); yid++) {
    for(int xid=0; xid<(levelInfo->level_width+1); xid++) {
      if(levelInfo->board[yid][xid] == 1) {

	vec2 myVec = vec2{(float)xid, (float)yid};
	if(hexMode) {
	  myVec = squareToHexGrid(myVec);
	}

	float current_dist = magnitude(point - myVec);
	if(current_dist < dist) {
	  nearest_anchor = ivec2{xid,yid};
	  dist = current_dist;
	}
      }
    }
  }

  return nearest_anchor;
}

bool isNearbyAnchor(LevelInfo *levelInfo, vec2 coord, float maxAcceptableDistance, bool hexMode) {
  for(int yid=0; yid < (levelInfo->level_height+1); yid++) {
    for(int xid=0; xid < (levelInfo->level_width+1); xid++) {

      vec2 thisVec = vec2{(float)xid, (float)yid};
      if(hexMode) {
	thisVec = squareToHexGrid(thisVec);
      }
      
      if(levelInfo->board[yid][xid] == 1
	 && magnitude(coord - thisVec) <= maxAcceptableDistance) {
	return true;
      }
    }
  }
  return false;
}

void reflectAlongHexMode(LevelState *state, ivec2 anchor, Angle angle) {
  state->is_animation_active = true;
  state->weight = 0;
  
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
  
  //  vec3 shift_to_center = {-2.25f, -2.25f, 0.0f};
  vec3 shift_to_center = {-0.5f * (float)levelInfo->level_width, -0.5f* (float)levelInfo->level_height, 0.0f};

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

void loadLevel(gameMemory *memoryInfo, GameState *gameState, LevelInfo *levelInfo, LevelState *state, void *tempMemory, int level_num) {

  unsigned int bufferid;

  memoryInfo->debugLog("");


  /**
  float m1 = customMod(10.2323f, 2.f);
  float m2 = customMod(2.65234f, 7.1232f);
  float m3 = customMod(-5.2342f, 3.f);
  std::printf("Mods:\n%f \n%f\n%f\n", m1, m2, m3);
  */

  for(int yid=0; yid<BOARD_HEIGHT; yid++) {
    for(int xid=0; xid<BOARD_WIDTH; xid++) {
      levelInfo->board[yid][xid] = 0;
    }
  }

  for(int i=0; i<MAX_TILES; i++) {
    levelInfo->tiles[i] = {0,0,0};
  }


  bool hasFoundPlayer = false;
  levelInfo->player_pos_original = {0,0};
  levelInfo->level_height = 0;
  levelInfo->level_width = 0;
  levelInfo->secondPlayerStartPos = {0,0};
  levelInfo->hasSecondPlayer = false;

  LoadedLevel *levelData = (LoadedLevel *) claimMemory(&tempMemory, sizeof(LoadedLevel));
  memoryInfo->loadLevelFromFile(level_num, levelData);
  
  int8 xid=0;
  int8 yid=0;
  for(int i=0; i < levelData->pointDataCount; i++) {
    if(levelData->pointData[i] == '\n') {
      xid=0;
      yid++;
      
    } else {
      int thisVal = (int) (levelData->pointData[i] - '0');
      
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
  levelInfo->activeTiles = 0;
  for(int i=0; i < levelData->tileDataCount; i++) {
    if(levelData->tileData[i] == '\n') {
      xid=0;
      yid++;
    } else {
      int8 thisVal = (int8) (levelData->tileData[i] - '0');

      if(thisVal == 1 || thisVal ==2) { // Regular tile or goal tile
	levelInfo->tiles[levelInfo->activeTiles] = Tile {xid, yid, thisVal};
	levelInfo->activeTiles++;
	
      } else if(thisVal == 3) { // Player tile
	levelInfo->tiles[levelInfo->activeTiles] = Tile {xid, yid, 1}; //regular tile from board's perspective
	levelInfo->activeTiles++;

	if(hasFoundPlayer) {
	  levelInfo->secondPlayerStartPos = {xid, yid};
	  levelInfo->hasSecondPlayer = true;
	} else {
	  hasFoundPlayer = true;
	  levelInfo->player_pos_original = {xid, yid};
	}

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

  assert(levelInfo->level_height < BOARD_HEIGHT);
  assert(levelInfo->level_width < BOARD_WIDTH);
  
  //Reset flags
  gameState->game_ended = false;
  state->mirrorState = MIRROR_INACTIVE;
  state->sleep_active = false;

  //Setup basis
  state->target_basis = identity3i;
  state->basis = identity3i;

  state->hexBasis = identity3f;

}





void onPlayerMovementFinished(gameMemory *memoryInfo, LevelInfo *levelInfo, LevelState *state, uint32 time) {

  // Update game state
  state->basis = state->target_basis;

  ivec2 player_lower_left = {10000, 10000};
  ivec2 secondPlayerLowerLeft = {10000, 10000};
  for(int yshift=0; yshift<2; yshift++) {
    for(int xshift=0; xshift<2; xshift++) {
      ivec3 rawCorner = ivec3{levelInfo->player_pos_original.x + xshift, levelInfo->player_pos_original.y + yshift, 1};
      ivec3 corner = state->basis * rawCorner;

      if(corner.x < player_lower_left.x || corner.y < player_lower_left.y) {
	player_lower_left = {corner.x, corner.y};
      }


      if(levelInfo->hasSecondPlayer) {
	ivec3 secondPlayerRawCorner = ivec3{
	  levelInfo->secondPlayerStartPos.x + xshift,
	  levelInfo->secondPlayerStartPos.y + yshift,
	  1};
	ivec3 secondPlayerCorner = state->basis * secondPlayerRawCorner;

	if(secondPlayerCorner.x < secondPlayerLowerLeft.x ||
	   secondPlayerCorner.y < secondPlayerLowerLeft.y) {
	  secondPlayerLowerLeft = {secondPlayerCorner.x, secondPlayerCorner.y};
	}
      }
      
    }

  }
  
  //printf("Lower left corner: (%f, %f)\n", player_lower_left.x, player_lower_left.y);

  // Check if player is supported
  bool is_player_supported = true;
  int supporting_tile_id = -100;

  bool isSecondPlayerSupported = true;
  int secondPlayerSupportingTile = -100;

  //TODO: should boundary still be considered BOARD_WIDTH(or height) - 1?
  if(player_lower_left.y < 0 || player_lower_left.y > BOARD_HEIGHT-1 || player_lower_left.x < 0 || player_lower_left.x > BOARD_WIDTH-1) {
    // off the board
    is_player_supported = false;


  } else if(levelInfo->hasSecondPlayer &&
	    (secondPlayerLowerLeft.y < 0 ||
	     secondPlayerLowerLeft.y > BOARD_HEIGHT-1 ||
	     secondPlayerLowerLeft.x < 0 ||
	     secondPlayerLowerLeft.x > BOARD_WIDTH-1)) {
    // off the board
    isSecondPlayerSupported = false;

  } else {

    is_player_supported = false;
    isSecondPlayerSupported = false;
    for(int i=0; i<levelInfo->activeTiles; i++) {

      bool firstPlayerOnThisTile = (levelInfo->tiles[i].xid == player_lower_left.x &&
				    levelInfo->tiles[i].yid == player_lower_left.y);

      if(!is_player_supported && firstPlayerOnThisTile) {
	is_player_supported = true;
	supporting_tile_id = i;
      }

      
      bool secondPlayerOnThisTile;
      if(levelInfo->hasSecondPlayer) {

	secondPlayerOnThisTile= (levelInfo->tiles[i].xid == secondPlayerLowerLeft.x &&
				 levelInfo->tiles[i].yid == secondPlayerLowerLeft.y);

	if(!isSecondPlayerSupported && secondPlayerOnThisTile) {
	  isSecondPlayerSupported = true;
	  secondPlayerSupportingTile = i;

	}

      }
        
    }
  }

  if(!is_player_supported || (levelInfo->hasSecondPlayer && !isSecondPlayerSupported)) {
    std::cout << "GOAL NOT SUPPORTED! Undoing...\n";

    //Start sleep counter
    state->sleepStartTime = time;
    state->sleep_active = true;


  } else {
    state->mirrorState = MIRROR_INACTIVE;

    // Check if level complete
    if(levelInfo->hasSecondPlayer &&
       levelInfo->tiles[supporting_tile_id].type == 2 &&
       levelInfo->tiles[secondPlayerSupportingTile].type == 2) {
      std::cout << "LEVEL COMPLETE!";
      memoryInfo->debugLog("Level complete!");

    } else if(!levelInfo->hasSecondPlayer &&
	      levelInfo->tiles[supporting_tile_id].type == 2) {
      std::cout << "LEVEL COMPLETE!";
      memoryInfo->debugLog("Level Complete!");
    }
  }
}


void loadRenderObjects(CreateNewRenderObject *createNewRenderObject, bool hexMode) {
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
    createNewRenderObject(background_vertices, 12, background_indices, 6, "shaders/background.shader", BACKGROUND);


    
    if(hexMode) {
      float floor_vertices[] = {
	0.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	0.5f, sqrt(0.75f), 0.0f
	//NOTE: these do not get affected by square to hex grid
      };
    
      unsigned int floor_indices[] = {
	0, 1, 2
      };
	
      createNewRenderObject(floor_vertices, 9, floor_indices, 3,
			    "shaders/floorTile.shader", FLOOR_TILE);
      
    } else {
      float floor_vertices[] = {
	0.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f
      };  
      unsigned int floor_indices[] = {
	0, 1, 2,
	2, 3, 0
      };
      createNewRenderObject(floor_vertices, 12, floor_indices, 6,
			    "shaders/floorTile.shader", FLOOR_TILE);

    }



    if(hexMode) {
    float pad = 0.1f;

    float player_vertices[] = {
      sqrt(3.f) * pad, 0.f + pad, 0.f, //left
      1.0f - sqrt(3.f) * pad, 0.f + pad, 0.f, //right
      0.5f, 1.5f / sqrt(3.f) - pad * 2.0f, 0.f //bottom      
    };
    unsigned int player_indices[] = {
      0, 1, 2
    };

    createNewRenderObject(player_vertices, 9, player_indices, 3,
			  "shaders/player.shader", PLAYER);


    } else {
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
      createNewRenderObject(player_vertices, 12, player_indices, 6,
			    "shaders/player.shader", PLAYER);

    }
    

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
    createNewRenderObject(anchor_vertices, 12, anchor_indices, 6, "shaders/anchor.shader", ANCHOR);
	
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
    createNewRenderObject(mirror_vertices, 12, mirror_indices, 6, "shaders/mirror.shader", MIRROR);

    float front_grid_vertices[] = {
      -50.f, -50.f, 0.0f,
      -50.f, 50.0f, 0.0f,
      50.0f, 50.0f, 0.0f,
      50.0f, -50.0f, 0.0f
    };
    unsigned int front_grid_indices[] = {
      0, 1, 2,
      2, 3, 0
    };
    if(hexMode) {
      createNewRenderObject(front_grid_vertices, 12, front_grid_indices, 6,
			    "shaders/frontGrid.shader", FRONT_GRID);
    } else {
      createNewRenderObject(front_grid_vertices, 12, front_grid_indices, 6,
			    "shaders/frontGrid_square.shader", FRONT_GRID);

    }
}



extern "C" GAME_UPDATE_AND_RENDER(gameUpdateAndRender) {

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


  //levelInfo->player_pos_original = ivec2{5,5};


  void *tempMemory = memoryInfo->temporaryStorage;

  if(!memoryInfo->isInitialized) {
    loadLevel(memoryInfo, gameState, levelInfo, state, tempMemory, 0);
    memoryInfo->isInitialized = true;
  }

  if(memoryInfo->isDllFirstFrame) {
    std::cout << "=== Game Code reloaded ===\n";
    std::cout << "Using hex " << memoryInfo->hexMode << "\n";
    memoryInfo->isDllFirstFrame = false;

    loadRenderObjects(memoryInfo->createNewRenderObject, memoryInfo->hexMode);
  }
  
  
  if(controller.rKey.transitionCount && controller.rKey.endedDown) {
    //reset level
    std::cout << "RESETTING LEVEL: " << gameState->active_level << std::endl;
    loadLevel(memoryInfo, gameState, levelInfo, state, tempMemory, gameState->active_level);
  }
  if(controller.leftArrow.transitionCount && controller.leftArrow.endedDown) {
    if(gameState->active_level > 0) {
      //cout << "LOADING LEVEL: " << active_level << endl;
      gameState->active_level -= 1;
      loadLevel(memoryInfo, gameState, levelInfo, state, tempMemory, gameState->active_level);
    } else {
      //cout << "That's already the first level\n";
    }
  }

  //std::cout << "[Right arrow] ended down: "<<controller.rightArrow.endedDown<<
  //   " transition count: "<<controller.rightArrow.transitionCount<<"\n";
  
  if(controller.rightArrow.transitionCount && controller.rightArrow.endedDown) {
    gameState->active_level += 1;
    //cout << "LOADING LEVEL: " << active_level << endl;
    loadLevel(memoryInfo, gameState, levelInfo, state, tempMemory, gameState->active_level);
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
  if(state->sleep_active && input.currentTime >= state->sleepStartTime + SLEEP_DURATION) {
    state->sleep_active = false;

    if(memoryInfo->hexMode) {
      reflectAlongHexMode(state, state->mirrorFragmentAnchor, state->mirrorFragmentAngle);
    } else {
      reflect_along(state, state->mirrorFragmentAnchor, state->mirrorFragmentAngle);
    }

  }

  state->mirrorTimeElapsed += input.deltaTime;

  
  mat3 animation_basis = state->target_basis;
  if(state->is_animation_active) {
    state->weight += 0.0025f * input.deltaTime;

    if(state->weight >= 1) {
      state->is_animation_active = false;
      onPlayerMovementFinished(memoryInfo, levelInfo, state, input.currentTime);	
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
  for(int i=0; i<levelInfo->activeTiles; i++) {
    mat4 model = identity4;

    if(memoryInfo->hexMode) {

      
      int overlappingXid;
      if(levelInfo->tiles[i].yid % 2 == 1) {
	overlappingXid = levelInfo->tiles[i].xid / 2;
      } else {
	overlappingXid = (1 + levelInfo->tiles[i].xid) / 2; //NOTE: intentional loss of info
      }
      int numInXid = levelInfo->tiles[i].xid % 2;
      float yid = (float)levelInfo->tiles[i].yid;

      vec2 hexCoords = squareToHexGrid(vec2{(float)overlappingXid, yid});
   
      bool flipped = false;
      if(levelInfo->tiles[i].yid % 2 == 1) {
	flipped = !flipped;
      }
      if(numInXid == 1) {
	flipped = !flipped;
      }

      model.xw = hexCoords.x;
      model.yw = hexCoords.y;      
      if(flipped) {
	model.yw += sqrt(0.75f);
	model.xw -= 0.5f;
	model.yy *= -1;
      }
      
      
    } else {

      model.xw = (float) levelInfo->tiles[i].xid;
      model.yw = (float) levelInfo->tiles[i].yid;

    }
    
    
    *renderMemory = renderObject {FLOOR_TILE, model, viewResult.view};
    if(levelInfo->tiles[i].type == 2) {
      renderMemory->highlight_key = 1;
    } else {
      renderMemory->highlight_key = 0;
    }
    renderMemory++;
  }

#if 1
  //Player
  {
    mat4 model = identity4;

    if(memoryInfo->hexMode) {
      vec2 hexCoords = squareToHexGrid(levelInfo->player_pos_original);
      model.xw = hexCoords.x;
      model.yw = hexCoords.y;

    } else {
      model.xw = (float) levelInfo->player_pos_original.x;
      model.yw = (float) levelInfo->player_pos_original.y;
    }
    
    *renderMemory = renderObject {PLAYER, model, viewResult.view, animation_basis};
    renderMemory++;
  }
  
  //Second player
  if(levelInfo->hasSecondPlayer) {
    mat4 model = identity4;

    if(memoryInfo->hexMode) {
      vec2 hexCoords = squareToHexGrid(levelInfo->secondPlayerStartPos);
      model.xw = hexCoords.x;
      model.yw = hexCoords.y;    

    } else {
      model.xw = (float) levelInfo->secondPlayerStartPos.x;
      model.yw = (float) levelInfo->secondPlayerStartPos.y;
    }
    
    *renderMemory = renderObject {PLAYER, model, viewResult.view, animation_basis};
    renderMemory++; 
  }
#endif

  //Anchors
  /**
  int isOdd = (state->mirrorFragmentAnchor.y % 2);	    
  ivec2 targetVec = {};
  ivec2 targetVecReverse= {};
  if(memoryInfo->hexMode) {

    switch(state->hexMirrorAngleId) {
    case 0:
      targetVec = ivec2{1,0};
      targetVecReverse = targetVec;
      break;
    case 1:
      targetVec = ivec2{1 + isOdd, 1};
      break;
    case 2:
      targetVec = ivec2{isOdd, 1};
      break;
    case 3:
      targetVec = ivec2{0, 2};
      targetVecReverse = targetVec;
      break;
    case 4:
      targetVec = ivec2{-1+isOdd,1};
      break;
    case 5:
      targetVec = ivec2{-2+isOdd,1};
      break;
    default:
      assert(false);
    }
  }
  **/  

  for(int yid=0; yid<BOARD_HEIGHT; yid++) {
    for(int xid=0; xid<BOARD_WIDTH; xid++) {
      if(levelInfo->board[yid][xid] != 0) {

	int highlight_key = 0;

	if(state->mirrorState==MIRROR_INACTIVE) {
	  ivec2 myVec = {xid,yid};

	  if(memoryInfo->hexMode) {

	    vec2 hexVec = squareToHexGrid(myVec);
	    if(nearestAnchor(levelInfo, mouse_coords, true) == myVec
	       && magnitude(mouse_coords - hexVec) < MOUSE_HOVER_SNAP_DIST) {
	      highlight_key = 1;
	    }
	    
	  } else {

	    if(nearestAnchor(levelInfo, mouse_coords, false) == myVec
	       && magnitude(mouse_coords - myVec) < MOUSE_HOVER_SNAP_DIST) {
	      highlight_key = 1;
	    }	    

	  }
	  
	}



	if(state->mirrorState==MIRROR_DRAGGABLE || state->mirrorState==MIRROR_LOCKED) {
	  bool isTheAnchor = (ivec2{xid,yid} == state->mirrorFragmentAnchor);
	  bool passesThroughLine = false;
	    
	  ivec2 point = {xid, yid};

	  ivec2 coord1;
	  vec2 coord2;

	  if(memoryInfo->hexMode) {

	    ivec2 hexAnchor = state->mirrorFragmentAnchor;
	    hexAnchor.x *= 2;
	    if(state->mirrorFragmentAnchor.y % 2 == 1) {
	      hexAnchor.x += 1;
	    }

	    ivec2 hexPoint = {2*xid, yid};
	    if(yid % 2 == 1) {
	      hexPoint.x += 1;
	    }


	    ivec2 diff = hexPoint - hexAnchor;
	    int mirrorId = state->hexMirrorAngleId;

	    vec2 mirrorFragRelative = {
	      state->mirrorFragmentMag * cos(state->hexMirrorAngleId * PI/6),
	      state->mirrorFragmentMag * sin(state->hexMirrorAngleId * PI/6),
	    };
	    vec2 mirrorFrag = squareToHexGrid(state->mirrorFragmentAnchor)
					      + mirrorFragRelative;
	    vec2 pointFrag = squareToHexGrid(point) - squareToHexGrid(state->mirrorFragmentAnchor);


	    if( (mirrorId==0 && diff.y==0) ||
	        (mirrorId==3 && diff.x==0) ||

		(mirrorId==2 && diff.x!=0 && diff.y!=0 &&
		 diff.x / 1 == diff.y / 1) ||

		(mirrorId==4 && diff.x!=0 && diff.y!=0 &&
		 diff.x / -1 == diff.y /1) ||

		(mirrorId==1 && diff.x!=0 && diff.y!=0 &&
		 (float)diff.x / 3.f == (float)diff.y / 1.f) ||

		(mirrorId==5 && diff.x!=0 && diff.y!=0 &&
		 (float)diff.x / -3.f == (float)diff.y / 1.f)
		
	       ){

	      //if(dot(mirrorFrag, pointFrag) > 0) {

	      if(magnitude(pointFrag) < state->mirrorFragmentMag) {

		passesThroughLine = true;
	      }
		//}

	    }
	    
	      
	  } else {

	    coord1 = state->mirrorFragmentAnchor;
	    //TODO: find more accurate way of representing coord2?
	    coord2 = state->mirrorFragmentAnchor +
	      state->mirrorFragmentMag * toVec(state->mirrorFragmentAngle);

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
							    
	    }	    

	  }



	  
	  if(isTheAnchor || passesThroughLine) {
	    highlight_key = 1;	      
	  }
	  
	}

	

	
	  
	mat4 anchorModel = identity4;

	if(memoryInfo->hexMode) {
	  vec2 hexCoords = squareToHexGrid(vec2{(float)xid, (float)yid});
	  anchorModel.xw = hexCoords.x;
	  anchorModel.yw = hexCoords.y;

	} else {
	  anchorModel.xw = (float)xid;
	  anchorModel.yw = (float)yid;
	}


	/*
	anchorModel.yw *= sqrt(0.75f);
	if(yid % 2 == 0) {
	  anchorModel.xw += 0.5f;
	}
	*/

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
    // Then calculate fragment magnitude
    //


    if(memoryInfo->hexMode) {

      int closestAngle = (int) floor(6.f/PI * (theta + PI/12));
      state->hexMirrorAngleId = (closestAngle + 12) % 6;

      float snapAngle = PI/6.f * state->hexMirrorAngleId;
      state->mirrorFragmentMag = mag * cos(snapAngle - theta);

      //std::cout << "Closest angle : "<<closestAngle <<
      //	" Mirror angle : "<<state->hexMirrorAngleId<<"\n";


    } else {

      int closestAngle = (int) floor(4/PI * (theta + PI/8));
      state->mirrorFragmentAngle = (Angle) ((closestAngle + 8) % 4);

      float snapAngle = PI/4.f * state->mirrorFragmentAngle;    
      state->mirrorFragmentMag = mag * cos(snapAngle - theta); 
    }    
  }

   
  if(controller.mouseLeft.transitionCount && controller.mouseLeft.endedDown) {
    //
    // Left Mouse Button Pressed
    //
    
    if(state->mirrorState == MIRROR_INACTIVE) {
      //Check if mouse pointer nearby anchor
      //If so, set mirror to MIRROR_DRAGGABLE

      if(isNearbyAnchor(levelInfo, mouse_coords, MOUSE_HOVER_SNAP_DIST, memoryInfo->hexMode)) {
	state->mirrorState = MIRROR_DRAGGABLE;

	state->mirrorFragmentAnchor = nearestAnchor(levelInfo, mouse_coords, memoryInfo->hexMode);
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
	state->mirrorTimeElapsed = 0.f;
	if(memoryInfo->hexMode) {
	  reflectAlongHexMode(state, state->mirrorFragmentAnchor, state->mirrorFragmentAngle);	  
	} else {
	  reflect_along(state, state->mirrorFragmentAnchor, state->mirrorFragmentAngle);
	}
       
      } else {
	state->mirrorState = MIRROR_INACTIVE;
      }
      
    }
  }

  
  if(state->mirrorState==MIRROR_DRAGGABLE || state->mirrorState==MIRROR_LOCKED) {
    
    float extensionMag;
    float mirrorAlpha;
    if(state->mirrorState==MIRROR_LOCKED) {
      extensionMag = 0.00055f * pow(state->mirrorTimeElapsed, 1.3f);
      mirrorAlpha = 1.0f - pow(state->mirrorTimeElapsed, 1.2f) / 1000.f;
      /**
      if(mirrorAlpha <= 0.3f) {
	mirrorAlpha = 0.3f;
      }
      **/
    } else {
      extensionMag = 0.f;
      mirrorAlpha = 1.f;
    }

    /**
    char tempLogBuffer[256];
    sprintf_s(tempLogBuffer, "Mirror alpha: %f", mirrorAlpha); 
    memoryInfo->debugLog(tempLogBuffer);
    **/
    
    float magSign = (state->mirrorFragmentMag > 0) ? 1.f : -1.f;
    vec2 mirrorExtension;
    
    vec2 diff;
    if(memoryInfo->hexMode) {

      vec2 mirrorDir = vec2 {
	cos(state->hexMirrorAngleId * PI/6.f),
	sin(state->hexMirrorAngleId * PI/6.f)
      };
      
      mirrorExtension = extensionMag * mirrorDir;
      diff = state->mirrorFragmentMag * mirrorDir;      

    } else {
      mirrorExtension = extensionMag * magSign * toVec(state->mirrorFragmentAngle);
      diff = state->mirrorFragmentMag * toVec(state->mirrorFragmentAngle);
    }
    
    float x1, y1, x2, y2;
    vec2 thickness_offset_dir;
    thickness_offset_dir = vec2(diff.y, -1 * diff.x);
    thickness_offset_dir = normalize(thickness_offset_dir);

    if(memoryInfo->hexMode) {

      vec2 hexAnchor = squareToHexGrid(state->mirrorFragmentAnchor);

      x1 = hexAnchor.x - mirrorExtension.x;
      y1 = hexAnchor.y - mirrorExtension.y;
      

    } else {
      x1 = (float) state->mirrorFragmentAnchor.x - mirrorExtension.x;
      y1 = (float) state->mirrorFragmentAnchor.y - mirrorExtension.y;
    }
    
    x2 = x1 + diff.x + 1.5f * mirrorExtension.x;
    y2 = y1 + diff.y + 1.5f * mirrorExtension.y;
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


    memoryInfo->updateRenderContextVertices(MIRROR, mirror_vertices, 12);
    *renderMemory = renderObject{MIRROR, identity4, viewResult.view};
    renderMemory->alpha = mirrorAlpha;
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

