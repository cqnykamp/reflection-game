#if !defined(REFLECT_H)


#include "gameutil.cpp"

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
  controllerInput controllers[4];
};

struct gameObject {
  TEMPmat4 model;
  TEMPmat4 view;
};

struct renderObject {
  unsigned int vaoID;
  unsigned int vboID;
  unsigned int eboID;
  unsigned int shaderID;
  int indicesCount;

  gameObject gameObj;

  //  TEMPmat4 model;

  bool isInitialized;
  //bool dynamicDraw;
  
};


struct gameMemory {
  bool isInitialized;
  void *permanentStorage;
  void *temporaryStorage;
};





int createNewRenderObject(float vertices[], int verticesLength, unsigned int indices[], int indicesLength, char *filePath);

gameObject* getObject(int id);

int getObjectCount();

void clearAllObjects();


#define REFLECT_H
#endif
