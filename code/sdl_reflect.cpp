//TODO: figure out why startup is laggy aka >17ms per frame 
//TODO: better logging system?

//#define SDL_MAIN_HANDLED
#include "SDL/SDL.h"
#include <glad/gl.h>

#include "reflect.h"
#include "renderer.cpp"

//TODO: don't include this
#include <iostream>
#include <fstream>

#include <windows.h>

#define global static

#define PERMANENT_STORAGE_SIZE (6 * 1024)
#define TEMPORARY_STORAGE_SIZE (1 * 1024 * 1024)


const int amplitude = 0;//10000;
const int sampleRate = 48000;

int screenWidth = 1919; //1920; //1280;
int screenHeight = 1079; //1080; //720;

Renderer renderer = Renderer(screenWidth, screenHeight);





int frameDurationLabel;
int innerFrameDurationLabel;
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

void catStrings(size_t sourceACount, char *sourceA,
			 size_t sourceBCount, char *sourceB,
			 size_t destCount, char *dest) {
  for(unsigned int index = 0; index < sourceACount; index++) {
    *dest++ = *sourceA++;
  }
  for(unsigned int index = 0; index < sourceBCount; index++) {
    *dest++ = *sourceB++;
  }

  *dest++ = 0;
}






inline FILETIME getLastWriteTime(char *filename) {

  FILETIME lastWriteTime = {};
  WIN32_FIND_DATAA findData;
  HANDLE findHandle = FindFirstFileA(filename, &findData);
  if(findHandle != INVALID_HANDLE_VALUE) {
    lastWriteTime = (findData).ftLastWriteTime;
    FindClose(findHandle);
  }
  
  return lastWriteTime;

}

