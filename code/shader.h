#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {
public:
  // the program id
  unsigned int ID;

  Shader() {
    return;
  }

  Shader(const char* filePath) {
    load(filePath);
  }


  //constructor reads and builds the shader
  Shader(const char* vertexPath, const char* fragmentPath) {
    load(vertexPath, fragmentPath);
  }

  void loadCode(const char* vShaderCode, const char* fShaderCode) {
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
    ID = glCreateProgram();
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
    
  }

  void load(const char *filePath) {

    
    // 1. retrieve the vertex/fragment source code from filePath
    
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream shaderFile;

    //ensure ifstream objects can throw exceptions
    shaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);

    //TODO: remove exception here so that can disable exceptions in build file
    try {
      //open files
      shaderFile.open(filePath);
      std::stringstream shaderStream;
      //read file's buffer contents into streams
      shaderStream << shaderFile.rdbuf();
      //close file handlers
      shaderFile.close();
      // convert stream into string
      vertexCode = "#version 330 core\n#define VERTEX_PROGRAM\n" + shaderStream.str();
      fragmentCode = "#version 330 core\n#define FRAGMENT_PROGRAM\n" + shaderStream.str();

    } catch (std::ifstream::failure& e) {

      std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << std::endl;
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();


    loadCode(vShaderCode, fShaderCode);

  }




  
  void load(const char*vertexPath, const char* fragmentPath) {

    // 1. retrieve the vertex/fragment source code from filePath
    
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    //ensure ifstream objects can throw exceptions
    vShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit); 

    try {
      //open files
      vShaderFile.open(vertexPath);
      fShaderFile.open(fragmentPath);
      std::stringstream vShaderStream, fShaderStream;
      //read file's buffer contents into streams
      vShaderStream << vShaderFile.rdbuf();
      fShaderStream << fShaderFile.rdbuf();
      //close file handlers
      vShaderFile.close();
      fShaderFile.close();
      // convert stream into string
      vertexCode = vShaderStream.str();
      fragmentCode = fShaderStream.str();

    } catch (std::ifstream::failure& e) {

      std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << std::endl;
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();


    loadCode(vShaderCode, fShaderCode);
    
  }

  
  //use/activate the shader
  void use() {
    glUseProgram(ID);
  }

   
  //utility uniform functions
  void setBool(const std::string &name, bool value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
  }
  void setInt(const std::string &name, int value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
  }
  void setIntArray(const std::string &name, int size, int value[]) {
    glUniform1iv(glGetUniformLocation(ID, name.c_str()), size, value);
  }

  void setFloat(const std::string &name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
  }
  

  void setMat4(const std::string &name, TEMPmat4 value) const {
    TEMPmat4 matData = transpose(value);
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &matData.xx);
  };
  

  void setMat3(const std::string &name, TEMPmat3 value) const {
    //glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
    TEMPmat3 matData =  transpose(value);
    glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &matData.xx);
  };
  
};

#endif
