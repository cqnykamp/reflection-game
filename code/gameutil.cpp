#if !defined(GAMEUTIL_CPP)


#include <cmath>



float max(float a, float b) {
  if(a > b) {
    return a;
  } else {
    return b;
  }
}

float min(float a, float b) {
  if(a < b) {
    return a;
  } else {
    return b;
  }
}



//
//
// VECTORS
//
//


struct vec2 {
  float x = 0.0f;
  float y = 0.0f;

  vec2() {}
  vec2(float _f)           { x = _f; y = _f; }
  vec2(float _x, float _y) { x = _x; y = _y; }
  void operator *= ( float f) {x = x*f; y = y*f; }
};

struct ivec2 {
  int x = 0;
  int y = 0;

  operator vec2() const { return vec2{(float)x, (float)y}; }
  void operator *= ( int f) { x = x*f; y = y*f; }  
};


bool operator==(vec2 v1, vec2 v2) {    return (v1.x == v2.x && v1.y == v2.y); }
vec2 operator+(vec2 v1, vec2 v2) { return vec2{v1.x + v2.x, v1.y+v2.y}; }
vec2 operator-(vec2 v1, vec2 v2) { return vec2{v1.x - v2.x, v1.y - v2.y}; }
vec2 operator*(float f, vec2 v) {      return vec2{f * v.x, f * v.y}; }
vec2 operator*(vec2 v, float f) {      return f * v; }

// Special ivec2 variations to keep result in ivec2
ivec2 operator+(ivec2 v1, ivec2 v2) { return ivec2{v1.x+v2.x, v1.y+v2.y}; }
ivec2 operator-(ivec2 v1, ivec2 v2) { return ivec2{v1.x-v2.x, v1.y-v2.y}; }
ivec2 operator*(int i, ivec2 v) {         return ivec2{i * v.x, i * v.y}; }
ivec2 operator*(ivec2 v, int i) {         return i * v; }


float magnitude(vec2 v) {  return sqrt(v.x * v.x + v.y * v.y); }
vec2 normalize(vec2 v) {  return vec2{v.x / magnitude(v), v.y / magnitude(v)}; }

float dot(vec2 v1, vec2 v2) {   return v1.x * v2.x + v1.y * v2.y; }
float dot(ivec2 v1, ivec2 v2) { return (float) (v1.x * v2.x + v1.y * v2.y); } 



struct vec3 {
  float x=0;
  float y=0;
  float z=0;
};

struct ivec3 {
  int x=0;
  int y=0;
  int z=0;
};



struct vec4 {
  float x=0;
  float y=0;
  float z=0;
  float w=0;
};




//
//
// MATRICES
//
//

struct mat3 {
  float xx=1.0f; float xy=0.0f; float xz=0.0f;
  float yx=0.0f; float yy=1.0f; float yz=0.0f;
  float zx=0.0f; float zy=0.0f; float zz=1.0f;
};


struct imat3 {
  int xx=1; int xy=0; int xz=0;
  int yx=0; int yy=1; int yz=0;
  int zx=0; int zy=0; int zz=1;

  operator mat3() const {
    return mat3{
      (float)xx, (float)xy, (float) xz,
      (float)yx, (float)yy, (float) yz,
      (float)zx, (float)zy, (float) zz
    };
  }
};


const mat3 identity3f = mat3 {
  1.0f, 0.0f, 0.0f, 
  0.0f, 1.0f, 0.0f, 
  0.0f, 0.0f, 1.0f
};
const imat3 identity3i = imat3 {
  1, 0, 0,
  0, 1, 0,
  0, 0, 1
};




mat3 operator*(float f, mat3 m) {
  return mat3 {
    f*m.xx, f*m.xy, f*m.xz,
    f*m.yx, f*m.yy, f*m.yz,
    f*m.zx, f*m.zy, f*m.zz
  };
}
mat3 operator*(mat3 m, float f) {
  return f * m;
}

ivec3 operator*(imat3 m, ivec3 v) {
  return ivec3{
    m.xx*v.x + m.xy*v.y + m.xz*v.z,
    m.yx*v.x + m.yy*v.y + m.yz*v.z,
    m.zx*v.x + m.zy*v.y + m.zz*v.z
  };
}

vec3 operator*(mat3 m, vec3 v) {
  return vec3{
    m.xx*v.x + m.xy*v.y + m.xz*v.z,
    m.yx*v.x + m.yy*v.y + m.yz*v.z,
    m.zx*v.x + m.zy*v.y + m.zz*v.z
  };
}


