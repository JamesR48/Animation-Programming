#ifndef CPPANIMPROGRAMMING_TEXTUREBUFFER_H
#define CPPANIMPROGRAMMING_TEXTUREBUFFER_H

#include <vector>
#include <glm/glm.hpp>
#include <glad/gl.h>

/*
* TBOs are not backed by an image like a real texture. Instead, a separated buffer is bound to the texture
  unit, and every texel can be read by its position. The value is returned without any filtering or
  interpolation that a real texture image may have, making it perfect for the transport of data larger than
  the minimal 64KB of a UBO to the GPU.
 */
class TextureBuffer
{
public:
    void Init(size_t BufferSize);
    void UploadTboData(const std::vector<glm::mat4>& BufferData, int BindingPoint);
    void Bind();
    void Cleanup();

private:
    size_t mBufferSize = 0;
    GLuint mTexIndex = 0;
    GLuint mTexture = 0;
    GLuint mTextureBuffer = 0;
};

#endif //CPPANIMPROGRAMMING_TEXTUREBUFFER_H
