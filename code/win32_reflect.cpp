
/**
TODO: THIS IS NOT A FINAL PLATFORM LAYER!!!
- saved game locations
- getting a handle to our own executable file
- asset loading path
- threading (launching a thread)
- raw input (support for multiple keyboards)
- sleep/timeBeginPeriod
- clipCursor() (for multimonitor support)
- fullscreen support
- control cursor visibility
- QueryCancelAutoplay
- WM_ACTIVATEAPP for when we are not the active app
- Blit speed improvements (BitBlt)
- hardware acceleration (OpenGl or Direct3d or both?)
- GetKeyboardLayout (international WASD support)

(just a partial list)

*/


#include <stdint.h>
#include <math.h>

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.141592653589f

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

#include "handmade.cpp"



#include <windows.h>
#include <stdio.h>
#include <xinput.h>
#include <dsound.h>





struct win32OffscreenBuffer {
  BITMAPINFO info;
  void *memory;
  int width;
  int height;
  int pitch;
};


// XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) {
  return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// YInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) {
  return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_



#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);








global_variable bool32  wKeyPressed;
global_variable bool32 aKeyPressed;

//TODO: this is a global for now
global_variable bool32 globalRunning;
global_variable win32OffscreenBuffer globalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER globalSecondaryBuffer;

struct win32WindowDimension {
  int width;
  int height;
};



void *platformLoadFile(char *fileName) {
  return 0;
}

internal void win32LoadXInput() {

  HMODULE xInputLibrary = LoadLibrary("xInput1_4.dll");
  if(!xInputLibrary) {
    xInputLibrary = LoadLibrary("xinput9_1_0.dll");
  }
  if(!xInputLibrary) {
    xInputLibrary = LoadLibrary("xInput1_3.dll");
  }
  if(xInputLibrary) {
    XInputGetState = (x_input_get_state *)GetProcAddress(xInputLibrary, "XInputGetState");
    XInputSetState = (x_input_set_state *)GetProcAddress(xInputLibrary, "XInputSetState");
  }

}

internal void win32InitDSound(HWND window, int32 samplesPerSecond, int32 bufferSize) {
  //Load library
  HMODULE dSoundLibrary = LoadLibrary("dsound.dll"); //TODO: dsound3d.dll?

  if(dSoundLibrary) {

    direct_sound_create *DirectSoundCreate = (direct_sound_create *) GetProcAddress(dSoundLibrary, "DirectSoundCreate");

    LPDIRECTSOUND DirectSound;
    if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))) {

      WAVEFORMATEX waveFormat = {};
      waveFormat.wFormatTag = WAVE_FORMAT_PCM;
      waveFormat.nChannels = 2;
      waveFormat.nSamplesPerSec = samplesPerSecond;
      waveFormat.wBitsPerSample = 16;
      waveFormat.nBlockAlign = waveFormat.nChannels*waveFormat.wBitsPerSample / 8;
      waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
      waveFormat.cbSize = 0;


      if(SUCCEEDED(DirectSound->SetCooperativeLevel(window, DSSCL_PRIORITY))) {

	// "Create" primary buffer (we don't write our audio data into this one)	
	DSBUFFERDESC bufferDescription = {};
	bufferDescription.dwSize = sizeof(bufferDescription);
	bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
	
	LPDIRECTSOUNDBUFFER primaryBuffer;
	if(SUCCEEDED(DirectSound->CreateSoundBuffer( &bufferDescription, &primaryBuffer, 0))) {
	  HRESULT error = primaryBuffer->SetFormat(&waveFormat);
	  if(SUCCEEDED(error)) {
	    //We have finally set the format
	    OutputDebugStringA("Primary buffer set succesfully\n");

	  } else {
	    //TODO: diagnostics
	  }

	}
	//TODO diagnostic
      }


      
      // "Create" secondary buffer (we will write to this)
      DSBUFFERDESC bufferDescription = {};
      bufferDescription.dwSize = sizeof(bufferDescription);
      bufferDescription.dwFlags = 0;
      bufferDescription.dwBufferBytes = bufferSize;
      bufferDescription.lpwfxFormat = &waveFormat;	
      HRESULT error = DirectSound->CreateSoundBuffer( &bufferDescription, &globalSecondaryBuffer, 0);
      if(SUCCEEDED(error)) {
	OutputDebugStringA("Secondary buffer set successfully!\n");
      }
      

      //Start playing

    } else {
      // logging system
    }

    
  }

}


