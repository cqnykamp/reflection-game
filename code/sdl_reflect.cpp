//TODO: find memory leak

#include "SDL/SDL.h"
#include <glad/gl.h>


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
#include "reflect.cpp"

//TODO: don't include this
#include <iostream>

#include <string>
#include <fstream>
#include <sstream>


#define global static

const int amplitude = 0;//10000;
const int sampleRate = 48000;

int screenWidth = 1280;
int screenHeight = 720;

unsigned int vao[10];
unsigned int vbo[10];
unsigned int ebo[10];
unsigned int shader[10];
unsigned int indicesCount[10];


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


struct NewRenderObject {
  unsigned int vao;
  unsigned int vbo;
  unsigned int ebo;
  unsigned int shader;
  unsigned int indicesCount;
};
NewRenderObject createNewRenderObject(float vertices[], int verticesLength, unsigned int indices[], int indicesLength, char *filePath) {

  NewRenderObject obj = {};

  glGenVertexArrays(1, &(obj.vao));
  glGenBuffers(1, &(obj.vbo));
  glGenBuffers(1, &(obj.ebo));

  glBindVertexArray(obj.vao);
  glBindBuffer(GL_ARRAY_BUFFER, obj.vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.ebo);

  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * verticesLength, vertices, GL_STATIC_DRAW);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indicesLength, indices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float),(void*) 0);
  glEnableVertexAttribArray(0);

  obj.shader = loadShaderFromFile(filePath);
  obj.indicesCount = indicesLength;

  return obj;
}


