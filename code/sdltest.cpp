
#include "SDL/SDL.h"
//#include <SDL/SDL_opengl.h>
#include <glad/gl.h>

//#include <gl/gl.h>

//#include <iostream>


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

#define PI 3.1415926535897932384626433f


#include "reflect.h"

#include "reflect_temp.cpp"

//TODO: don't include this
#include <iostream>

#include <string>
#include <fstream>
#include <sstream>


#define global static


const int amplitude = 28000;
const int sampleRate = 48000;

int screenWidth = 1280;
int screenHeight = 720;


void audioCallback(void*  userdata, uint8* rawStream, int lengthInBytes) {
  int16 *stream = (int16 *) rawStream;
  int streamLengthInSamples = lengthInBytes  / 2; //2 bytes per sample

  int &samplePos (*(int *) userdata);

  for(int i=0; i < streamLengthInSamples; ++i) {
    double time = ((double) samplePos) / ((double) sampleRate); // in seconds
    *(stream + i) = (int16) (amplitude * sin(2.0f * PI * 260.0f * time));

    samplePos++;			     
  }
}


unsigned int loadShaderFromFile(const char *filePath) {    
    // 1. retrieve the vertex/fragment source code from filePath    
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream shaderFile;
    //ensure ifstream objects can throw exceptions
    shaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);

    //TODO: remove exception here so that can disable exceptions in build file
    try {
      shaderFile.open(filePath);
      std::stringstream shaderStream;
      shaderStream << shaderFile.rdbuf(); //read file's buffer contents into streams
      shaderFile.close(); //close file handlers
      vertexCode = "#version 330 core\n#define VERTEX_PROGRAM\n" + shaderStream.str();
      fragmentCode = "#version 330 core\n#define FRAGMENT_PROGRAM\n" + shaderStream.str();
    } catch (std::ifstream::failure& e) {
      std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << std::endl;
    }
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    // 2. compile shaders
    unsigned int vertex, fragment;
    int success;
    char infoLog[512];

    //vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    //print compile errors if any
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if(!success) {
      glGetShaderInfoLog(vertex, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    };
    
    //fragment shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    //print compile errors if any
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if(!success) {
      glGetShaderInfoLog(fragment, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    };

    //shader program
    unsigned int ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    //print linking errors if any
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if(!success) {
      glGetProgramInfoLog(ID, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    //delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    
    return ID;
}

renderObject objects[500];
int activeObjects = 0;

int createNewRenderObject(float vertices[], int verticesLength, unsigned int indices[], int indicesLength, char *filePath) {

  renderObject obj = {};

  glGenVertexArrays(1, &(obj.vaoID));
  glGenBuffers(1, &(obj.vboID));
  glGenBuffers(1, &(obj.eboID));

  glBindVertexArray(obj.vaoID);
  glBindBuffer(GL_ARRAY_BUFFER, obj.vboID);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.eboID);

  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * verticesLength, vertices, GL_STATIC_DRAW);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indicesLength, indices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float),(void*) 0);
  glEnableVertexAttribArray(0);

  obj.shaderID = loadShaderFromFile(filePath);
  obj.indicesCount = indicesLength;

  obj.gameObj = {};
  
  objects[activeObjects] = obj;
  activeObjects++;
  return (activeObjects-1);
}


gameObject* getObject(int id) {
  return &(objects[id].gameObj);
}


/*
int getAllObjects(gameObject *allObjects) {
  for(int i=0; i<activeObjects; i++) {
    *(allObjects + i)  = objects[i].gameObj;
  }

  return activeObjects;
}
*/

int getObjectCount() {
  return activeObjects;
}


void clearAllObjects() {
  SDL_memset(&objects, 0, sizeof(renderObject) * 500);
  activeObjects = 0;
}