internal win32WindowDimension getWindowDimension(HWND window) {
  win32WindowDimension result;
  
  RECT clientRect;
  GetClientRect(window, &clientRect);      
  result.width = clientRect.right - clientRect.left;
  result.height = clientRect.bottom - clientRect.top;
  
  return result;
}



internal void win32ResizeDIBSection(win32OffscreenBuffer *buffer, int width, int height) {

  if(buffer->memory) {
    VirtualFree(buffer->memory, 0, MEM_RELEASE);
  }

  int bytesPerPixel = 4;
  
  buffer->width = width;
  buffer->height = height;
  
  BITMAPINFOHEADER biHeader;
  biHeader.biSize = sizeof(biHeader);
  biHeader.biWidth = buffer->width;
  biHeader.biHeight = -buffer->height;
  biHeader.biPlanes = 1;
  biHeader.biBitCount = 32; //we actually only need 3*8=24, but we're asking for 32 to keep alignment
  biHeader.biCompression = BI_RGB; //aka no compression
  
  buffer->info.bmiHeader = biHeader;

  int bitmapMemorySize = bytesPerPixel * buffer->width * buffer->height;
  buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

  buffer->pitch = buffer->width * bytesPerPixel;

  //TODO: Probably clear this to black
}

internal void win32DisplayBufferInWindow(win32OffscreenBuffer *buffer, HDC deviceContext, int windowWidth, int windowHeight) {
 
  //This function copies our array buffer to the location where Windows uses to draw
  //TODO: aspect ratio correction
  StretchDIBits(deviceContext,
		0, 0, windowWidth, windowHeight,
		0, 0, buffer->width, buffer->height,
		buffer->memory,
		&buffer->info,
		DIB_RGB_COLORS,
		SRCCOPY);
}






internal LRESULT CALLBACK win32MainWindowCallback(HWND window,
						  UINT   message,
						  WPARAM wParam,
						  LPARAM lParam) {

  LRESULT Result = 0;
    
  switch(message) {
  case WM_SIZE:
    {
      
    } break;
  case WM_CLOSE:
    {
      //TODO: Handle this with a message to user?
      globalRunning = false;

    } break;
  case WM_ACTIVATEAPP:
    {
      OutputDebugStringA("WN_ACTIVATEAPP\n");
    } break;
  case WM_DESTROY:
    {
      //TODO: Handle this as an error - recreate window?
      globalRunning = false;

    } break;

    
  case WM_SYSKEYDOWN:
  case WM_SYSKEYUP:
  case WM_KEYDOWN:
  case WM_KEYUP:
    {

      uint32 vkCode = wParam;
      bool32 wasDown = ( (lParam & (1<<30)) != 0);
      bool32 isDown = ( (lParam & (1<<31)) == 0);

      if(wasDown != isDown) {

	
	if(vkCode == 'W') {
	  
	  if(isDown) {
	    wKeyPressed = true;
	  } else if(wasDown) {
	    wKeyPressed = false;
	  }

	} else if(vkCode == 'A') {

	} else if(vkCode == 'S') {

	  if(isDown) {
	    aKeyPressed = true;
	  } else if(wasDown) {
	    aKeyPressed = false;
	  }

	} else if(vkCode == 'D') {

	} else if(vkCode == 'Q') {

	} else if(vkCode == 'E') {

	}  else if(vkCode == 'A') {

	} else if(vkCode == VK_UP) {

	}else if(vkCode == VK_LEFT) {

	} else if(vkCode == VK_RIGHT) {

	} else if(vkCode == VK_DOWN) {

	} else if(vkCode == VK_ESCAPE) {

	  OutputDebugStringA("ESCAPE: ");
	  if(isDown) {


	    OutputDebugStringA("pressed ");
	  }
	  if(wasDown) {
	    OutputDebugStringA("released ");
	  }
	  
	  OutputDebugStringA("\n");

	} else if(vkCode == VK_SPACE) {
	}
      }

      bool32 altKeyWasDown = lParam & (1<<29);
      if(vkCode == VK_F4 && altKeyWasDown) {
	globalRunning = false;
      }

    } break;
  case WM_PAINT:
    {
      //NOTE: Windows gives us this message whenever our window gets partially obscured by something pelse
      PAINTSTRUCT Paint;
      HDC deviceContext = BeginPaint(window, &Paint);
      int x = Paint.rcPaint.left;
      int y = Paint.rcPaint.top;
      int width = Paint.rcPaint.right - Paint.rcPaint.left;
      int height = Paint.rcPaint.bottom - Paint.rcPaint.top;

      win32WindowDimension dimension = getWindowDimension(window);
      win32DisplayBufferInWindow(&globalBackBuffer, deviceContext, dimension.width, dimension.height);


      
    } break;
  default:
    {
      //OutputDebugStringA("default\n");
      Result = DefWindowProc(window, message, wParam, lParam);
    } break;
      
  }
  return Result;
}