GameCode loadGameCode(char *sourceDLLName, char *tempDLLName) {

	
  GameCode gameCode = {};

  gameCode.dllLastWriteTime = getLastWriteTime(sourceDLLName);
  
  CopyFileA(sourceDLLName, tempDLLName, FALSE);
  
  gameCode.gameHandle = LoadLibraryA(tempDLLName);
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

  renderer.rendererUnload();
  
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






void win32BeginRecordingInput(LoopedCodeData *loopedCodeData) {
  char *filename = "looped_data.ri";
  loopedCodeData->recordingFileHandle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

  DWORD bytesWritten;
  WriteFile(loopedCodeData->recordingFileHandle, loopedCodeData->gameMemoryBlock,
	    loopedCodeData->gameMemoryBlockSize, &bytesWritten, 0);
}

void win32EndRecordingInput(LoopedCodeData *loopedCodeData) {
  CloseHandle(loopedCodeData->recordingFileHandle);
}

void win32BeginInputPlayback(LoopedCodeData *loopedCodeData) {
  char *filename = "looped_data.ri";
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


// int wmain(int argc, char *argv[]) {




// UPDATE_RENDER_CONTEXT_VERTICES(updateRenderContextVertices) {
//   renderer.updateRenderContextVertices(context, vertices, verticesLength);
// }
CREATE_NEW_RENDER_OBJECT(createNewRenderObject) {
  renderer.createNewRenderObject(vertices, verticesLength, indices, indicesLength,filePath, context, textureName, outputWithTransparency);
}







int CALLBACK  WinMain(
		      HINSTANCE Instance,
		      HINSTANCE PrevInstance,
		      LPSTR     CommandLine,
		      int       ShowCode )
{


	//Tell windows about our dpi awareness, aka that we will deal with different dpis manually
	//TODO: do this in the manifest file instead
	SetProcessDPIAware();

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
					  SDL_WINDOW_OPENGL); // | SDL_WINDOW_FULLSCREEN);
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


  //TODO: SHOULD NOT NEED THE FOLLOWING FUNCTION POINTERS, DO IT WITH MEMORY INSTEAD

	// memory.updateRenderContextVertices = updateRenderContextVertices;
	// memory.debugLog = debugLog;
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

	
  if(renderer.init() != 0) {
    return -1; //quit
  }
	
	uint64 timerFrequency = SDL_GetPerformanceFrequency();
	uint64 prevTime = SDL_GetPerformanceCounter();

	uint64 innerFrameTime = 0;
	
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

	  uint64 innerFrameStart = SDL_GetPerformanceCounter();


    // ========= DLL loading and unloading =============

	  FILETIME newDllWriteTime = getLastWriteTime(sourceGameCodeDLLFullPath);
	  if(CompareFileTime(&newDllWriteTime, &prevFrameDllWriteTime) == 0) {

	    if(CompareFileTime(&newDllWriteTime, &gameCode.dllLastWriteTime) != 0) {
	      memory.isDllFirstFrame = true;
	      unloadGameCode(&gameCode);
	      gameCode = loadGameCode(sourceGameCodeDLLFullPath,
				      tempGameCodeDLLFullPath);
	    }
	  }

	  prevFrameDllWriteTime = newDllWriteTime;


    //Reset Temporary Storage memory
	  SDL_memset(memory.temporaryStorage, 0, memory.temporaryStorageSize);

    //Reset Render Output memory
	  SDL_memset(renderMemoryInfo.memory, 0, sizeof(renderObject) * renderMemoryInfo.count);



    // ============ Read Keyboard/Mouse Input ==========
	
	//NOTE: Keyboard input is all messed up. Ended down should be when (event.type == [KEYDOWN]).
	// Also, SDL sends multiple events when key held down, so we need to ignore those.


	  controllerInput mainController = {};

	  SDL_Event event;
	  while(SDL_PollEvent(&event)) {
	    switch(event.type) {
	    case SDL_QUIT: {
	      running = false;
	    } break;

	    case SDL_MOUSEMOTION: {
	    } break;

	    case SDL_MOUSEBUTTONDOWN:
	    case SDL_MOUSEBUTTONUP: {


        switch(event.button.button) {
        case SDL_BUTTON_LEFT: {
          mainController.mouseLeft.transitionCount += 1;
          mainController.mouseLeft.endedDown = (event.type == SDL_MOUSEBUTTONDOWN);
        } break;

        case SDL_BUTTON_RIGHT: { } break;
        }

      } break;	      
          
      case SDL_KEYDOWN:
      case SDL_KEYUP: {

        SDL_Scancode scancode = event.key.keysym.scancode;
        
        if(event.type == SDL_KEYDOWN && scancode==SDL_SCANCODE_ESCAPE) {
          running = false;
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

        if(event.type==SDL_KEYDOWN && scancode==SDL_SCANCODE_D) {
          memory.debugTextActive = !memory.debugTextActive;
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





    // ================ Audio ===============

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
	  


    // ====== Capture input data ==========

	  gameInput input;
	  input.screenWidth = screenWidth;
	  input.screenHeight = screenHeight;
	  input.controllers[0] = mainController;



    FrameInfo frameInfo;
	  
	  uint64 timerCount = SDL_GetPerformanceCounter();
	  uint64 deltaCount = timerCount - prevTime;
	  uint32 deltaTime = (uint32) (1000.f * (float)deltaCount / (float)timerFrequency);
	  uint32 currentTime = (uint32) (1000.f * (float) timerCount / (float)timerFrequency); 
    
    frameInfo.currentTime = currentTime;
	  frameInfo.deltaTime = deltaTime;

    uint32 innerDeltaTime = (uint32) (1000.f*(float)innerFrameTime / (float)timerFrequency);
    frameInfo.innerDeltaTime = innerDeltaTime;

	  prevTime = timerCount;


	  //std::cout << "Duraton: "<<deltaTime<< " ms\n";

	  
	  if(loopedCodeData.isRecording) {
	    win32RecordInput(&loopedCodeData, &input);
	  }
	  if(loopedCodeData.isPlayingBack) {
	    win32PlaybackInput(&loopedCodeData, &input);
	  }


	  // ====== GAME UPDATE STATE ==========
	  gameCode.gameUpdateAndRender(input, frameInfo, &memory, &renderMemoryInfo);
	  


	  assert(samplesToAppend <= audioStreamMaxSize);
	  SDL_memset(stream, 0, audioStreamMaxSize);
	  for(int i=0; i < samplesToAppend; ++i) {
	    double time = ((double) samplePos) / ((double) sampleRate); // in seconds
	    *(stream + i) = (int16) (amplitude * sin(2.0f * PI * 260.0f * time));
	    
	    samplePos++;			     
	  }
	  SDL_QueueAudio(device, (void *)stream, samplesToAppend * sizeof(int16));


    //Render frame
    renderer.renderFrame(&renderMemoryInfo, memory);

    /*
    if(currentTime > lastLabelTimestamp + 500) {
      frameDurationLabel = (int)deltaTime;
      innerFrameDurationLabel = (int)innerDeltaTime;
      lastLabelTimestamp = currentTime;
    }
    */
	    
	  uint64 innerFrameEnd = SDL_GetPerformanceCounter();
	  innerFrameTime = innerFrameEnd - innerFrameStart;

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
