#ifndef OBJECTDATA_H
#define OBJECTDATA_H

#include <glad/gl.h>

class ObjectData {
public:
  unsigned int vao;
  unsigned int vbo;
  unsigned int ebo;

  int _indices_count;
  
  ObjectData(int data_count, int indices_count, float vertex_data[], unsigned int vertex_indices[], bool using_texture = false) {
    
    _indices_count = indices_count;
    
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * data_count, vertex_data, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices_count, vertex_indices, GL_STATIC_DRAW);


    if(using_texture) {
      
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float),(void*) 0);
      glEnableVertexAttribArray(0);
      
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float),(void*)(3*sizeof(float)));
      glEnableVertexAttribArray(1);
      
    } else {
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float),(void*) 0);
      glEnableVertexAttribArray(0);
    }
      


  }

  void use() {
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  }

  void draw() {
    glDrawElements(GL_TRIANGLES, _indices_count, GL_UNSIGNED_INT, 0);
  }

  void delete_data() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
  }

};


#endif

