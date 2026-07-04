#ifndef CPPANIMPROGRAMMING_UNIFORMBUFFER_H
#define CPPANIMPROGRAMMING_UNIFORMBUFFER_H

#include <vector>
#include <glm/glm.hpp>
#include <glad/gl.h>

class UniformBuffer
{
public:
    void Init(size_t BufferSize);
    //void UploadUboData(glm::mat4 ViewMatrix, glm::mat4 ProjectionMatrix);
    void UploadUboData(const std::vector<glm::mat4>& BufferData, int BindingPoint);
    void Cleanup();

private:
    size_t mBufferSize;
    GLuint mUboBuffer = 0;
};

#endif //CPPANIMPROGRAMMING_UNIFORMBUFFER_H
