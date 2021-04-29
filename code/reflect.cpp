//TODO: use aliases for game state structs on some functions?

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
  mat3 hexTargetBasis;
  int hexMirrorAngleId;


  //NOTE: only for hex mode, updates throughout level
  ivec2 pos;
  
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


void printMat3(mat3 m ) {
  std::printf("%f %f %f \n%f %f %f \n%f %f %f \n",
	      m.xx, m.xy, m.xz,
	      m.yx, m.yy, m.yz,
	      m.zx, m.zy, m.zz); 
}


void * claimMemory(void **existingMemoryBuffer, int bytes) {
  void *myNewMemory = *existingMemoryBuffer;
  *existingMemoryBuffer = (void*)((char*)(*existingMemoryBuffer) + bytes);
  return myNewMemory;
}


//NOTE: these space conversion programs assume you start in anchor coords, not tile coords

ivec2 hexBoardToLinearSpace(ivec2 coords) {

  ivec2 linearCoords = 2 * coords;
  if(linearCoords.y % 4 == 2) {
    linearCoords.x += 1;
  }
  return linearCoords;
}

vec2 hexLinearSpaceToRenderSpace(vec2 coords) {
  vec2 newCoords = (vec2) coords;
  newCoords.x *= 0.5;
  newCoords.y *= sqrt(0.75f);
  return newCoords;
}

