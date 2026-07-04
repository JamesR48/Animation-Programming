#ifndef CPPANIMPROGRAMMING_SHADERSTORAGEBUFFER_H
#define CPPANIMPROGRAMMING_SHADERSTORAGEBUFFER_H

#include <vector>
#include <glm/glm.hpp>
#include <glad/gl.h>

class ShaderStorageBuffer
{
public:
    void Init(size_t BufferSize);
    void UploadSsboData(const std::vector<glm::mat4>& BufferData, const int BindingPoint);
    void UploadSsboData(const std::vector<glm::mat2x4>& BufferData, const int BindingPoint);
    void Cleanup();

private:
    size_t mBufferSize;
    GLuint mShaderStorageBuffer = 0;
};

#endif //CPPANIMPROGRAMMING_SHADERSTORAGEBUFFER_H