mat3 operator*(mat3 a, mat3 b) {
  return mat3 {
    (a.xx*b.xx + a.xy*b.yx + a.xz*b.zx),
    (a.xx*b.xy + a.xy*b.yy + a.xz*b.zy),
    (a.xx*b.xz + a.xy*b.yz + a.xz*b.zz),
    
    (a.yx*b.xx + a.yy*b.yx + a.yz*b.zx),
    (a.yx*b.xy + a.yy*b.yy + a.yz*b.zy),
    (a.yx*b.xz + a.yy*b.yz + a.yz*b.zz),

    (a.zx*b.xx + a.zy*b.yx + a.zz*b.zx),
    (a.zx*b.xy + a.zy*b.yy + a.zz*b.zy),
    (a.zx*b.xz + a.zy*b.yz + a.zz*b.zz)
  };
}
imat3 operator*(imat3 a, imat3 b) {
  return imat3 {
    (a.xx*b.xx + a.xy*b.yx + a.xz*b.zx),
    (a.xx*b.xy + a.xy*b.yy + a.xz*b.zy),
    (a.xx*b.xz + a.xy*b.yz + a.xz*b.zz),
    
    (a.yx*b.xx + a.yy*b.yx + a.yz*b.zx),
    (a.yx*b.xy + a.yy*b.yy + a.yz*b.zy),
    (a.yx*b.xz + a.yy*b.yz + a.yz*b.zz),

    (a.zx*b.xx + a.zy*b.yx + a.zz*b.zx),
    (a.zx*b.xy + a.zy*b.yy + a.zz*b.zy),
    (a.zx*b.xz + a.zy*b.yz + a.zz*b.zz)
  };
}


mat3 operator+(mat3 a, mat3 b) {
  return mat3 {
    a.xx+b.xx, a.xy+b.xy, a.xz+b.xz,
    a.yx+b.yx, a.yy+b.yy, a.yz+b.yz,
    a.zx+b.zx, a.zy+b.zy, a.zz+b.zz,
  };
}


mat3 transpose(mat3 m) {
  return mat3 {
    m.xx, m.yx, m.zx,
    m.xy, m.yy, m.zy,
    m.xz, m.yz, m.zz
  };
}
imat3 transpose(imat3 m) {
  return imat3 {
    m.xx, m.yx, m.zx,
    m.xy, m.yy, m.zy,
    m.xz, m.yz, m.zz
  };
}












struct mat4 {
  float xx; float xy; float xz; float xw;
  float yx; float yy; float yz; float yw;
  float zx; float zy; float zz; float zw;
  float wx; float wy; float wz; float ww;

  mat4 translated(float x, float y, float z) {
    return mat4 {
      xx,xy,xz, xw + x,
      yx,yy,yz, yw + y,
      zx,zy,zz, zw + z,
      wx,wy,wz,ww
    };	      
  }

  mat4 translated(vec3 v) {
    return translated(v.x, v.y, v.z);
  }

  mat4 translatedX(float x) {
    return translated(x, 0.f, 0.f);
  }
  mat4 translatedY(float y) {
    return translated(0.f, y, 0.f);
  }
  
};



mat4 identity4 = mat4 {
  1.0f, 0.0f, 0.0f, 0.0f,
  0.0f, 1.0f, 0.0f, 0.0f,
  0.0f, 0.0f, 1.0f, 0.0f,
  0.0f, 0.0f, 0.0f, 1.0f
};



mat4 transpose(mat4 m) {
  return mat4 {
    m.xx, m.yx, m.zx, m.wx,
    m.xy, m.yy, m.zy, m.wy,
    m.xz, m.yz, m.zz, m.wz,
    m.xw, m.yw, m.zw, m.ww
  };
}



vec4 operator*(mat4 m, vec4 v) {
  return vec4 {
    m.xx*v.x + m.xy*v.y + m.xz*v.z + m.xw*v.w,
    m.yx*v.x + m.yy*v.y + m.yz*v.z + m.yw*v.w,
    m.zx*v.x + m.zy*v.y + m.zz*v.z + m.zw*v.w,
    m.wx*v.x + m.wy*v.y + m.wz*v.z + m.ww*v.w
  };
}




mat4 operator*(mat4 a, mat4 b) {
  return mat4 {
    (a.xx*b.xx + a.xy*b.yx + a.xz*b.zx + a.xw*b.wx),
    (a.xx*b.xy + a.xy*b.yy + a.xz*b.zy + a.xw*b.wy),
    (a.xx*b.xz + a.xy*b.yz + a.xz*b.zz + a.xw*b.wz),
    (a.xx*b.xw + a.xy*b.yw + a.xz*b.zw + a.xw*b.ww),
    
    (a.yx*b.xx + a.yy*b.yx + a.yz*b.zx + a.yw*b.wx),
    (a.yx*b.xy + a.yy*b.yy + a.yz*b.zy + a.yw*b.wy),
    (a.yx*b.xz + a.yy*b.yz + a.yz*b.zz + a.yw*b.wz),
    (a.yx*b.xw + a.yy*b.yw + a.yz*b.zw + a.yw*b.ww),

    (a.zx*b.xx + a.zy*b.yx + a.zz*b.zx + a.zw*b.wx),
    (a.zx*b.xy + a.zy*b.yy + a.zz*b.zy + a.zw*b.wy),
    (a.zx*b.xz + a.zy*b.yz + a.zz*b.zz + a.zw*b.wz),
    (a.zx*b.xw + a.zy*b.yw + a.zz*b.zw + a.zw*b.ww),

    (a.wx*b.xx + a.wy*b.yx + a.wz*b.zx + a.ww*b.wx),
    (a.wx*b.xy + a.wy*b.yy + a.wz*b.zy + a.ww*b.wy),
    (a.wx*b.xz + a.wy*b.yz + a.wz*b.zz + a.ww*b.wz),
    (a.wx*b.xw + a.wy*b.yw + a.wz*b.zw + a.ww*b.ww),
        
    
  };
}


#define GAMEUTIL_CPP
#endif
