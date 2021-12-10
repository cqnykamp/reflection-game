#if !defined(SPRITE_CPP)
#define SPRITE_CPP

struct Sprite {

    float width;
    float height;
    char *textureName;

    float vertices[20];

    //activated by renderer
    bool rendererHasInitMe;
    int rendererId;
    unsigned int rendererTextureId;
    // unsigned int rendererVao;
    // unsigned int rendererVbo;
    // unsigned int rendererEbo;

    Sprite(float spriteWidth, float spriteHeight, char *textureFilename) {
        width = spriteWidth;
        height = spriteHeight;
        textureName = textureFilename;

        rendererHasInitMe = false;

        float verticesData[] = {
            //positions             //texture coords
            -0.5f * width, -0.5f * height, 0.0f,   0.0f, 0.0f,
            -0.5f * width,  0.5f * height, 0.0f,   0.0f, 1.0f,
             0.5f * width,  0.5f * height, 0.0f,   1.0f, 1.0f,
             0.5f * width, -0.5f * height, 0.0f,   1.0f, 0.0f
        };

        for(int i=0; i<20; i++) {
            vertices[i] = verticesData[i];
        }

        unsigned int indices[] = {
            0, 1, 2,
            2, 3, 0
        };
    }

    Sprite() {
        Sprite(1,1,"awesomeface.png");
    }


};

#endif