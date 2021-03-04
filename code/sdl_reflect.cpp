//TODO: figure out why startup is laggy aka >17ms per frame
//TODO: better logging system?

//#define SDL_MAIN_HANDLED
#include "SDL/SDL.h"
#include <glad/gl.h>

#include "reflect.h"

//TODO: don't include this
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>

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

int activeRenderTypes = 0;
//TODO: make sure MAX_RENDER_TYPES is big enough for RenderContext enum
#define MAX_RENDER_TYPES 10
unsigned int vao[MAX_RENDER_TYPES];
unsigned int vbo[MAX_RENDER_TYPES];
unsigned int ebo[MAX_RENDER_TYPES];
unsigned int shader[MAX_RENDER_TYPES];
unsigned int indicesCount[MAX_RENDER_TYPES];


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

#define MAX_LOG_TEXT 4096
char logText[MAX_LOG_TEXT];
int usedChars = 0;

int frameDurationLabel;
uint32 lastLabelTimestamp = 0;

struct GameCode {
  HMODULE gameHandle;
  FILETIME dllLastWriteTime;
  GameUpdateAndRender *gameUpdateAndRender;
  bool isValid;
};

struct LoopedCodeData {
  void *gameMemoryBlock;
  int gameMemoryBlockSize;
  HANDLE recordingFileHandle;
  HANDLE playbackFileHandle;
  bool isRecording;
  bool isPlayingBack;
  
};



void log(const char *message) {
  for(int i=0; i<strlen(message); i++ ) {
    
    logText[usedChars] = *(message + i);
    usedChars++;
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


CREATE_NEW_RENDER_OBJECT(createNewRenderObject) {

  //TODO: this doesn't work bc active types aren't neccessarily sequential in the list; just clear all of them instead?
  activeRenderTypes++;

  unsigned int thisVao;
  unsigned int thisVbo;
  unsigned int thisEbo;

  glGenVertexArrays(1, &(thisVao));
  glGenBuffers(1, &(thisVbo));
  glGenBuffers(1, &(thisEbo));

  glBindVertexArray(thisVao);
  glBindBuffer(GL_ARRAY_BUFFER, thisVbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, thisEbo);

  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * verticesLength, vertices, GL_STATIC_DRAW);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indicesLength, indices, GL_STATIC_DRAW);
  
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float),(void*) 0);
  glEnableVertexAttribArray(0);

  vao[context] = thisVao;
  vbo[context] = thisVbo;
  ebo[context] = thisEbo;
  shader[context] = loadShaderFromFile(filePath);
  indicesCount[context] = indicesLength;

}





inline FILETIME getLastWriteTime(char *filename) {

  FILETIME lastWriteTime = {};
  WIN32_FIND_DATA findData;
  HANDLE findHandle = FindFirstFileA(filename, &findData);
  if(findHandle != INVALID_HANDLE_VALUE) {
    lastWriteTime = findData.ftLastWriteTime;
    FindClose(findHandle);
  }
  
  return lastWriteTime;

}

