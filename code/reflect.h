#if !defined(REFLECT_H)

//TODO: define internal and external run mode
// and filter out asserts

#include "gameutil.cpp"

#define assert(expression) \
  if(!(expression)) {*(int *)0 = 0;}


struct LoadedLevel {
  char tileData[512];
  char pointData[512];
  int tileDataCount;
  int pointDataCount;
};



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
  uint32 currentTime;
  uint32 deltaTime;
  controllerInput controllers[4];
};


enum RenderContext {
  //NOTE: 0 means nothing there, don't draw
  PLAYER = 1,
  BACKGROUND = 2,
  FLOOR_TILE = 3,
  FRONT_GRID = 4,
  MIRROR = 5,
  ANCHOR = 6,
  //GOAL,
};


struct renderObject {
  RenderContext renderContext;
  mat4 model;
  mat4 view;
  mat3 basis;
  int highlight_key;
};


struct gameMemory {
  bool isInitialized;
  int permanentStorageSize;
  int temporaryStorageSize;
  void *permanentStorage;
  void *temporaryStorage;
};


struct RenderMemoryInfo {
  int count;
  renderObject *memory;
};

void loadLevelFromFile(int levelNum, LoadedLevel *data);

void updateRenderContextVertices(RenderContext context, float *vertices, int verticesLength);


#define REFLECT_H
#endif