void updateRenderContextVertices(RenderContext context, float *vertices, int verticesLength) {
  //TODO: don't use GL_STATIC_DRAW
  glBindVertexArray(vao[context]);
  glBindBuffer(GL_ARRAY_BUFFER, vbo[context]);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * verticesLength, vertices);
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

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	SDL_AudioSpec want;
	SDL_AudioSpec have;
	SDL_AudioDeviceID device;

	//	int samplePos = 0;

	SDL_memset(&want, 0, sizeof(want)); /* or SDL_zero(want) */
	want.freq = 48000;
	want.format = AUDIO_S16SYS;
	want.channels = 1;
	want.samples = 4096;
	want.callback = NULL;

	//TODO: account for available sound card format
	device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0); //SDL_AUDIO_ALLOW_FORMAT_CHANGE	

	SDL_PauseAudioDevice(device, 0);	
	
	//Synchronize buffer swap with monitor's vertical refresh
	SDL_GL_SetSwapInterval(1);

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
	NewRenderObject bgObj = createNewRenderObject(background_vertices, 12, background_indices, 6, "shaders/background.shader");
	vao[BACKGROUND] = bgObj.vao;
	vbo[BACKGROUND] = bgObj.vbo;
	ebo[BACKGROUND] = bgObj.ebo;
	shader[BACKGROUND] = bgObj.shader;
	indicesCount[BACKGROUND] = bgObj.indicesCount;
	
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

	NewRenderObject floorObj = createNewRenderObject(floor_vertices, 12, floor_indices, 6, "shaders/floorTile.shader");
	vao[FLOOR_TILE] = floorObj.vao;
	vbo[FLOOR_TILE] = floorObj.vbo;
	ebo[FLOOR_TILE] = floorObj.ebo;
	shader[FLOOR_TILE] = floorObj.shader;
	indicesCount[FLOOR_TILE] = floorObj.indicesCount;

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
	NewRenderObject playerObj = createNewRenderObject(player_vertices, 12, player_indices, 6, "shaders/player.shader");
	vao[PLAYER] = playerObj.vao;
	vbo[PLAYER] = playerObj.vbo;
	ebo[PLAYER] = playerObj.ebo;
	shader[PLAYER] = playerObj.shader;
	indicesCount[PLAYER] = playerObj.indicesCount;

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
	NewRenderObject anchorObj = createNewRenderObject(anchor_vertices, 12, anchor_indices, 6, "shaders/anchor.shader");
	vao[ANCHOR] = anchorObj.vao;
	vbo[ANCHOR] = anchorObj.vbo;
	ebo[ANCHOR] = anchorObj.ebo;
	shader[ANCHOR] = anchorObj.shader;
	indicesCount[ANCHOR] = anchorObj.indicesCount;	

	
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
	NewRenderObject mirrorObj = createNewRenderObject(mirror_vertices, 12, mirror_indices, 6, "shaders/mirror.shader");
	vao[MIRROR] = mirrorObj.vao;
	vbo[MIRROR] = mirrorObj.vbo;
	ebo[MIRROR] = mirrorObj.ebo;
	shader[MIRROR] = mirrorObj.shader;
	indicesCount[MIRROR] = mirrorObj.indicesCount;

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
	NewRenderObject frontGridObj = createNewRenderObject(front_grid_vertices, 12, front_grid_indices, 6, "shaders/frontGrid.shader");
	vao[FRONT_GRID] = frontGridObj.vao;
	vbo[FRONT_GRID] = frontGridObj.vbo;
	ebo[FRONT_GRID] = frontGridObj.ebo;
	shader[FRONT_GRID] = frontGridObj.shader;
	indicesCount[FRONT_GRID] = frontGridObj.indicesCount;


       
	//TODO: look into different allocator than malloc
	int maxRenderObjects = 1000;
	void *renderMemoryRaw = malloc(sizeof(renderObject) * maxRenderObjects);
	renderObject *renderObjects = (renderObject *) renderMemoryRaw;
	SDL_memset(renderObjects, 0, sizeof(renderObject) * maxRenderObjects);	
	RenderMemoryInfo renderMemoryInfo = {maxRenderObjects, renderObjects};


	gameMemory memory;
	memory.isInitialized = false;
	memory.permanentStorage = malloc(10 * 1024);
	memory.temporaryStorage = (void *)((char *) memory.permanentStorage + 5*1024);
	SDL_memset(memory.permanentStorage, 0, 10 * 1024);

	
	uint64 timerFrequency = SDL_GetPerformanceFrequency();
	uint64 prevTime = SDL_GetPerformanceCounter();
	
	int samplesAheadTarget = (int) (sampleRate * 0.1f); // 1/10 of a sec ahead
	int samplePos = 0;
	
	bool running = true;
	while(running) {

	  SDL_memset(renderMemoryInfo.memory, 0, sizeof(renderObject) * renderMemoryInfo.count);

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

	  uint64 timerCount = SDL_GetPerformanceCounter();
	  uint64 deltaCount = timerCount - prevTime;	  
	  uint32 deltaTime = (uint32) (1000.f * (float)deltaCount / (float)timerFrequency);
	  uint32 currentTime = (uint32) (1000.f * (float) timerCount / (float)timerFrequency); 
	  input.currentTime = currentTime;
	  input.deltaTime = deltaTime;
	  prevTime = timerCount;
	  //std::cout << "Duraton: "<<deltaTime<< " ms\n";

	  
	  gameUpdateAndRender(input, &memory, &renderMemoryInfo);

	  
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


	  for(int i=0; i<renderMemoryInfo.count; i++) {
	    
	    renderObject obj = *(renderMemoryInfo.memory + i);

	    if(obj.renderContext) { //aka if game has put something here
	    
	      glBindVertexArray(vao[obj.renderContext]);
	      glBindBuffer(GL_ARRAY_BUFFER, vbo[obj.renderContext]);
	      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo[obj.renderContext]);
	      glUseProgram(shader[obj.renderContext]);

	      mat4 modelMatData = transpose(obj.model);
	      glUniformMatrix4fv(glGetUniformLocation(shader[obj.renderContext], "model"), 1, GL_FALSE, &modelMatData.xx);
	      mat4 viewMatData = transpose(obj.view);
	      glUniformMatrix4fv(glGetUniformLocation(shader[obj.renderContext], "view"), 1, GL_FALSE, &viewMatData.xx);
	      mat3 basisMatData = transpose(obj.basis);
	      glUniformMatrix3fv(glGetUniformLocation(shader[obj.renderContext], "basis"), 1, GL_FALSE, &basisMatData.xx);

	      glUniform1i(glGetUniformLocation(shader[obj.renderContext], "highlight_key"), obj.highlight_key);
	    
	      glDrawElements(GL_TRIANGLES, indicesCount[obj.renderContext], GL_UNSIGNED_INT, 0);

	    }
	    
	  }

	  SDL_GL_SwapWindow(window);
	}

	//SDL_GL_DeleteContext(glcontext);
	SDL_CloseAudioDevice(device);


      } else {
	//TODO: logging: gl context creation has failed
      }

      SDL_DestroyWindow(window);
      
      
    } else {

      //TODO: logging: sdl failed to make window
    
    }
    
    SDL_Quit();

  } else {
    //TODO: logging: sdl init failed
  }


  return 0;
}