vec2 hexBoardToRenderSpace(ivec2 squareCoords) {

  return hexLinearSpaceToRenderSpace(hexBoardToLinearSpace(squareCoords));
    
				     
  

  /**float hexY = squareCoords.y; // * sqrt(0.75f);

  vec2 newCoords = vec2 {
    squareCoords.x + 0.5f * abs( customMod(hexY - 1.f, 2.f) - 1.f),
    hexY * sqrt(0.75f)
  };
  return newCoords;
  **/
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

//TODO: does this function really need to know if in hex mode?
ivec2 nearestAnchor(LevelInfo *levelInfo, vec2 point, bool hexMode) {
  
  ivec2 nearest_anchor = {};
  float dist = 10000;
  for(int yid=0; yid<(levelInfo->level_height+1); yid++) {
    for(int xid=0; xid<(levelInfo->level_width+1); xid++) {
      if(levelInfo->board[yid][xid] == 1) {

	vec2 myVec = vec2{(float)xid, (float)yid};
	if(hexMode) {
	  ivec2 hexVec = ivec2{2*xid,2*yid};
	  if(hexVec.y % 4 == 2) {
	    hexVec.x += 1;
	  }	  
	  myVec = hexVec;
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
	thisVec = hexBoardToLinearSpace(ivec2{xid,yid});
      }
      
      if(levelInfo->board[yid][xid] == 1
	 && magnitude(coord - thisVec) <= maxAcceptableDistance) {
	return true;
      }
    }
  }
  return false;
}

mat3 rotate(float theta) {
  return mat3 {
    cos(theta), -sin(theta), 0.f,
    sin(theta),  cos(theta), 0.f,
    0.f,         0.f,        1.f
  };
}



bool isTriangleFlipped(ivec2 hexCoords) {
  bool flipped = false;
  if( (hexCoords.x + hexCoords.y) % 2 == 1) {
    flipped = !flipped;
  }

  if( (int)floor((float)hexCoords.y / 2.f) % 2 == 1) {
    flipped = !flipped;
  }

  return flipped;
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


  //Points
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



  //Tiles
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
	//regular tile from board's perspective	
	levelInfo->tiles[levelInfo->activeTiles] = Tile {xid, yid, 1}; 
	levelInfo->activeTiles++;

	if(hasFoundPlayer) {
	  levelInfo->secondPlayerStartPos = {xid, yid};
	  //TODO: second player hex pos
	  levelInfo->hasSecondPlayer = true;
	  
	} else {
	  hasFoundPlayer = true;	  
	  levelInfo->player_pos_original = {xid, yid};


	  if(memoryInfo->hexMode) {


	    //TODO: fix player positioning?

	    //levelInfo->player_pos_original =
	    // hexBoardToLinearSpace(levelInfo->player_pos_original);

	    /**
	    levelInfo->player_pos_original.x = (int)
	      ((float)levelInfo->player_pos_original.x * 0.5f);
	    if(levelInfo->player_pos_original.y % 2 == 0) {
	      levelInfo->player_pos_original.x += 1;

	    }
	    **/


	  }

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

  state->hexBasis = identity3i;
  //state->hexBasis.xx = 2;
  
  state->hexTargetBasis = state->hexBasis;


  state->pos = ivec2{
    levelInfo->player_pos_original.x + 1,
    levelInfo->player_pos_original.y * 2  + 1};


  

  std::cout << "Hex basis\n";
  printMat3(state->hexBasis);
  std::cout << "Hex target basis\n";
  printMat3(state->hexTargetBasis);
  std::printf("Player pos orig: (%i, %i)\n",
	      levelInfo->player_pos_original.x,
	      levelInfo->player_pos_original.y);

  std::printf("Player pos (updating): (%i, %i)\n",
	      state->pos.x, state->pos.y);


  std::printf("Player pos render space:\n");
  vec2 playerRenderPos = hexLinearSpaceToRenderSpace(levelInfo->player_pos_original);
  std::printf("(%f, %f)\n", playerRenderPos.x, playerRenderPos.y);
}



//Input: hex player pos (not neccessarily level starting pos)
//Output: new hex player pos
ivec2 findNewPlayerPos(LevelState *state, ivec2 pos) {

  ivec2 newPos;
  
  ivec2 mirrorPos = hexBoardToLinearSpace(state->mirrorFragmentAnchor);

  std::printf("Player pos: (%i, %i)\n", state->pos.x, state->pos.y);
  
  std::cout << "Mirror y pos raw: " << state->mirrorFragmentAnchor.y << "\n";
  std::cout << "Mirror y pos: " << mirrorPos.y << "\n";


  switch(state->hexMirrorAngleId) {
  case 0:
    {

      newPos.x = pos.x;
      newPos.y = 2 * mirrorPos.y - pos.y;
      
    } break;
    
  case 3:
    {
      newPos.y = pos.y;
      newPos.x = 2 * mirrorPos.x - pos.x;
    } break;

  case 2:
    {

      int mirrorDiag = 2*mirrorPos.x - mirrorPos.y;

      int rollDir;
      if( 2*pos.x - pos.y > mirrorDiag) {
	rollDir = 1;
      } else {
	rollDir = -1;
      }

      std::cout << "Roll dir: " << rollDir << "\n";

      bool hasPassedMirror = false;
      ivec2 rollPos = pos;
      int rolls = 0;

      while(!hasPassedMirror) {
	bool flipped = isTriangleFlipped(rollPos);

	if(flipped != (rollDir == -1)) {
	  rollPos = rollPos +  rollDir * ivec2{-2, 2};
	} else {
	  rollPos = rollPos + rollDir * ivec2{-1, 0};
	}
	rolls++;

	std::printf("New roll: (%i, %i) Was flipped: %i Dir: %i\n",
		    rollPos.x, rollPos.y, flipped, rollDir);

	if( (rollDir == 1 && 2*rollPos.x-rollPos.y < mirrorDiag)
	  || (rollDir == -1 && 2*rollPos.x-rollPos.y > mirrorDiag) ) {
	  
	  hasPassedMirror = true;
	}
	
      }


      for(int i=0; i < rolls-1; i++) {
	bool flipped = isTriangleFlipped(rollPos);

	if(flipped != (rollDir == -1)) {
	  rollPos = rollPos +  rollDir * ivec2{-2, 2};
	} else {
	  rollPos = rollPos + rollDir * ivec2{-1, 0};
	}
	//rolls++;

	std::printf("New roll: (%i, %i) Was flipped: %i Dir: %i\n",
		    rollPos.x, rollPos.y, flipped, rollDir);	
      }

      newPos = rollPos;

      

      /**
      int playerDiagRaw = 2*state->pos.x - state->pos.y;
      int playerDiagOffset = (int)floor( ceil((float)playerDiagRaw/2.f) / 2.f);
      int playerDiag = playerDiagRaw - playerDiagOffset;


      //int playerDiag = 2 * pos.x - pos.y;
      
      int mirrorDiag = 2 * mirrorPos.x - mirrorPos.y;

      std::cout << "Player diag: " << playerDiag << "\n";
      std::cout << "Player diag raw: " << playerDiagRaw << "\n";
      std::cout << "Player diag offset: " << playerDiagOffset << "\n";

      

      std::cout << "Mirror diag: " << mirrorDiag << "\n";

      int c = (int) ceil( ((float)playerDiagRaw - (float)mirrorDiag - 1.f) / 2.f);
      int vertOffset = (int) floor( ((float) c + 1.f) / 2.f);

      newPos.y = pos.y += -2 * vertOffset;
      newPos.x = pos.x + (mirrorDiag - playerDiag);

      **/

      /**
      if(2 * pos.x - pos.y == 1) {
	newPos.x = pos.x - 1;
	newPos.y = pos.y;
	
      } else if (2*pos.x - pos.y == -1) {
	newPos.x = pos.x + 1;
	newPos.y = pos.y;
      }
      **/

      
    } break;

  case 4:
    {
      int mirrorDiag = 2*mirrorPos.x + mirrorPos.y;

      int rollDir;
      if( 2*pos.x + pos.y > mirrorDiag) {
	rollDir = 1;
      } else {
	rollDir = -1;
      }

      std::cout << "Roll dir: " << rollDir << "\n";

      bool hasPassedMirror = false;
      ivec2 rollPos = pos;
      int rolls = 0;

      while(!hasPassedMirror) {
	bool flipped = isTriangleFlipped(rollPos);

	if(flipped != (rollDir == -1)) {
	  rollPos = rollPos +  rollDir * ivec2{-1, 0};
	} else {
	  rollPos = rollPos + rollDir * ivec2{-2, -2};
	}
	rolls++;

	std::printf("New roll: (%i, %i) Was flipped: %i Dir: %i\n",
		    rollPos.x, rollPos.y, flipped, rollDir);

	if( (rollDir == 1 && 2*rollPos.x + rollPos.y < mirrorDiag)
	  || (rollDir == -1 && 2*rollPos.x + rollPos.y > mirrorDiag) ) {
	  
	  hasPassedMirror = true;
	}
	
      }


      for(int i=0; i < rolls-1; i++) {
	bool flipped = isTriangleFlipped(rollPos);

	if(flipped != (rollDir == -1)) {
	  rollPos = rollPos +  rollDir * ivec2{-1, 0};
	} else {
	  rollPos = rollPos + rollDir * ivec2{-2, -2};
	}
	//rolls++;

	std::printf("New roll: (%i, %i) Was flipped: %i Dir: %i\n",
		    rollPos.x, rollPos.y, flipped, rollDir);	
      }

      newPos = rollPos;

    } break;


  case 1:
    {
      int mirrorDiag = 2*mirrorPos.x - 3* mirrorPos.y;

      int rollDir;
      if( 2*pos.x - 3*pos.y > mirrorDiag + 1) {
	rollDir = 1;
      } else if (2*pos.x -3*pos.y < mirrorDiag - 1) {
	rollDir = -1;
      
      } else {

	std::cout << "Already on the mirror\n";
	return pos;
      }

      std::cout << "Roll dir: " << rollDir << "\n";

      bool hasPassedMirror = false;
      ivec2 rollPos = pos;
      int rolls = 0;

      while(!hasPassedMirror) {
	rollPos = rollPos + rollDir * ivec2{-1, 2};
	
	/**
	bool flipped = isTriangleFlipped(rollPos);

	if(flipped != (rollDir == -1)) {
	  rollPos = rollPos +  rollDir * ivec2{-1, -2};
	} else {
	  rollPos = rollPos + rollDir * ivec2{-2, -2};
	}
	**/
	rolls++;

	std::printf("New roll: (%i, %i) Dir: %i\n",
		    rollPos.x, rollPos.y, rollDir);


	if(2*rollPos.x - 3*rollPos.y <= mirrorDiag + 1 &&
	   2*rollPos.x - 3*rollPos.y >= mirrorDiag - 1) {
	  hasPassedMirror = true;
	  std::cout << "Is on mirror\n";
	  rolls++;

	} else if( (rollDir == 1 && 2*rollPos.x - 3*rollPos.y <= mirrorDiag )
	  || (rollDir == -1 && 2*rollPos.x - 3*rollPos.y >= mirrorDiag) ) {
	  
	  hasPassedMirror = true;
	  std::cout << "Has passed mirror\n"; 
	}
	
      }

      for(int i=0; i < rolls-1; i++) {
	rollPos = rollPos + rollDir * ivec2{-1, 2};
	std::printf("New roll: (%i, %i) Dir: %i\n",
		    rollPos.x, rollPos.y, rollDir);	
      }

      newPos = rollPos;

    } break;

  case 5:
    {

      int mirrorDiag = 2*mirrorPos.x + 3* mirrorPos.y;

      int rollDir;
      if( 2*pos.x + 3*pos.y > mirrorDiag + 1) {
	rollDir = 1;
      } else if (2*pos.x + 3*pos.y < mirrorDiag - 1) {
	rollDir = -1;
      
      } else {

	std::cout << "Already on the mirror\n";
	return pos;
      }

      std::cout << "Roll dir: " << rollDir << "\n";

      bool hasPassedMirror = false;
      ivec2 rollPos = pos;
      int rolls = 0;

      while(!hasPassedMirror) {
	rollPos = rollPos + rollDir * ivec2{-1, -2};
	
	/**
	   bool flipped = isTriangleFlipped(rollPos);

	   if(flipped != (rollDir == -1)) {
	   rollPos = rollPos +  rollDir * ivec2{-1, -2};
	   } else {
	   rollPos = rollPos + rollDir * ivec2{-2, -2};
	   }
	**/
	rolls++;

	std::printf("New roll: (%i, %i) Dir: %i\n",
		    rollPos.x, rollPos.y, rollDir);


	if(2*rollPos.x + 3*rollPos.y <= mirrorDiag + 1 &&
	   2*rollPos.x + 3*rollPos.y >= mirrorDiag - 1) {
	  hasPassedMirror = true;
	  std::cout << "Is on mirror\n";
	  rolls++;

	} else if( (rollDir == 1 && 2*rollPos.x + 3*rollPos.y <= mirrorDiag )
		   || (rollDir == -1 && 2*rollPos.x + 3*rollPos.y >= mirrorDiag) ) {
	  
	  hasPassedMirror = true;
	  std::cout << "Has passed mirror\n"; 
	}
	
      }

      for(int i=0; i < rolls-1; i++) {
	rollPos = rollPos + rollDir * ivec2{-1, -2};
	std::printf("New roll: (%i, %i) Dir: %i\n",
		    rollPos.x, rollPos.y, rollDir);	
      }

      newPos = rollPos;
      
    } break;
    
    
  default: {
    newPos = pos;
  }
  }

  return newPos;
  
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


void reflectAlongHexMode(LevelState *state, ivec2 anchor, int angle) {

  ivec2 hexAnchor = hexBoardToLinearSpace(anchor);


  mat3 anchorToCenter = {
    1.0f, 0.0f, (float) -hexAnchor.x,
    0.0f, 1.0f, (float) -hexAnchor.y,
    0.0f, 0.0f, 1.0f
  };

  mat3 anchorBack = {
    1.0f, 0.0f, (float)hexAnchor.x,
    0.0f, 1.0f, (float)hexAnchor.y,
    0.0f, 0.0f, 1.0f
  };

  mat3 r = identity3i;

  if(angle == 0) {
    r = {
      1.f, 0.f, 0.f,
      0.f, -1.f, 0.f,
      0.f, 0.f, 1.f
    };
  }

  if(angle == 3) {
    r = {
      -1.f, 0.f, 0.f,
      0.f, 1.f, 0.f,
      0.f, 0.f, 1.f
    };
  }


  if(angle == 2) {
    r = {
      -0.5f, 0.75f, 0.f,
      1.0f,  0.5f,  0.f,
      0.f,   0.f,   1.f,
    };
  }
  
  
  mat3 reflectMatrix = anchorBack * r * anchorToCenter;
  
  state->hexTargetBasis = reflectMatrix * state->hexTargetBasis;

  assert(state->hexTargetBasis.zx == 0.f);
  assert(state->hexTargetBasis.zy == 0.f);
  assert(state->hexTargetBasis.zz == 1.f);

  std::cout << "Reflect matrix\n";
  printMat3(reflectMatrix);
  std::cout << "Hex basis\n";
  printMat3(state->hexBasis);
  std::cout << "Hex target basis\n";
  printMat3(state->hexTargetBasis);

  
  state->is_animation_active = true;
  state->weight = 0;
  
}

void onPlayerMovementFinished(gameMemory *memoryInfo, LevelInfo *levelInfo, LevelState *state, uint32 time) {

  // Update game state
  if(memoryInfo->hexMode) {
    state->hexBasis = state->hexTargetBasis;


    /*
    ivec2 boardPos = levelInfo->player_pos_original;
    ivec2 playerPos = ivec2{boardPos.x + 1, 2*boardPos.y + 1};
    
    vec3 tempPlayerPos = vec3{(float)playerPos.x, (float)playerPos.y, 1.0f};
    tempPlayerPos = state->hexTargetBasis * tempPlayerPos;

    playerPos = ivec2{(int)tempPlayerPos.x, (int)tempPlayerPos.y};
    */

    bool isPlayerSupported = false;
    bool isPlayerOnGoal = false;
    
    for(int i=0; i<levelInfo->activeTiles; i++) {
      Tile tile = levelInfo->tiles[i];

      ivec2 tilePos = ivec2{ tile.xid + 1, 2*tile.yid + 1};
     
      if(tilePos.x == state->pos.x && tilePos.y == state->pos.y) {

	std::printf("Supported pos (%i, %i)\n", state->pos.x, state->pos.y);
	
	isPlayerSupported = true;

	if(tile.type == 2) {
	  isPlayerOnGoal = true;
	}
	
	break;
      }
    }

    std::cout << "Player supported : " << isPlayerSupported << "\n";
    std::cout << "Player on goal : " << isPlayerOnGoal << "\n";


    if(!isPlayerSupported) {
      //Start sleep counter
      state->sleepStartTime = time;
      state->sleep_active = true;

    } else {
      state->mirrorState = MIRROR_INACTIVE;

      if(isPlayerOnGoal) {
	std::cout << "LEVEL COMPLETE!";
	memoryInfo->debugLog("Level complete!");

      }
      
    }
    
    
  } else {
  
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
	-0.5f, -1.f/3.f * sqrt(0.75f), 0.f, //left
	 0.5f, -1.f/3.f * sqrt(0.75f), 0.f, //right
	 0.0f,  2.f/3.f * sqrt(0.75f), 0.f //bottom
      };


      /**
      float floor_vertices[] = {
	-1.0f, -1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	0.0f, 1.0f, 0.0f
      };
      **/
    
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
      float pad = 0.0f;

      float player_vertices[] = {
	-0.5f, -1.f/3.f * sqrt(0.75f), 0.f, //left
	0.5f, -1.f/3.f * sqrt(0.75f), 0.f, //right
	0.0f,  2.f/3.f * sqrt(0.75f), 0.f //bottom
      };

      /**
      float player_vertices[] = {
	-1.0f + pad, -1.0f + pad, 0.0f,
	1.0f - pad, -1.0f + pad, 0.0f,
	0.0f, 1.0f - pad, 0.0f
      };
      **/

    
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
      //reflectAlongHexMode(state, state->mirrorFragmentAnchor, state->hexMirrorAngleId);
      state->pos = findNewPlayerPos(state, state->pos);
      state->is_animation_active = false;
      onPlayerMovementFinished(memoryInfo, levelInfo, state, input.currentTime);	

      
    } else {
      reflect_along(state, state->mirrorFragmentAnchor, state->mirrorFragmentAngle);
    }

  }

  state->mirrorTimeElapsed += input.deltaTime;

  
  mat3 animation_basis;

  if(memoryInfo->hexMode) {
    animation_basis = state->hexTargetBasis;
  } else {
    animation_basis  = state->target_basis;
  }

    
  if(state->is_animation_active) {
    state->weight += 0.0025f * input.deltaTime;

    if(state->weight >= 1) {
      state->is_animation_active = false;
      onPlayerMovementFinished(memoryInfo, levelInfo, state, input.currentTime);	
    } else {
      //Lerp basis

      if(memoryInfo->hexMode) {

	animation_basis  = state->hexBasis * mat3() * (1.0f-state->weight)
	  + state->hexTargetBasis * mat3() * (state->weight);
	  
      } else {
      
	animation_basis  = state->basis * mat3() * (1.0f-state->weight)
	  + state->target_basis * mat3() * (state->weight);
      }
    }

  }




  //
  // Update Mirror Angle
  //
  
  if(state->mirrorState == MIRROR_DRAGGABLE) {
    
    vec2 d;
    if(memoryInfo->hexMode) {
      d = mouse_coords - hexBoardToLinearSpace(state->mirrorFragmentAnchor);
    } else {
      d = mouse_coords - state->mirrorFragmentAnchor;
    }
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
 





  
  //Background
  renderObject *renderMemory = renderMemoryInfo->memory;
  *renderMemory = renderObject {BACKGROUND, identity4, viewResult.view};
  renderMemory++;
  
  
  //Floor tiles
  for(int i=0; i<levelInfo->activeTiles; i++) {
    mat4 model = identity4;

    Tile tile = levelInfo->tiles[i];
    ivec2 hexCoords;

    if(memoryInfo->hexMode) {
      
      int overlappingXid;
      if(levelInfo->tiles[i].yid % 2 == 1) {
	overlappingXid = levelInfo->tiles[i].xid / 2;
      } else {
	overlappingXid = (1 + levelInfo->tiles[i].xid) / 2; //NOTE: intentional loss of info
      }
      int numInXid = levelInfo->tiles[i].xid % 2;
      float yid = (float)levelInfo->tiles[i].yid;

      //vec2 hexCoords = hexBoardToRenderSpace(vec2{(float)overlappingXid, yid});

      hexCoords = ivec2{tile.xid + 1, 2*tile.yid + 1};

      /**
      hexCoords = hexBoardToLinearSpace(ivec2{
	  levelInfo->tiles[i].xid, //levelInfo->tiles[i].xid,
	  levelInfo->tiles[i].yid});
      */

      model.xw = 0.5f * (float)hexCoords.x;
      model.yw = 0.5f * sqrt(0.75f) * (float)hexCoords.y;

      if(isTriangleFlipped(hexCoords)) {
	model.yw += 1.f/3.f * sqrt(0.75f);
	model.yy *= -1;
      }

      model.xx *= 0.95f;
      model.yy *= 0.95f;
            
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

    if(memoryInfo->debugTextActive) {
      char text[256];
      sprintf_s(text, "(%i,%i)->(%i,%i)", tile.xid, tile.yid,hexCoords.x, hexCoords.y);
      *renderMemory = renderObject{TEXT, model.translatedX(-0.5f), viewResult.view, identity3f,0,0.f, text};
      renderMemory++;
    }

    
  }


  
  //Player
  {
    mat4 model = identity4;
    mat3 basis = identity3f;

    ivec2 boardPos = levelInfo->player_pos_original;
    ivec2 hexCoords;

    if(memoryInfo->hexMode) {

      hexCoords = state->pos;

      model.xw = 0.5f * (float)hexCoords.x;
      model.yw = 0.5f * sqrt(0.75f) * (float)hexCoords.y;

      if(isTriangleFlipped(hexCoords)) {
	model.yw += 1.f/3.f * sqrt(0.75f);
	model.yy *= -1;
      }

      model.xx *= 0.95f;
      model.yy *= 0.95f;


      assert(animation_basis.zx == 0);
      assert(animation_basis.zy == 0);
      assert(animation_basis.zz == 1);

      vec2 xcol = vec2{animation_basis.xx, animation_basis.yx};
      //xcol = hexLinearSpaceToRenderSpace(xcol);
      vec2 ycol = vec2{animation_basis.xy, animation_basis.yy};
      //ycol = hexLinearSpaceToRenderSpace(ycol);
      vec2 zcol = vec2{animation_basis.xz, animation_basis.yz};
      //zcol = hexLinearSpaceToRenderSpace(zcol);

      //std::printf("X: (%f,%f) Y: (%f,%f) Z:(%f,%f)\n", xcol.x, xcol.y, ycol.x, ycol.y, zcol.x, zcol.y);
      
      basis = {
	xcol.x, ycol.x, zcol.x,
	xcol.y, ycol.y, zcol.y,
	0, 0, 1
      };

      //basis = animation_basis;

      //std::cout << "Basis\n";
      //printMat3(basis);

    } else {
      model.xw = (float) levelInfo->player_pos_original.x;
      model.yw = (float) levelInfo->player_pos_original.y;
      basis = animation_basis;
    }
    
    *renderMemory = renderObject {PLAYER, model, viewResult.view, basis};
    renderMemory++;


    

    //std::printf("Target player pos (%f, %f)\n", playerPosTemp.x, playerPosTemp.y);
    

    if(memoryInfo->debugTextActive) {

      char text[256];

      vec3 playerPosTemp = vec3{(float)hexCoords.x, (float)hexCoords.y, 1.f};
      playerPosTemp = basis * playerPosTemp;

      mat4 textModel = identity4.translated(playerPosTemp.x, playerPosTemp.y, 0.0f);
    
      sprintf_s(text, "(%i,%i)->(%i,%i)", boardPos.x, boardPos.y ,hexCoords.x, hexCoords.y);
      *renderMemory = renderObject{TEXT, textModel.translatedX(-0.5f), viewResult.view, identity3f,0,0.f, text};
      renderMemory++;

      sprintf_s(text, "(%f,%f)", playerPosTemp.x, playerPosTemp.y);
      *renderMemory = renderObject{TEXT, textModel.translated(-0.5f, 0.3f, 0.f), viewResult.view, identity3f,0,0.f, text};
      renderMemory++;
      

      /**
      for(int i=0; i<3; i++) {
	mat4 vertexPos = vec3{
	  (float) (hexCoords.x + player_vertices[3*i]),
	  (float) (hexCoords.y + player_vertices[3*i + 1]),
	  0.0f
	};
	
	vertexPos = basis * vertexPos;

	mat4 vertexTextModel = identity4.translated{vertexPos.x, vertexPos.y, 0.0f);
    
	sprintf_s(text, "(%f,%f)", vertexPos.x, vertexPos.y);
	*renderMemory = renderObject{TEXT, vertexTextModel, viewResult.view, identity3f,0,0.f, text};
	renderMemory++;
		 
      }
      **/

      
    }
    
  }
  
  //Second player
  if(levelInfo->hasSecondPlayer) {
    mat4 model = identity4;
    mat3 basis = identity3f;

    if(memoryInfo->hexMode) {
      vec2 hexCoords = hexBoardToRenderSpace(levelInfo->secondPlayerStartPos);
      model.xw = hexCoords.x;
      model.yw = hexCoords.y;

      //TODO: basis

    } else {
      model.xw = (float) levelInfo->secondPlayerStartPos.x;
      model.yw = (float) levelInfo->secondPlayerStartPos.y;
      basis = animation_basis;
    }
    
    *renderMemory = renderObject {PLAYER, model, viewResult.view, basis};
    renderMemory++; 
  }




  //Anchors


  //NOTE: this var is used to determine if valid mirror on mouse release
  bool passesThroughAnchor = false; 

  for(int yid=0; yid<BOARD_HEIGHT; yid++) {
    for(int xid=0; xid<BOARD_WIDTH; xid++) {
      if(levelInfo->board[yid][xid] != 0) {

	ivec2 hexCoords = ivec2{2*xid, 2*yid};
	if(hexCoords.y % 4 == 2) {
	  hexCoords.x += 1;
	}

	ivec2 diff;
	
	int highlight_key = 0;

	if(state->mirrorState==MIRROR_INACTIVE) {
	  ivec2 myVec = {xid,yid};

	  if(memoryInfo->hexMode) {

	    
	    if(nearestAnchor(levelInfo, mouse_coords, true) == myVec
	       && magnitude(mouse_coords - hexCoords) < MOUSE_HOVER_SNAP_DIST) {
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

	    ivec2 hexAnchor = hexBoardToLinearSpace(state->mirrorFragmentAnchor);
	    ivec2 hexPoint = hexBoardToLinearSpace(point);
	    
	    diff = hexPoint - hexAnchor;
	    //std::printf("Diff (%i, %i)\n", diff.x, diff.y);
	    
	    int mirrorId = state->hexMirrorAngleId;

	    if( (mirrorId==0 && diff.y==0) ||
	        (mirrorId==3 && diff.x==0) ||

		(mirrorId==2 && diff.x!=0 && diff.y!=0 &&
		 diff.x / 1 == diff.y / 2) ||

		(mirrorId==4 && diff.x!=0 && diff.y!=0 &&
		 diff.x / -1 == diff.y / 2) ||

		(mirrorId==1 && diff.x!=0 && diff.y!=0 &&
		 (float)diff.x / 3.f == (float)diff.y / 2.f) ||

		(mirrorId==5 && diff.x!=0 && diff.y!=0 &&
		 (float)diff.x / -3.f == (float)diff.y / 2.f)
		
	       ){
	      
	      vec2 mirrorAnchor = hexBoardToRenderSpace(state->mirrorFragmentAnchor);
	      vec2 mirrorFragRelative = {
		state->mirrorFragmentMag * cos(state->hexMirrorAngleId * PI/6),
		state->mirrorFragmentMag * sin(state->hexMirrorAngleId * PI/6),
	      };
	      vec2 mirrorFrag = hexBoardToRenderSpace(state->mirrorFragmentAnchor)
		+ mirrorFragRelative;
	      	      
	      float leftBound = min(mirrorAnchor.x, mirrorFrag.x);
	      float rightBound = max(mirrorAnchor.x, mirrorFrag.x);
	      float bottomBound = min(mirrorAnchor.y, mirrorFrag.y);
	      float topBound = max(mirrorAnchor.y, mirrorFrag.y);
	      
	      vec2 p = hexBoardToRenderSpace(point);

	      //std::printf("L %f R %f B %f T %f Px %f Py %f\n",
	      //leftBound,rightBound,bottomBound,topBound,p.x,p.y);	      

	      if( (mirrorId==3 || (p.x > leftBound && p.x < rightBound)) &&
		  (mirrorId==0 || (p.y > bottomBound && p.y < topBound)) ) {
		 
		passesThroughLine = true;
		
		passesThroughAnchor = true;
	      }
	      
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

		passesThroughAnchor = true;
	      }	    
							    
	    }	    

	  }

	  if(isTheAnchor || passesThroughLine) {
	    highlight_key = 1;	      
	  }
	  
	}
	
	mat4 anchorModel = identity4;

	if(memoryInfo->hexMode) {
	  //vec2 hexCoords = hexBoardToRenderSpace(ivec2{xid,yid});

	  /**
	  ivec2 hexCoords = ivec2{2*xid, 2*yid};
	  if(hexCoords.y % 4 == 2) {
	    hexCoords.x += 1;
	  }
	  */

	  
	  anchorModel.xw = 0.5f * (float)hexCoords.x;
	  anchorModel.yw = 0.5f * sqrt(0.75f) * (float)hexCoords.y;

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

	if(memoryInfo->debugTextActive) {

	  mat4 textModel = anchorModel;
	  //textModel.xw -= 0.5;
	  char text[256];
	  sprintf_s(text, "(%i,%i)>(%i,%i)", xid, yid, hexCoords.x, hexCoords.y);
	  *renderMemory = renderObject{TEXT, textModel, viewResult.view, identity3f,0,0.f, text};
	  renderMemory++;
	}


	
	    
      }
    }
  }
  


  
  //Mirror
   
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
      
      if(passesThroughAnchor) {
	
	state->mirrorState = MIRROR_LOCKED;
	state->mirrorTimeElapsed = 0.f;

	if(memoryInfo->hexMode) {  
	  //reflectAlongHexMode(state, state->mirrorFragmentAnchor,
	  //			      state->hexMirrorAngleId);

	  state->pos = findNewPlayerPos(state, state->pos);
	  state->is_animation_active = false;
	  onPlayerMovementFinished(memoryInfo, levelInfo, state, input.currentTime); 


	  
	  
	} else {
	  reflect_along(state, state->mirrorFragmentAnchor,
			state->mirrorFragmentAngle);
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


      float theta = (float)state->hexMirrorAngleId * PI/6.f;
      theta = fmod(theta + 4*PI, PI);

      float xdir = cos(theta);
      float ydir = sin(theta);
      //std::cout << "Theta "<<theta<<" Mirror dir "<< xdir<<" "<<ydir<<"\n";

      vec2 mirrorDir = vec2 {xdir, ydir};
      
      mirrorExtension = extensionMag * magSign * mirrorDir;
      diff = state->mirrorFragmentMag * mirrorDir;

      //vec2 diffToMouse =mouse_coords-hexBoardToRenderSpace(state->mirrorFragmentAnchor);

      /*
      std::cout << "Mag target "<<state->mirrorFragmentMag<<
	" diff "<<magnitude(diff)<<
	" mouse "<< magnitude(diffToMouse)<<"\n";
      */

    } else {
      mirrorExtension = extensionMag * magSign * toVec(state->mirrorFragmentAngle);
      diff = state->mirrorFragmentMag * toVec(state->mirrorFragmentAngle);
    }
    
    float x1, y1, x2, y2;
    vec2 thickness_offset_dir;
    thickness_offset_dir = vec2(diff.y, -1 * diff.x);
    thickness_offset_dir = normalize(thickness_offset_dir);

    if(memoryInfo->hexMode) {

      vec2 hexAnchor = hexBoardToLinearSpace(state->mirrorFragmentAnchor);

      x1 = hexAnchor.x - mirrorExtension.x;
      y1 = hexAnchor.y - mirrorExtension.y;
      

    } else {
      x1 = (float) state->mirrorFragmentAnchor.x - mirrorExtension.x;
      y1 = (float) state->mirrorFragmentAnchor.y - mirrorExtension.y;
    }
    
    x2 = x1 + diff.x + 1.5f * mirrorExtension.x;
    y2 = y1 + diff.y + 1.5f * mirrorExtension.y;
    //std::printf("Mirror segment: (%f, %f) to (%f, %f) Thickness dir: (%f, %f)\n", x1, y1, x2, y2, thickness_offset_dir.x, thickness_offset_dir.y);
      
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


  //Game-specific log values
  if(memoryInfo->debugTextActive) {
    char text[256];
    
    ivec2 hexAnchor = hexBoardToLinearSpace(state->mirrorFragmentAnchor);
    sprintf_s(text, "Mirror anchor:  board(%i,%i)   hex(%i, %i)",
	      state->mirrorFragmentAnchor.x, state->mirrorFragmentAnchor.y,
	      hexAnchor.x, hexAnchor.y);
    *renderMemory = renderObject{TEXT, identity4.translated(0.5f, 0.95f, 0.f),
      identity4,identity3f,0,0.f, text};
    renderMemory++;
    
    ivec2 hexPlayerPos = hexBoardToLinearSpace(levelInfo->player_pos_original);
    sprintf_s(text, "Original player pos:  board(%i,%i)   hex(%i,%i)",
	      levelInfo->player_pos_original.x, levelInfo->player_pos_original.y,
	      hexPlayerPos.x, hexPlayerPos.y);
    *renderMemory = renderObject{TEXT, identity4.translated(0.5f, 0.90f, 0.f),
      identity4,identity3f,0,0.f, text};
    renderMemory++;

    sprintf_s(text, "Current player pos (in linear space): (%i,%i)",
	      state->pos.x, state->pos.y);
    *renderMemory = renderObject{TEXT, identity4.translated(0.5f, 0.85f, 0.f),
      identity4,identity3f,0,0.f, text};
    renderMemory++;

    
    sprintf_s(text, "Player flipped: %i", isTriangleFlipped(state->pos));
    *renderMemory = renderObject{TEXT, identity4.translated(0.5f, 0.80f, 0.f),
      identity4,identity3f,0,0.f, text};
    renderMemory++;


    


    
    ivec2 mirrorPos = hexBoardToLinearSpace(state->mirrorFragmentAnchor);


    /**
    int playerDiagRaw = 2*state->pos.x - state->pos.y;
    int playerDiagOffset = (int)floor( ceil((float)playerDiagRaw/2.f) / 2.f);
    int playerDiag = playerDiagRaw - playerDiagOffset;
   
    int mirrorDiag = 2 * mirrorPos.x - mirrorPos.y;

    sprintf_s(text, "Player diag: %i    Mirror diag: %i",
	      playerDiag, mirrorDiag);
    *renderMemory = renderObject{TEXT, identity4.translated(0.5f, 0.80f, 0.f),
      identity4,identity3f,0,0.f, text};
    renderMemory++;

    sprintf_s(text, "Player diag offset: %i", playerDiagOffset);
    *renderMemory = renderObject{TEXT, identity4.translated(0.5f, 0.75f, 0.f),
      identity4,identity3f,0,0.f, text};
    renderMemory++;

    **/
    
  }
  

  //Make sure that we didn't overuse memory in our render buffer
  assert(renderMemory - renderMemoryInfo->memory < renderMemoryInfo->count);


  //std::cout << "Mirror state: " << state->mirrorState << "\n";
  //std::cout << "Current time: " << input.currentTime << "\n";
  //std::cout << "Weight: "<< state->weight << "\n";


}
