#ifndef CPPANIMPROGRAMMING_TEXTURE_H
#define CPPANIMPROGRAMMING_TEXTURE_H

#include <string>
#include <glad/gl.h>

class Texture
{
public:
    bool LoadTexture(std::string TextureFilename, bool bFlipImage = true);
    void Bind();
    void Unbind();
    void Cleanup();

private:
    GLuint mTexture = 0;
    int mTexWidth = 0;
    int mTexHeight = 0;
    int mNumberOfChannels = 0;
    std::string mTextureName;
};

#endif //CPPANIMPROGRAMMING_TEXTURE_H
