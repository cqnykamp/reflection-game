#if !defined(TEXTURE_H)
#define TEXTURE_H

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad/gl.h>

#include <iostream>
#include <string>

// using namespace std;

class Texture {
public:

  unsigned int id;

  void initWithImage(std::string image_path, bool outputWithTransparency) {
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
      std::cout << "Loaded image data. Width: "<<width<<" Height: "<<height<<" Num of channels: "<<nrChannels << std::endl;

      // std::printf("Filepath: %s\n", image_path.c_str());
      // std::printf("Size of filepath: %i\n", (int)sizeof(image_path));
      // std::printf("Length of filepath: %i\n", (int)image_path.length());

      std::string imageExt = image_path.substr(image_path.length()-3,3);
      // std::cout << "Image file extension as std::string: " << imageExt << "\n";

      GLenum inputFormat = (imageExt == "png") ? GL_RGBA : GL_RGB;
      GLint outputFormat = (outputWithTransparency) ? GL_RGBA : GL_RGB;

      glTexImage2D(GL_TEXTURE_2D, 0, outputFormat, width, height, 0, inputFormat, GL_UNSIGNED_BYTE, data);
          
      glGenerateMipmap(GL_TEXTURE_2D);

    } else {
      std::cout << "Failed to load texture" << std::endl;

    }

    stbi_image_free(data);



  }

  void bind() {
    glBindTexture(GL_TEXTURE_2D, id);
  }

  

};



#endif
