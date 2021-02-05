#ifndef TEXTURE_H
#define TEXTUE_H

#include <glad/gl.h>

#include <iostream>
#include <string>

using namespace std;

class Texture {
public:

  unsigned int id;

  Texture(string image_path) {
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    // Set wrapping and filtering options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);  
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  

    // Load data and generate texture
    int width, height, nrChannels;
    unsigned char *data = stbi_load(image_path.c_str(), &width, &height, &nrChannels, 0);
    //stbi_set_flip_vertically_on_load(true);
    if(data) {
      cout << "Loaded image data. Width: "<<width<<" Height: "<<height<<" Num of channels: "<<nrChannels << endl;
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
      glGenerateMipmap(GL_TEXTURE_2D);

    } else {
      cout << "Failed to load texture" << endl;

    }

    stbi_image_free(data);

  }

  void bind() {
    glBindTexture(GL_TEXTURE_2D, id);
  }

  
};



#endif
