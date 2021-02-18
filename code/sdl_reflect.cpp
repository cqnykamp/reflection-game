
//#define SDL_MAIN_HANDLED
#include "SDL/SDL.h"
#include <glad/gl.h>


#include "reflect.h"

//TODO: don't include this
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

#include <windows.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#define global static

#define PERMANENT_STORAGE_SIZE (5 * 1024)
#define TEMPORARY_STORAGE_SIZE (1 * 1024 * 1024)





const int amplitude = 0;//10000;
const int sampleRate = 48000;

int screenWidth = 1280;
int screenHeight = 720;

unsigned int vao[10];
unsigned int vbo[10];
unsigned int ebo[10];
unsigned int shader[10];
unsigned int indicesCount[10];


unsigned int fontShader;
unsigned int fontVao;
unsigned int fontVbo;

struct Character {
  unsigned int textureID;
  ivec2 size;
  ivec2 bearing;
  unsigned int advance;
};

Character characters[128];

char logText[4096];

int frameDurationLabel;
uint32 lastLabelTimestamp = 0;


struct GameCode {
  HMODULE gameHandle;
  GameUpdateAndRender *gameUpdateAndRender;
  bool isValid;
};

GameCode loadGameCode() {

  GameCode gameCode = {};

  //CopyFile("reflect.dll", "reflect_temp.dll");
  gameCode.gameHandle = LoadLibrary("reflect.dll");
  if(gameCode.gameHandle) {
    gameCode.gameUpdateAndRender = (GameUpdateAndRender *)
      GetProcAddress(gameCode.gameHandle, "gameUpdateAndRender");
    
    gameCode.isValid = (gameCode.gameUpdateAndRender);
  }

  if(!gameCode.isValid) {
    gameCode.gameUpdateAndRender = gameUpdateAndRenderStub;
  }

  return gameCode;

}

void unloadGameCode(GameCode *gameCode) {
  if(gameCode->gameHandle) {
    FreeLibrary(gameCode->gameHandle);
  }
  gameCode->isValid = false;
  gameCode->gameUpdateAndRender = gameUpdateAndRenderStub;
  
}
	





