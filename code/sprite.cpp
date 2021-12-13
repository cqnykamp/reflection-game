#if !defined(SPRITE_CPP)
#define SPRITE_CPP


#include "gameutil.cpp"

struct Sprite {

    vec2 defaultScale;
    vec2 defaultOffset;

    vec2 scale;
    vec2 offset;

    vec2 position;
    float rotation;

    char *textureName;

    //unused right now. activated by renderer
    bool rendererHasInitMe;
    unsigned int rendererTextureId;

    Sprite(vec2 spriteScale, vec2 spriteOffset, char *textureFilename) {
        defaultScale = spriteScale;
        defaultOffset = spriteOffset;
        textureName = textureFilename;
        resetTransform();

        rendererHasInitMe = false;
        rendererTextureId = 0;

    }

    Sprite() {
        Sprite(vec2(1.0f), vec2(0.f), "awesomeface.png");
    }

    void resetTransform() {
        position = vec2(0.f);
        scale = defaultScale;
        offset = defaultOffset;
        rotation = 0;
    }


    mat4 getModelMatrix() {

        mat4 model = identity4;
        model = model.translated(vec3(offset, 0.0f));
        mat4 scaleMatrix = {
            scale.x, 0.0f, 0.0f, 0.0f,
            0.f, scale.y, 0.0f, 0.0f,
            0.f, 0.f, 1.0f, 0.0f,
            0.f, 0.f, 0.0f, 1.0f
        };
        mat4 rotationMatrix = {
            cos(rotation), -sin(rotation), 0.0f, 0.0f,
            sin(rotation), cos(rotation), 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        model = rotationMatrix * scaleMatrix * model;
        model = model.translated(vec3(position, 0.0f));

        return model;
    }


};

#endif