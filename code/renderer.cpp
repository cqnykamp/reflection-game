#if !defined(RENDERER_CPP)

#include "SDL/SDL.h"

#include <fstream>
#include <sstream>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "reflect.h"
#include "gameutil.cpp"
#include "texture.h"

struct Renderer {

  int screenWidth;
  int screenHeight;

  mat4 projectionInverse;

  Renderer(int width, int height) {

    screenWidth = width;
    screenHeight = height;

    projectionInverse = mat4 {
      (float)screenWidth / 2.f, 0.f,                     0.f, (float)screenWidth/2.f,
      0.f,                      (float)screenHeight/2.f, 0.f, (float)screenHeight/2.f,
      0.f,                      0.f,                     1.f, 0.f,
      0.f,                      0.f,                     0.f, 1.f
    };

  }



  int activeRenderTypes = 0;
  //TODO: make sure MAX_RENDER_TYPES is big enough for RenderContext enum
  #define MAX_RENDER_TYPES 10
  unsigned int vao[MAX_RENDER_TYPES];
  unsigned int vbo[MAX_RENDER_TYPES];
  unsigned int ebo[MAX_RENDER_TYPES];
  unsigned int shader[MAX_RENDER_TYPES];
  unsigned int indicesCount[MAX_RENDER_TYPES];
  Texture textures[MAX_RENDER_TYPES];

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
  int innerFrameDurationLabel;
  uint32 lastLabelTimestamp = 0;


  //unused
  void log(const char *message) {
    for(unsigned int i=0; i < strlen(message); i++ ) {
      
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


  UPDATE_RENDER_CONTEXT_VERTICES(updateRenderContextVertices) {
    //TODO: don't use GL_STATIC_DRAW
    glBindVertexArray(vao[context]);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[context]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * verticesLength, vertices);
  }

  CREATE_NEW_RENDER_OBJECT(createNewRenderObject) {

    //TODO: this doesn't work bc active types aren't neccessarily sequential in the list; just clear all of them instead?
    activeRenderTypes++;


    unsigned int thisVao;
    unsigned int thisVbo;
    unsigned int thisEbo;
  // 
    glGenVertexArrays(1, &(thisVao));
    glGenBuffers(1, &(thisVbo));
    glGenBuffers(1, &(thisEbo));

    glBindVertexArray(thisVao);
    glBindBuffer(GL_ARRAY_BUFFER, thisVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, thisEbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * verticesLength, vertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indicesLength, indices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),(void*) 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float),(void*) (3*sizeof(float)) );
      glEnableVertexAttribArray(1);


    textures[context] = Texture();

    char fullTexturePath[100] = "textures/";
    strcat_s(fullTexturePath, textureName);
    textures[context].initWithImage(fullTexturePath, outputWithTransparency);

