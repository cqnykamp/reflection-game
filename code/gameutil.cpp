#if !defined(GAMEUTIL_CPP)


#include <cmath>

#include<vector>

//
//
// VECTORS
//
//


struct TEMPvec2 {
  float x = 0.0f;
  float y = 0.0f;

  TEMPvec2() {}
  TEMPvec2(float _f)           { x = _f; y = _f; }
  TEMPvec2(float _x, float _y) { x = _x; y = _y; }
  void operator *= ( float f) {x = x*f; y = y*f; }
};

struct TEMPivec2 {
  int x = 0;
  int y = 0;

  operator TEMPvec2() const { return TEMPvec2{(float)x, (float)y}; }
  void operator *= ( int f) { x = x*f; y = y*f; }  
};


bool operator==(TEMPvec2 v1, TEMPvec2 v2) {    return (v1.x == v2.x && v1.y == v2.y); }
TEMPvec2 operator+(TEMPvec2 v1, TEMPvec2 v2) { return TEMPvec2{v1.x + v2.x, v1.y+v2.y}; }
TEMPvec2 operator-(TEMPvec2 v1, TEMPvec2 v2) { return TEMPvec2{v1.x - v2.x, v1.y - v2.y}; }
TEMPvec2 operator*(float f, TEMPvec2 v) {      return TEMPvec2{f * v.x, f * v.y}; }
TEMPvec2 operator*(TEMPvec2 v, float f) {      return f * v; }

// Special ivec2 variations to keep result in ivec2
TEMPivec2 operator+(TEMPivec2 v1, TEMPivec2 v2) { return TEMPivec2{v1.x+v2.x, v1.y+v2.y}; }
TEMPivec2 operator-(TEMPivec2 v1, TEMPivec2 v2) { return TEMPivec2{v1.x-v2.x, v1.y-v2.y}; }
TEMPivec2 operator*(int i, TEMPivec2 v) {         return TEMPivec2{i * v.x, i * v.y}; }
TEMPivec2 operator*(TEMPivec2 v, int i) {         return i * v; }


float magnitude(TEMPvec2 v) {  return sqrt(v.x * v.x + v.y * v.y); }
TEMPvec2 normalize(TEMPvec2 v) {  return TEMPvec2{v.x / magnitude(v), v.y / magnitude(v)}; }

float dot(TEMPvec2 v1, TEMPvec2 v2) {   return v1.x * v2.x + v1.y * v2.y; }
float dot(TEMPivec2 v1, TEMPivec2 v2) { return (float) (v1.x * v2.x + v1.y * v2.y); } 



struct TEMPvec3 {
  float x=0;
  float y=0;
  float z=0;
};

struct TEMPivec3 {
  int x=0;
  int y=0;
  int z=0;
};



struct TEMPvec4 {
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

struct TEMPmat3 {
  float xx=1.0f; float xy=0.0f; float xz=0.0f;
  float yx=0.0f; float yy=1.0f; float yz=0.0f;
  float zx=0.0f; float zy=0.0f; float zz=1.0f;
};

struct TEMPimat3 {
  int xx=1; int xy=0; int xz=0;
  int yx=0; int yy=1; int yz=0;
  int zx=0; int zy=0; int zz=1;