//TODO: figure out how this program starts. Why do we need wmain?
int wmain(int argc, char* args[])
{
  if(SDL_Init( SDL_INIT_EVERYTHING ) == 0) {

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);

    SDL_Window *window = SDL_CreateWindow("Reflection Game",
					  SDL_WINDOWPOS_UNDEFINED,
					  SDL_WINDOWPOS_UNDEFINED,
					  screenWidth, screenHeight,
					  SDL_WINDOW_OPENGL); //TODO: SDL_WINDOW_FULLSCREEN    
    if(window) {

      SDL_GLContext context = SDL_GL_CreateContext(window);
      if(context) {


	//TODO: account for possible failure
	gladLoadGL((GLADloadfunc) SDL_GL_GetProcAddress);

	SDL_AudioSpec want;
	SDL_AudioSpec have;
	SDL_AudioDeviceID device;

	//	int samplePos = 0;

	SDL_memset(&want, 0, sizeof(want)); /* or SDL_zero(want) */
	want.freq = 48000;
	want.format = AUDIO_S16SYS;
	want.channels = 1;
	want.samples = 4096;
	//want.callback = audioCallback;
	want.callback = NULL;
	//want.userdata = &samplePos;

	//TODO: account for available sound card format
	device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0); //SDL_AUDIO_ALLOW_FORMAT_CHANGE	

	SDL_PauseAudioDevice(device, 0);	
	
	//Synchronize buffer swap with monitor's vertical refresh
	SDL_GL_SetSwapInterval(1);

	//TODO: look into different allocator than malloc
	void *renderStateRaw = malloc(sizeof(renderObject)* 100);
	renderObject *renderState = (renderObject *) renderStateRaw;
	SDL_memset(renderState, 0, sizeof(renderObject) * 100);



	gameMemory memory;
	memory.isInitialized = false;
	memory.permanentStorage = malloc(10 * 1024);
	memory.temporaryStorage = (void *)((char *) memory.permanentStorage + 5*1024);
	SDL_memset(memory.permanentStorage, 0, 10 * 1024);


	/*
	uint64 timerFrequency = SDL_GetPerformanceFrequency();
	uint64 timerCount = SDL_GetPerformanceCounter();
	*/

	uint32 prevTime = SDL_GetTicks();
	

	/*
	int availableShaders[4];
	availableShaders[0] = loadShaderFromFile("shaders/testShader.shader");
	*/


	int samplesAheadTarget = (int) (sampleRate * 0.1f); // 1/10 of a sec ahead
	int samplePos = 0;
	
	bool running = true;
	while(running) {

	  controllerInput mainController = {};
	  //mainController = {};

	  SDL_Event event;
	  while(SDL_PollEvent(&event)) {
	    switch(event.type) {
	    case SDL_QUIT: {
	      running = false;
	    } break;

	    case SDL_MOUSEMOTION:
	      {
	      } break;

	    case SDL_MOUSEBUTTONDOWN:
	    case SDL_MOUSEBUTTONUP:
	      {
		switch(event.button.button) {
		case SDL_BUTTON_LEFT:
		  {
		    mainController.mouseLeft.transitionCount += 1;
		    mainController.mouseLeft.endedDown = (event.type == SDL_MOUSEBUTTONDOWN);
		  } break;
		case SDL_BUTTON_RIGHT:
		  {

		  } break;
		}
	      } break;	      
	    
	    case SDL_KEYDOWN:
	    case SDL_KEYUP:
	      {
		if(event.type == SDL_KEYDOWN && event.key.keysym.scancode==SDL_SCANCODE_ESCAPE) {
		  running = false;
		}
		if(event.key.keysym.scancode==SDL_SCANCODE_R) {
		  mainController.rKey.transitionCount += 1;
		  mainController.rKey.endedDown = (event.type==SDL_KEYUP);
		}
		if(event.key.keysym.scancode==SDL_SCANCODE_LEFT) {
		  mainController.leftArrow.transitionCount += 1;
		  mainController.leftArrow.endedDown = (event.type==SDL_KEYUP);
		}
		if(event.key.keysym.scancode==SDL_SCANCODE_RIGHT) {
		  mainController.rightArrow.transitionCount += 1;
		  mainController.rightArrow.endedDown = (event.type==SDL_KEYUP);
		}		
	      } break;
	      
	    
	    }
	  }
	  int32 mouseX;
	  int32 mouseY;
	  SDL_GetMouseState(&mouseX, &mouseY);

	  mainController.mouseX = mouseX;
	  mainController.mouseY = mouseY;
	  //std::cout << "Mouse x " << mainController.mouseX << "\n";
	  //std::cout << "Mouse y " << mainController.mouseY << "\n";


	  int bytesLeftInQueue = SDL_GetQueuedAudioSize(device);
	  int samplesLeftInQueue = bytesLeftInQueue / 2;
	  int samplesToAppend = 0;
	  if(samplesLeftInQueue < samplesAheadTarget) {
	    samplesToAppend = samplesAheadTarget - samplesLeftInQueue;
	  }

	  gameInput input;
	  input.screenWidth = screenWidth;
	  input.screenHeight = screenHeight;
	  input.controllers[0] = mainController;


	  uint32 currentTime = SDL_GetTicks();
	  //uint32 deltaTime = currentTime - prevTime;
	  prevTime = currentTime;

	  std::cout << "Duraton: "<<deltaTime<< " ms\n";
	  
	  input.currentTime = currentTime;
	  
	  gameUpdateAndRender(input, &memory);

	  
	  int16 *stream = new int16[samplesToAppend];

	  for(int i=0; i < samplesToAppend; ++i) {
	    double time = ((double) samplePos) / ((double) sampleRate); // in seconds
	    *(stream + i) = (int16) (amplitude * sin(2.0f * PI * 260.0f * time));

	    samplePos++;			     
	  }

	  SDL_QueueAudio(device, (void *)stream, samplesToAppend * 2);

	  
	  glViewport(0, 0, 1280, 720);
	  glClearColor(1.f, 0.f, 1.f, 1.f);
	  glClear(GL_COLOR_BUFFER_BIT);

	  /*
	  renderObject *objects = (renderObject *) renderState;
	  renderObject obj = *objects;
	  */

	  //myShader.use();

	  for(int i=0; i<activeObjects; i++) {
	    renderObject obj = objects[i];
	    
	    glUseProgram(obj.shaderID);
	    
	    TEMPmat4 modelMatData = transpose(obj.gameObj.model);
	    glUniformMatrix4fv(glGetUniformLocation(obj.shaderID, "model"), 1, GL_FALSE, &modelMatData.xx);

	    TEMPmat4 viewMatData = transpose(obj.gameObj.view);
	    glUniformMatrix4fv(glGetUniformLocation(obj.shaderID, "view"), 1, GL_FALSE, &viewMatData.xx);
	  
	    glBindVertexArray(obj.vaoID);
	    glBindBuffer(GL_ARRAY_BUFFER, obj.vboID);
	    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.eboID);
	    glDrawElements(GL_TRIANGLES, obj.indicesCount, GL_UNSIGNED_INT, 0);
	    
	  }

	  SDL_GL_SwapWindow(window);
	}

	//SDL_GL_DeleteContext(glcontext);
	SDL_CloseAudioDevice(device);


      } else {
	//TODO: logging: gl context creation has failed
      }
      
      
    } else {

      //TODO: logging: sdl failed to make window
    
    }
    
    SDL_Quit();

  } else {
    //TODO: logging: sdl init failed
  }


  return 0;
}