    vao[context] = thisVao;
    vbo[context] = thisVbo;
    ebo[context] = thisEbo;
    shader[context] = loadShaderFromFile(filePath);
    indicesCount[context] = indicesLength;

  }


  void rendererUnload() {
    glDeleteVertexArrays(MAX_RENDER_TYPES, vao);
    glDeleteBuffers(MAX_RENDER_TYPES, vbo);
    glDeleteBuffers(MAX_RENDER_TYPES, ebo);
    activeRenderTypes = 0;

  }



  void renderText(std::string text, float x, float y, float scale, vec3 color) {

    glUseProgram(fontShader);
    glUniform3f(glGetUniformLocation(fontShader, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(fontVao);

    for(unsigned int i=0; i<text.size(); i++) {
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


  int initFonts() {
      //Fonts
    FT_Library ft;
    if(FT_Init_FreeType(&ft)) {
      std::cout << "ERROR::FREETYPE: Could not init FreeType Library\n";
      return -1;
    }
    
    FT_Face face;
    if(FT_New_Face(ft, "arial.ttf", 0, &face)) { 	//TODO: use open-source font
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


    return 0; //aka loaded successfully
  }



  void renderFrame(RenderMemoryInfo* renderMemoryInfoPointer, gameMemory memory) {

    RenderMemoryInfo renderMemoryInfo = *renderMemoryInfoPointer;

    SDL_memset(logText, 0, sizeof(char) * MAX_LOG_TEXT);
    usedChars = 0;

    glViewport(0, 0, screenWidth, screenHeight);
    glClearColor(1.f, 0.f, 1.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);


    for(int i=0; i<renderMemoryInfo.count; i++) {
      
      renderObject obj = *(renderMemoryInfo.memory + i);

      if(obj.renderContext) { //aka if game has put something here


        if(obj.renderContext == TEXT) {

    mat4 pos = projectionInverse * obj.view * obj.model;

    renderText(obj.text,
    pos.xw, pos.yw, 0.3f, vec3{1.f, 1.f, 1.f});

        } else {
        

    textures[obj.renderContext].bind();

    glBindVertexArray(vao[obj.renderContext]);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[obj.renderContext]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo[obj.renderContext]);
    glUseProgram(shader[obj.renderContext]);

    mat4 modelMatData = transpose(obj.model);
    glUniformMatrix4fv(glGetUniformLocation(shader[obj.renderContext], "model"),
          1, GL_FALSE, &modelMatData.xx);	      
    mat4 viewMatData = transpose(obj.view);
    glUniformMatrix4fv(glGetUniformLocation(shader[obj.renderContext], "view"),
          1, GL_FALSE, &viewMatData.xx);
    mat3 basisMatData = transpose(obj.basis);
    glUniformMatrix3fv(glGetUniformLocation(shader[obj.renderContext], "basis"),
          1, GL_FALSE, &basisMatData.xx);

    glUniform1i(glGetUniformLocation(shader[obj.renderContext],
            "highlight_key"), obj.highlight_key);
    glUniform1f(glGetUniformLocation(shader[obj.renderContext],
            "alpha"), obj.alpha);
      
    glDrawElements(GL_TRIANGLES, indicesCount[obj.renderContext], GL_UNSIGNED_INT, 0);
        }

      }
      
    }
  
    //Deactivated renderer-level log text for now
    /*
    if(memory.debugTextActive) {
      uint32 innerDeltaTime = (uint32) (1000.f*(float)innerFrameTime / (float)timerFrequency);	  

      if(currentTime > lastLabelTimestamp + 500) {
        frameDurationLabel = (int)deltaTime;
        innerFrameDurationLabel = (int)innerDeltaTime;
        lastLabelTimestamp = currentTime;
      }

      char fpsCounter[256];
      sprintf_s(fpsCounter, "Frame dur (ms): %i", frameDurationLabel);
      renderText(fpsCounter, 10.f, screenHeight - 20.f, 0.25f, vec3{1.f, 1.f, 1.f});

      char label[256];
      sprintf_s(label, "Inner dur (ms): %i",innerFrameDurationLabel);
      renderText(label, 10.f, screenHeight - 35.f, 0.25f, vec3{1.f, 1.f, 1.f});
    
      char frameCounter[256];
      sprintf_s(frameCounter, "Counter: %i ", counter);
      renderText(frameCounter, 10.f, screenHeight - 60.f, 0.3f, vec3{1.f, 1.f, 1.f});

      char recordingText[256];
      sprintf_s(recordingText, "Is recording: %i ", loopedCodeData.isRecording);
      renderText(recordingText, 10.f, screenHeight - 80.f, 0.3f, vec3{1.f, 1.f, 1.f});

      char playbackText[256];
      sprintf_s(playbackText, "Is playing back: %i ", loopedCodeData.isPlayingBack);
      renderText(playbackText, 10.f, screenHeight - 100.f, 0.3f, vec3{1.f, 1.f, 1.f});

      //log("print this ");
      //log("and print this\n");

      renderText(logText, 10.f, screenHeight - 120.f, 0.4f, vec3{1.f, 1.f, 1.f});

    }
    */

  }

};

#endif