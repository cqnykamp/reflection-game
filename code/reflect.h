#if !defined(REFLECT_H)
#define REFLECT_H


//TODO: define internal and external run mode
// and filter out asserts

#include "gameutil.cpp"
#include "sprite.cpp"

#define PI 3.1415926535897932384626433f

// Assert
#ifdef REFLECT_INTERNAL
#define assert(expression) \
  if(!(expression)) {*(int *)0 = 0;}
#else
#define assert
#endif


#include<vector>

//TODO: Why do these need #include<vector>
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef int32 bool32;

typedef float real32;
typedef double real64;

#include <string>


struct LoadedLevel {
  char tileData[256];
  char pointData[256];
  int tileDataCount;
  int pointDataCount;
};



enum RenderContext {
  //NOTE: 0 means nothing there, don't draw
  TEXT = 1,
  PLAYER = 2,
  BACKGROUND = 3,
  FLOOR_TILE = 4,
  FRONT_GRID = 5,
  MIRROR = 6,
  ANCHOR = 7,

  //GOAL,
};


#define LOAD_LEVEL_FROM_FILE(name) void name(int levelNum, LoadedLevel *data)
typedef LOAD_LEVEL_FROM_FILE(LoadLevelFromFile);

// #define UPDATE_RENDER_CONTEXT_VERTICES(name) void name (RenderContext context, float *vertices, int verticesLength)
// typedef UPDATE_RENDER_CONTEXT_VERTICES(UpdateRenderContextVertices);

#define DEBUG_LOG(name) void name(char text[])
typedef DEBUG_LOG(DebugLog);

#define CREATE_NEW_RENDER_OBJECT(name) void name(float vertices[], int verticesLength, unsigned int indices[], int indicesLength, char *filePath, RenderContext context, char *textureName, bool outputWithTransparency)
typedef CREATE_NEW_RENDER_OBJECT(CreateNewRenderObject);



struct buttonState {
  int transitionCount;
  bool endedDown;
};

struct controllerInput {
  buttonState rKey;
  buttonState leftArrow;
  buttonState rightArrow;

  buttonState mouseLeft;
  buttonState mouseRight;
  int32 mouseX;
  int32 mouseY;
};

struct gameInput {
  int screenHeight;
  int screenWidth;
  controllerInput controllers[4];
};


struct renderObject {
  RenderContext renderContext;
  mat4 model;
  mat4 view;
  mat3 basis;
  int highlight_key;
  float alpha;
  std::string text;
  Sprite sprite;
};


struct gameMemory {
  bool isInitialized;
  bool isDllFirstFrame;
  bool hexMode;
  bool debugTextActive;

  int permanentStorageSize;
  int temporaryStorageSize;
  void *permanentStorage;
  void *temporaryStorage;

  LoadLevelFromFile *loadLevelFromFile;
  // UpdateRenderContextVertices *updateRenderContextVertices;
  DebugLog *debugLog;
  CreateNewRenderObject *createNewRenderObject;

};


struct RenderMemoryInfo {
  int count;
  renderObject *memory;
};


//Should never be recorded by looped data
struct FrameInfo {
  uint32 currentTime;
  uint32 deltaTime;
  uint32 innerDeltaTime;
};


#define GAME_UPDATE_AND_RENDER(name) void name(gameInput input, FrameInfo frameInfo, gameMemory *memoryInfo, RenderMemoryInfo *renderMemoryInfo)

typedef GAME_UPDATE_AND_RENDER(GameUpdateAndRender);

GAME_UPDATE_AND_RENDER(gameUpdateAndRenderStub) {
  return;
}



#endif
