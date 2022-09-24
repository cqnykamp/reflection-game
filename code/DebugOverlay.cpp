#if !defined(DEBUG_OVERLAY_CPP)
#define DEBUG_OVERLAY_CPP

#include <cstdarg>

#include "reflect.h"

struct DebugOverlay {

  float height = 0.95f;

  void reset() {
    height = 0.95f;
  }

  void display(renderObject** renderMemory, char* message, ...) {

    char text[256];

    va_list valist;
    va_start(valist, message);

    vsprintf_s(text, message, valist);
    
    va_end(valist);

    **renderMemory = renderObject{TEXT, identity4.translated(0.5f, height, 0.f),
      identity4,identity3f,0,0.f, text};

    (*renderMemory) ++;
    
    height -= 0.03f;

  }

};

#endif