GameCode loadGameCode(char *sourceDLLName, char *tempDLLName) {




	
  GameCode gameCode = {};

  gameCode.dllLastWriteTime = getLastWriteTime(sourceDLLName);
  
  CopyFile(sourceDLLName, tempDLLName, FALSE);
  
  gameCode.gameHandle = LoadLibrary(tempDLLName);
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

  glDeleteVertexArrays(MAX_RENDER_TYPES, vao);
  glDeleteBuffers(MAX_RENDER_TYPES, vbo);
  glDeleteBuffers(MAX_RENDER_TYPES, ebo);
  activeRenderTypes = 0;
  
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

DEBUG_LOG(debugLog) {
  //SDL_memset(logText, 0, sizeof(char) * MAX_LOG_TEXT);
  for(int i=0; i<MAX_LOG_TEXT-usedChars; i++) {
    if(text[i] == '\0') {
      break;
    } else {
      logText[usedChars] = text[i];
      usedChars++;
    }
  }
}



void catStrings(size_t sourceACount, char *sourceA,
			 size_t sourceBCount, char *sourceB,
			 size_t destCount, char *dest) {
  for(int index = 0; index < sourceACount; index++) {
    *dest++ = *sourceA++;
  }
  for(int index = 0; index < sourceBCount; index++) {
    *dest++ = *sourceB++;
  }

  *dest++ = 0;
}

void win32BeginRecordingInput(LoopedCodeData *loopedCodeData) {
  char *filename = "foo.ri";
  loopedCodeData->recordingFileHandle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

  DWORD bytesWritten;
  WriteFile(loopedCodeData->recordingFileHandle, loopedCodeData->gameMemoryBlock,
	    loopedCodeData->gameMemoryBlockSize, &bytesWritten, 0);


}

void win32EndRecordingInput(LoopedCodeData *loopedCodeData) {
  CloseHandle(loopedCodeData->recordingFileHandle);
}

void win32BeginInputPlayback(LoopedCodeData *loopedCodeData) {
  char *filename = "foo.ri";
  loopedCodeData->playbackFileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

  DWORD bytesRead;
  ReadFile(loopedCodeData->playbackFileHandle, loopedCodeData->gameMemoryBlock,
	   loopedCodeData->gameMemoryBlockSize, &bytesRead, 0);

  
}

void win32EndInputPlayback(LoopedCodeData *loopedCodeData) {
  CloseHandle(loopedCodeData->playbackFileHandle);
}



void win32RecordInput(LoopedCodeData *loopedCodeData, gameInput *newInput) {

  DWORD bytesWritten;
  WriteFile(loopedCodeData->recordingFileHandle, newInput, sizeof(gameInput), &bytesWritten, 0);

}

void win32PlaybackInput(LoopedCodeData *loopedCodeData, gameInput *newInput) {
  DWORD bytesRead;
  if(ReadFile(loopedCodeData->playbackFileHandle, newInput, sizeof(gameInput), &bytesRead, 0) && bytesRead != 0) {

  } else {
    win32EndInputPlayback(loopedCodeData);
    win32BeginInputPlayback(loopedCodeData);
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

  //NOTE: never use MAX_PATH in user-facing code
  char exeFileName[MAX_PATH];
  DWORD sizeOfFilename = GetModuleFileNameA(0, exeFileName, sizeof(exeFileName));

  char *onePastLastSlash = exeFileName;
  for(char *scan = exeFileName; *scan; scan++) {
    if(*scan == '\\') {
      onePastLastSlash = scan + 1;
    }
  }


  char sourceGameCodeDLLFilename[] = "reflect.dll";
  char sourceGameCodeDLLFullPath[MAX_PATH];
  catStrings(onePastLastSlash - exeFileName, exeFileName,
	     sizeof(sourceGameCodeDLLFilename) - 1, sourceGameCodeDLLFilename,
	     sizeof(sourceGameCodeDLLFullPath), sourceGameCodeDLLFullPath);

  
  char tempGameCodeDLLFilename[] = "temp_reflect.dll";
  char tempGameCodeDLLFullPath[MAX_PATH];
  catStrings(onePastLastSlash - exeFileName, exeFileName,
	     sizeof(tempGameCodeDLLFilename) - 1, tempGameCodeDLLFilename,
	     sizeof(tempGameCodeDLLFullPath), tempGameCodeDLLFullPath);
  
  
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


       
	//TODO: look into different allocator than malloc
	int maxRenderObjects = 1000;
	void *renderMemoryRaw = malloc(sizeof(renderObject) * maxRenderObjects);
	renderObject *renderObjects = (renderObject *) renderMemoryRaw;
	SDL_memset(renderObjects, 0, sizeof(renderObject) * maxRenderObjects);	
	RenderMemoryInfo renderMemoryInfo = {
	  maxRenderObjects,
	  renderObjects};


	LoopedCodeData loopedCodeData = {};

	//Game Memory
	gameMemory memory = {};
	memory.isInitialized = false;
	memory.isDllFirstFrame = true;
	memory.hexMode = false;
	memory.permanentStorageSize = PERMANENT_STORAGE_SIZE;
	memory.temporaryStorageSize = TEMPORARY_STORAGE_SIZE;

	//Pass platform function pointers
	memory.loadLevelFromFile = loadLevelFromFile;
	memory.updateRenderContextVertices = updateRenderContextVertices;
	memory.debugLog = debugLog;
	memory.createNewRenderObject = createNewRenderObject;


	LPVOID baseAddress = 0;

	loopedCodeData.gameMemoryBlock  = VirtualAlloc(baseAddress,
			 (size_t)(PERMANENT_STORAGE_SIZE+TEMPORARY_STORAGE_SIZE),
			 MEM_RESERVE|MEM_COMMIT,
			 PAGE_READWRITE);
	loopedCodeData.gameMemoryBlockSize = PERMANENT_STORAGE_SIZE + TEMPORARY_STORAGE_SIZE;

	memory.permanentStorage = loopedCodeData.gameMemoryBlock;
	memory.temporaryStorage = ((uint8 *)memory.permanentStorage + memory.permanentStorageSize);

	/*
	memory.permanentStorage = malloc(PERMANENT_STORAGE_SIZE);
	memory.temporaryStorage = malloc(TEMPORARY_STORAGE_SIZE);

	SDL_memset(memory.permanentStorage, 0, memory.permanentStorageSize);
	SDL_memset(memory.temporaryStorage, 0, memory.temporaryStorageSize);
	*/

	//Memory snapshot for live looped code editing


	
	//Audio
	int audioStreamMaxSize = (int)(sizeof(int16) * sampleRate * 0.5f); //enough for 1/2 second
	int16 *stream = (int16 *)malloc(audioStreamMaxSize);


	//TODO: use open-source font
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
	int counter = 0;

	memory.isDllFirstFrame = true;
	GameCode gameCode = loadGameCode(sourceGameCodeDLLFullPath,
					 tempGameCodeDLLFullPath);

	loopedCodeData.isRecording = false;
	
	FILETIME prevFrameDllWriteTime = getLastWriteTime(sourceGameCodeDLLFullPath);
	
	while(running) {


	  FILETIME newDllWriteTime = getLastWriteTime(sourceGameCodeDLLFullPath);
	  if(CompareFileTime(&newDllWriteTime, &prevFrameDllWriteTime) == 0) {

	    if(CompareFileTime(&newDllWriteTime, &gameCode.dllLastWriteTime) != 0) {
	      //gameCode.dllLastWriteTime = newDllWriteTime;

	      memory.isDllFirstFrame = true;
	      unloadGameCode(&gameCode);
	      gameCode = loadGameCode(sourceGameCodeDLLFullPath,
				      tempGameCodeDLLFullPath);
	      //counter = 30;

	    }
	  }

	  prevFrameDllWriteTime = newDllWriteTime;

	  
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

		SDL_Scancode scancode = event.key.keysym.scancode;
		
		if(event.type == SDL_KEYDOWN && scancode==SDL_SCANCODE_ESCAPE) {
		  //running = false;
		}
		if(event.type == SDL_KEYDOWN && scancode==SDL_SCANCODE_L) {
		  if(!loopedCodeData.isRecording && !loopedCodeData.isPlayingBack) {
		    //start recording
		    loopedCodeData.isRecording = true;
		    win32BeginRecordingInput(&loopedCodeData);
		    
		  } else if(loopedCodeData.isRecording) {
		    //stop recording, start playback		    
		    loopedCodeData.isRecording = false;
		    loopedCodeData.isPlayingBack = true;

		    win32EndRecordingInput(&loopedCodeData);

		    win32BeginInputPlayback(&loopedCodeData);


		  } else if(loopedCodeData.isPlayingBack) {
		    loopedCodeData.isPlayingBack = false;
		    win32EndInputPlayback(&loopedCodeData);
		    
		  }
		}
		if(scancode==SDL_SCANCODE_R) {
		  mainController.rKey.transitionCount += 1;
		  mainController.rKey.endedDown = (event.type==SDL_KEYUP);
		}
		if(scancode==SDL_SCANCODE_LEFT) {
		  mainController.leftArrow.transitionCount += 1;
		  mainController.leftArrow.endedDown = (event.type==SDL_KEYUP);
		}
		if(scancode==SDL_SCANCODE_RIGHT) {
		  mainController.rightArrow.transitionCount += 1;
		  mainController.rightArrow.endedDown = (event.type==SDL_KEYUP);
		}

		if(event.type==SDL_KEYDOWN &&
		   (scancode==SDL_SCANCODE_Q || scancode==SDL_SCANCODE_H)) {
		  #ifdef REFLECT_INTERNAL

		  memory.isDllFirstFrame = true;
		  if(scancode==SDL_SCANCODE_H) {
		    memory.hexMode = !memory.hexMode;
		  }
		  unloadGameCode(&gameCode);
		  gameCode = loadGameCode(sourceGameCodeDLLFullPath, tempGameCodeDLLFullPath);
		  
		  #endif
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

	  SDL_memset(logText, 0, sizeof(char) * MAX_LOG_TEXT);
	  usedChars = 0;

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

	  
	  if(loopedCodeData.isRecording) {
	    win32RecordInput(&loopedCodeData, &input);
	  }
	  if(loopedCodeData.isPlayingBack) {
	    win32PlaybackInput(&loopedCodeData, &input);
	  }
	  
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

	  char frameCounter[256];
	  sprintf_s(frameCounter, "Counter: %i ", counter);
	  renderText(frameCounter, 10.f, screenHeight - 40.f, 0.3f, vec3{1.f, 1.f, 1.f});

	  char recordingText[256];
	  sprintf_s(recordingText, "Is recording: %i ", loopedCodeData.isRecording);
	  renderText(recordingText, 10.f, screenHeight - 60.f, 0.3f, vec3{1.f, 1.f, 1.f});

	  char playbackText[256];
	  sprintf_s(playbackText, "Is playing back: %i ", loopedCodeData.isPlayingBack);
	  renderText(playbackText, 10.f, screenHeight - 80.f, 0.3f, vec3{1.f, 1.f, 1.f});

	  //log("print this ");
	  //log("and print this\n");

	  renderText(logText, 10.f, screenHeight - 100.f, 0.4f, vec3{1.f, 1.f, 1.f});

	  //std::cout << ("print this " + "and this\n");

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