LOAD_LEVEL_FROM_FILE(loadLevelFromFile) {

  SDL_memset(data, 0, sizeof(LoadedLevel));

  if(std::ifstream levelFile{"levels"}) {
    
    int readingBlock = 0;
    bool onNumBlock = false;
  
    char lastChar = 'Q';
    char c;  
    while(levelFile.get(c)) {
      if(c=='0' || c=='1' || c=='2' || c=='3') {
	onNumBlock = true;

	if(readingBlock == 2*levelNum) {
	  //Maximum board space: 256
	  assert(data->tileDataCount < 256);
	  data->tileData[data->tileDataCount] = c;
	  data->tileDataCount++;
	} else if(readingBlock == 2*levelNum + 1) {
	  //Maximum board space: 256
	  assert(data->pointDataCount < 256);
	  data->pointData[data->pointDataCount] = c;
	  data->pointDataCount++;
	}
      
      } else if(c=='\n' && lastChar=='\n') {
	//new paragraph
	if(onNumBlock) {
	  readingBlock += 1;
	}
	onNumBlock = false;
	
      } else if(c=='\n' && onNumBlock) {
	//just single line break, continue as normal


	if(readingBlock == 2*levelNum) {
	  //Maximum board space: 256
	  assert(data->tileDataCount < 256);
	  data->tileData[data->tileDataCount] = c;
	  data->tileDataCount++;
	} else if(readingBlock == 2*levelNum + 1) {
	  //Maximum board space: 256
	  assert(data->pointDataCount < 256);
	  data->pointData[data->pointDataCount] = c;
	  data->pointDataCount++;
	}
           
      }

      lastChar = c;
    }
    if(levelFile.bad()) {
      perror("error while reading file");
    }

    try {
      levelFile.close();
    } catch(...) {
      //      std::cerr << ": " << strerror(errno);
    }


  } else {
    //TODO: logging, failed to open file
    //    std::cerr << "Error: " << strerror(errno);

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


UPDATE_RENDER_CONTEXT_VERTICES(updateRenderContextVertices) {
  //TODO: don't use GL_STATIC_DRAW
  glBindVertexArray(vao[context]);
  glBindBuffer(GL_ARRAY_BUFFER, vbo[context]);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * verticesLength, vertices);
}



void renderText(std::string text, float x, float y, float scale, vec3 color) {
  
  glUseProgram(fontShader);
  glUniform3f(glGetUniformLocation(fontShader, "textColor"), color.x, color.y, color.z);
  glActiveTexture(GL_TEXTURE0);
  glBindVertexArray(fontVao);

  for(int i=0; i<text.size(); i++) {
    char c = text[i];
    Character ch = characters[c];

    float xpos = x + ch.bearing.x * scale;
    float ypos = y - (ch.size.y - ch.bearing.y) * scale;

    float w = ch.size.x * scale;
    float h = ch.size.y * scale;
    
    float vertices[6][4] = {
      {xpos,     ypos + h, 0.f, 0.f},
      {xpos,     ypos,     0.f, 1.f},
      {xpos + w, ypos,     1.f, 1.f},

      {xpos,     ypos + h, 0.f, 0.f},
      {xpos + w, ypos,     1.f, 1.f},
      {xpos + w, ypos + h, 1.f, 0.f}

    };

    glBindTexture(GL_TEXTURE_2D, ch.textureID);

    glBindBuffer(GL_ARRAY_BUFFER, fontVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    x+= (ch.advance >> 6) * scale; //2^6 = 64 aka 1 pixel
    
  }

  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);  

}

//TODO: don't use string
DEBUG_LOG(debugLog) {
  SDL_memset(logText, 0, sizeof(char) * 4096);
  for(int i=0; i<4096; i++) {
    if(text[i] == '\0' || text[i] == '\n') {
      break;
    } else {
      logText[i] = text[i];
    }
  }
}


int wmain(int argc, char *argv[]) {

/**
int CALLBACK  WinMain(
		      HINSTANCE Instance,
		      HINSTANCE PrevInstance,
		      LPSTR     CommandLine,
		      int       ShowCode )
{
**/
  
  if(SDL_Init(SDL_INIT_VIDEO |
	      SDL_INIT_AUDIO |
	      SDL_INIT_TIMER |
	      SDL_INIT_EVENTS) == 0) {

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

	//Start playing audio
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
	RenderMemoryInfo renderMemoryInfo = {
	  maxRenderObjects,
	  renderObjects};



	//Game Memory
	gameMemory memory;
	memory.isInitialized = false;
	memory.permanentStorageSize = PERMANENT_STORAGE_SIZE;
	memory.temporaryStorageSize = TEMPORARY_STORAGE_SIZE;

	//Pass platform function pointers
	memory.loadLevelFromFile = loadLevelFromFile;
	memory.updateRenderContextVertices = updateRenderContextVertices;
	memory.debugLog = debugLog;

	/*
	memory.permanentStorage = malloc(PERMANENT_STORAGE_SIZE +
					 TEMPORARY_STORAGE_SIZE);
	memory.temporaryStorage = (void *)((char *) memory.permanentStorage +
					   PERMANENT_STORAGE_SIZE);
	*/

	memory.permanentStorage = malloc(PERMANENT_STORAGE_SIZE);
	memory.temporaryStorage = malloc(TEMPORARY_STORAGE_SIZE);

	SDL_memset(memory.permanentStorage, 0, memory.permanentStorageSize);
	SDL_memset(memory.temporaryStorage, 0, memory.temporaryStorageSize);



	GameCode gameCode = loadGameCode();

	

	//Audio
	int audioStreamMaxSize = (int)(sizeof(int16) * sampleRate * 0.5f); //enough for 1/2 second
	int16 *stream = (int16 *)malloc(audioStreamMaxSize);


	//Fonts
	FT_Library ft;
	if(FT_Init_FreeType(&ft)) {
	  std::cout << "ERROR::FREETYPE: Could not init FreeType Library\n";
	  return -1;
	}
	
	FT_Face face;
	if(FT_New_Face(ft, "arial.ttf", 0, &face)) {
	  std::cout << "ERROR::FREETYPE: Could not load font \n";
	  return -1;	  
	}

	FT_Set_Pixel_Sizes(face, 0, 48);

	if(FT_Load_Char(face, 'X', FT_LOAD_RENDER)) {
	  std::cout << "ERROR::FREETYPE: Failed to load initial glyph\n";
	  return -1;
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);


	for(unsigned char c = 0; c < 128; c++) {
	  if(FT_Load_Char(face, c, FT_LOAD_RENDER)) {
	    std::cout << "ERROR::FREETYPE: Failed to load glyph\n";
	    continue;
	  }
	  unsigned int texture;
	  glGenTextures(1, &texture);
	  glBindTexture(GL_TEXTURE_2D, texture);
	  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows,
		       0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

	  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	  Character character = {
	    texture,
	    ivec2{(int)face->glyph->bitmap.width, (int)face->glyph->bitmap.rows},
	    ivec2{(int)face->glyph->bitmap_left, (int)face->glyph->bitmap_top},
	    (unsigned int)face->glyph->advance.x
	  };

	  characters[c] = character;

	}

	FT_Done_Face(face);
	FT_Done_FreeType(ft);



	glGenVertexArrays(1, &fontVao);
	glGenBuffers(1, &fontVbo);
	glBindVertexArray(fontVao);
	glBindBuffer(GL_ARRAY_BUFFER, fontVbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	fontShader = loadShaderFromFile("shaders/font.shader");
	glUseProgram(fontShader);
	mat4 projection = mat4 {
	  2.f / (float) screenWidth, 0.f, 0.f, -1.f,
	  0.f, 2.f / (float)screenHeight, 0.f, -1.f,
	  0.f, 0.f, 1.f, 0.f,
	  0.f, 0.f, 0.f, 1.f
	};
	  
	projection = transpose(projection);
	glUniformMatrix4fv(glGetUniformLocation(fontShader, "projection"), 1, GL_FALSE, &projection.xx);

	uint64 timerFrequency = SDL_GetPerformanceFrequency();
	uint64 prevTime = SDL_GetPerformanceCounter();
	
	int samplesAheadTarget = (int) (sampleRate * 0.1f); // 1/10 of a sec ahead
	int samplePos = 0;
	
	bool running = true;
	while(running) {

	   
	  

	  SDL_memset(renderMemoryInfo.memory, 0, sizeof(renderObject) * renderMemoryInfo.count);
	  SDL_memset(memory.temporaryStorage, 0, memory.temporaryStorageSize);

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

	  if(samplesToAppend > audioStreamMaxSize) {
	    //TODO: logging: audio length necessary to be continuous is longer than buffer size
	    samplesToAppend = audioStreamMaxSize;
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

	  
	  gameCode.gameUpdateAndRender(input, &memory, &renderMemoryInfo);
	  

	  assert(samplesToAppend <= audioStreamMaxSize);
	  SDL_memset(stream, 0, audioStreamMaxSize);
	  for(int i=0; i < samplesToAppend; ++i) {
	    double time = ((double) samplePos) / ((double) sampleRate); // in seconds
	    *(stream + i) = (int16) (amplitude * sin(2.0f * PI * 260.0f * time));
	    
	    samplePos++;			     
	  }
	  SDL_QueueAudio(device, (void *)stream, samplesToAppend * sizeof(int16));

	  
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
	      glUniform1f(glGetUniformLocation(shader[obj.renderContext], "alpha"), obj.alpha);

	    
	      glDrawElements(GL_TRIANGLES, indicesCount[obj.renderContext], GL_UNSIGNED_INT, 0);

	    }
	    
	  }

	  if(currentTime > lastLabelTimestamp + 1000) {
	    frameDurationLabel = (int)deltaTime;
	    lastLabelTimestamp = currentTime;
	  }

	  char fpsCounter[256];
	  sprintf_s(fpsCounter, "Frame duration: %i ms", frameDurationLabel);
	  renderText(fpsCounter, 10.f, screenHeight - 20.f, 0.3f, vec3{1.f, 1.f, 1.f});
	  renderText(logText, 10.f, screenHeight - 60.f, 0.4f, vec3{1.f, 1.f, 1.f});

	  SDL_GL_SwapWindow(window);
	}

	//SDL_GL_DeleteContext(glcontext);
	SDL_CloseAudioDevice(device);

	//free(renderMemoryRaw);

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