  operator TEMPmat3() const {
    return TEMPmat3{
      (float)xx, (float)xy, (float) xz,
      (float)yx, (float)yy, (float) yz,
      (float)zx, (float)zy, (float) zz
    };
  }
};

TEMPmat3 operator*(float f, TEMPmat3 m) {
  return TEMPmat3 {
    f*m.xx, f*m.xy, f*m.xz,
    f*m.yx, f*m.yy, f*m.yz,
    f*m.zx, f*m.zy, f*m.zz
  };
}
TEMPmat3 operator*(TEMPmat3 m, float f) {
  return f * m;
}

TEMPivec3 operator*(TEMPimat3 m, TEMPivec3 v) {
  return TEMPivec3{
    m.xx*v.x + m.xy*v.y + m.xz*v.z,
    m.yx*v.x + m.yy*v.y + m.yz*v.z,
    m.zx*v.x + m.zy*v.y + m.zz*v.z
  };
}

TEMPmat3 operator*(TEMPmat3 a, TEMPmat3 b) {
  return TEMPmat3 {
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
TEMPimat3 operator*(TEMPimat3 a, TEMPimat3 b) {
  return TEMPimat3 {
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


TEMPmat3 operator+(TEMPmat3 a, TEMPmat3 b) {
  return TEMPmat3 {
    a.xx+b.xx, a.xy+b.xy, a.xz+b.xz,
    a.yx+b.yx, a.yy+b.yy, a.yz+b.yz,
    a.zx+b.zx, a.zy+b.zy, a.zz+b.zz,
  };
}


TEMPmat3 transpose(TEMPmat3 m) {
  return TEMPmat3 {
    m.xx, m.yx, m.zx,
    m.xy, m.yy, m.zy,
    m.xz, m.yz, m.zz
  };
}
TEMPimat3 transpose(TEMPimat3 m) {
  return TEMPimat3 {
    m.xx, m.yx, m.zx,
    m.xy, m.yy, m.zy,
    m.xz, m.yz, m.zz
  };
}











struct TEMPmat4 {
  float xx; float xy; float xz; float xw;
  float yx; float yy; float yz; float yw;
  float zx; float zy; float zz; float zw;
  float wx; float wy; float wz; float ww;
};


const TEMPmat4 identity4 = TEMPmat4 {
  1.0f, 0.0f, 0.0f, 0.0f,
  0.0f, 1.0f, 0.0f, 0.0f,
  0.0f, 0.0f, 1.0f, 0.0f,
  0.0f, 0.0f, 0.0f, 1.0f
};


/**
TEMPmat3 TEMPmat3WithDiag(float f) {
  TEMPmat3 m = {};
  m.xx = f;
  m.yy = f;
  m.zz = f;
  return m;
}

TEMPimat3 TEMPimat3WithDiag(int n) {
  TEMPimat3 m = {};
  m.xx = n;
  m.yy = n;
  m.zz = n;
  return m;
}
**/



TEMPmat4 transpose(TEMPmat4 m) {
  return TEMPmat4 {
    m.xx, m.yx, m.zx, m.wx,
    m.xy, m.yy, m.zy, m.wy,
    m.xz, m.yz, m.zz, m.wz,
    m.xw, m.yw, m.zw, m.ww
  };
}



TEMPvec4 operator*(TEMPmat4 m, TEMPvec4 v) {
  return TEMPvec4 {
    m.xx*v.x + m.xy*v.y + m.xz*v.z + m.xw*v.w,
    m.yx*v.x + m.yy*v.y + m.yz*v.z + m.yw*v.w,
    m.zx*v.x + m.zy*v.y + m.zz*v.z + m.zw*v.w,
    m.wx*v.x + m.wy*v.y + m.wz*v.z + m.ww*v.w
  };
}




TEMPmat4 operator*(TEMPmat4 a, TEMPmat4 b) {
  return TEMPmat4 {
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






/**
void matrixVectorMultiplyInt(int m[], int v[], int result[], int vecSize) {
  //NOTE: assumes matrix is row-major
  for(int i=0; i < vecSize; i++) {
    int sum = 0;
    for(int j=0; j  < vecSize; j++) {
      sum += m[i * vecSize + j] * v[j]; 
    }
    result[i] = sum;
  } 
}
void matrixVectorMultiplyFloat(float m[], float v[], float result[], int vecSize) {
  //NOTE: assumes matrix is row-major
  for(int i=0; i < vecSize; i++) {
    float sum = 0;
    for(int j=0; j  < vecSize; j++) {
      sum += m[i * vecSize + j] * v[j]; 
    }
    result[i] = sum;
  }    
}

TEMPivec3 operator*(TEMPimat3 m, TEMPivec3 v) {
  int m_array[] = {m.xx, m.xy, m.xz, m.yx, m.yy, m.yz, m.zx, m.zy, m.zz};
  int v_array[] = {v.x, v.y, v.z};
  int r_array[3];
  matrixVectorMultiplyInt(m_array, v_array, r_array, 3);
  return TEMPivec3{r_array[0], r_array[1], r_array[2]};
}
  **/


#define GAMEUTIL_CPP
#endif