struct win32_sound_output {
  int samplesPerSecond;
  int toneVolume;
  int toneHz;
  uint32 runningSampleIndex;
  int wavePeriod;
  int bytesPerSample;
  int secondaryBufferSize;
  real32 tSine;
  int latencySampleCount;
  
};



void win32FillSoundBuffer(win32_sound_output *soundOutput, DWORD byteToLock, DWORD bytesToWrite) {
  VOID *region1;
  DWORD region1Size;
  VOID *region2;
  DWORD region2Size;
  if(SUCCEEDED(globalSecondaryBuffer->Lock(
					   byteToLock,
					   bytesToWrite,
					   &region1,&region1Size,
					   &region2, &region2Size,
					   0))) {

    //NOTE: region1Size + region2Size should equal bytesToWrite
	    
    //TODO: assert that region1size/region2Size is valid

    globalSecondaryBuffer-> Unlock(region1, region1Size, region2, region2Size);

  }
}



int CALLBACK  WinMain(
		      HINSTANCE Instance,
		      HINSTANCE PrevInstance,
		      LPSTR     CommandLine,
		      int       ShowCode )
{

  
  LARGE_INTEGER perfCountFrequencyResult;
  QueryPerformanceFrequency(&perfCountFrequencyResult);
  int64 perfCountFrequency = perfCountFrequencyResult.QuadPart;

  WNDCLASSA windowClass = {};

  win32LoadXInput();

  windowClass.style = CS_HREDRAW|CS_VREDRAW;
  windowClass.lpfnWndProc = win32MainWindowCallback;
  windowClass.hInstance = Instance;
  //windowClass.hIcon;
  windowClass.lpszClassName = "HandmadeHeroWindowClass";

  
  //Allocate back buffer
  win32ResizeDIBSection(&globalBackBuffer, 1280, 720);


  if(RegisterClass(&windowClass)) {
    
    HWND window =
      CreateWindowEx(
		     0,
		     windowClass.lpszClassName,
		     "Handmade Hero",
		     WS_OVERLAPPEDWINDOW|WS_VISIBLE,
		     CW_USEDEFAULT,
		     CW_USEDEFAULT,
		     CW_USEDEFAULT,
		     CW_USEDEFAULT,
		     0,
		     0,
		     Instance,
		     0
		     );

    if(window) {
      globalRunning = true;

      int xOffset = 0;
      int yOffset = 0;


      win32_sound_output soundOutput = {};
      soundOutput.samplesPerSecond = 48000;
      soundOutput.toneVolume = 6000;
      soundOutput.toneHz = 256;
      soundOutput.runningSampleIndex = 0;
      soundOutput.wavePeriod = soundOutput.samplesPerSecond / soundOutput.toneHz;
      //      int halfWavePeriod = wavePeriod / 2;
      soundOutput.bytesPerSample = sizeof(int16) * 2;      
      soundOutput.secondaryBufferSize = soundOutput.samplesPerSecond*soundOutput.bytesPerSample; //we have 1sec buffer
      soundOutput.latencySampleCount = soundOutput.samplesPerSecond / 15;
      
      win32InitDSound(window, soundOutput.samplesPerSecond, soundOutput.samplesPerSecond* soundOutput.bytesPerSample);
      win32FillSoundBuffer(&soundOutput, 0, soundOutput.latencySampleCount * soundOutput.bytesPerSample);
      globalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
      
      LARGE_INTEGER lastCounter;
      QueryPerformanceCounter(&lastCounter);

      uint64 lastCycleCount =  __rdtsc();
      
      
      while(globalRunning) {
	
	MSG message;

	


	while( PeekMessageA(&message, 0,0,0, PM_REMOVE)) {
	  if(message.message == WM_QUIT) {
	    globalRunning = false;
	  }
	  
	  TranslateMessage(&message);
	  DispatchMessageA(&message);
	}

	//TODO: should we poll this more frequently than every frame?	
	for(DWORD controllerIndex = 0; controllerIndex< XUSER_MAX_COUNT; controllerIndex++) {
	  
	  XINPUT_STATE controllerState ;
	  if(XInputGetState( controllerIndex, &controllerState ) == ERROR_SUCCESS) {
	    // the controller is plugged in

	    
	    //TODO: see if dwPacketNumber increments too rapidly
	    XINPUT_GAMEPAD *pad = &controllerState.Gamepad;
	    
	    bool32 up = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
	    bool32 down = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
	    bool32 left = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
	    bool32 right = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
	    bool32 start = (pad->wButtons & XINPUT_GAMEPAD_START);
	    bool32 back = (pad->wButtons & XINPUT_GAMEPAD_BACK);
	    bool32 leftShoulder = (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
	    bool32 rightShoulder = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
	    bool32 aButton = (pad->wButtons & XINPUT_GAMEPAD_A);
	    bool32 bButton = (pad->wButtons & XINPUT_GAMEPAD_B);
	    bool32 xButton = (pad->wButtons & XINPUT_GAMEPAD_X);
	    bool32 yButton = (pad->wButtons & XINPUT_GAMEPAD_Y);

	    int16 stickX = pad->sThumbLX;
	    int16 stickY = pad->sThumbLY;

	    xOffset += stickX / 4096;
	    yOffset += stickY / 4096;
	  
	    XINPUT_VIBRATION vibration;
	    vibration.wLeftMotorSpeed = 60000;
	    vibration.wRightMotorSpeed = 60000;
	    XInputSetState(0, &vibration);
	    
	    
	  } else {
	    // the controller is not available
	  }
	  
	}

	if(wKeyPressed) {
	  soundOutput.toneHz += 1;
	  soundOutput.wavePeriod = soundOutput.samplesPerSecond / soundOutput.toneHz;
	} else if(aKeyPressed) {
	  soundOutput.toneHz -= 1;
	  soundOutput.wavePeriod = soundOutput.samplesPerSecond / soundOutput.toneHz;	  
	}
	

	int16 samples[(48000 / 30) * 2];
	gameSoundOutputBuffer soundBuffer = {};
	soundBuffer.samplesPerSecond = soundOutput.samplesPerSecond;
	soundBuffer.sampleCount = soundBuffer.samplesPerSecond / 30;
	soundBuffer.samples = samples;
	
	
	
	gameOffscreenBuffer buffer = {};
	buffer.memory = globalBackBuffer.memory;
	buffer.width = globalBackBuffer.width;
	buffer.height = globalBackBuffer.height;
	buffer.pitch = globalBackBuffer.pitch;	
	
	gameUpdateAndRender(&buffer, xOffset, yOffset, &soundBuffer);



	
	DWORD playCursor;
	DWORD writeCursor; //we don't actually care about this value

	if(SUCCEEDED(globalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor))) {

	  DWORD byteToLock = (soundOutput.runningSampleIndex*soundOutput.bytesPerSample) % soundOutput.secondaryBufferSize;
	  DWORD targetCursor = (playCursor + (soundOutput.latencySampleCount * soundOutput.bytesPerSample)) % soundOutput.secondaryBufferSize;
	  DWORD bytesToWrite;

	  //NOTE: Play cursor is an **estimate** of where the sound card is currenty playing on the speakers

	  
	  //TODO: change this to use a lower latency offset from te playcursor when we start having sound
	  if(byteToLock > targetCursor) {
	    bytesToWrite = (soundOutput.secondaryBufferSize - byteToLock);
	    bytesToWrite += targetCursor;
	  } else {
	    bytesToWrite = targetCursor - byteToLock;
	  }


	  win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite);





	  
	  


	  HDC deviceContext = GetDC(window);

	  win32WindowDimension dimension = getWindowDimension(window);
	  win32DisplayBufferInWindow(&globalBackBuffer, deviceContext, dimension.width, dimension.height);

	  ReleaseDC(window, deviceContext);

	  
	
	  //	xOffset++;
	  //yOffset++;

	  uint64 endCycleCount =  __rdtsc();

	  LARGE_INTEGER endCounter;
	  QueryPerformanceCounter(&endCounter);


	  int cyclesElapsed = endCycleCount - lastCycleCount;
	  int64 counterElapsed = endCounter.QuadPart - lastCounter.QuadPart;
	  real64 msPerFrame = (real64) (1000.0f*(real64)counterElapsed) / (real64)perfCountFrequency;
	  real64 fps = (real64)perfCountFrequency / (real64)counterElapsed;
	  real64 mcpf = ((real64)cyclesElapsed / (1000.0f * 1000.0f)); //mega cycles per frame

#if 0
	  char buffer[256];
	  sprintf(buffer, "%.02fms, %.02ffps, %.02fmc/f\n", msPerFrame, fps, mcpf);
	  OutputDebugStringA(buffer);
#endif


	  
	  lastCycleCount = endCycleCount;
	  lastCounter = endCounter;
		  

	}
      }
    
    }


  }
  return 0;
}
