#include "UniformBuffer.h"
#include <glm/gtc/type_ptr.hpp>
#include <glad/gl.h>

GLuint mUboBuffer = 0;

void UniformBuffer::Init(size_t BufferSize)
{
    mBufferSize = BufferSize;

    glGenBuffers(1, &mUboBuffer);

    glBindBuffer(GL_UNIFORM_BUFFER, mUboBuffer);
    glBufferData(GL_UNIFORM_BUFFER, BufferSize, NULL, GL_DYNAMIC_DRAW);

    // Unbinding
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UniformBuffer::UploadUboData(const std::vector<glm::mat4>& BufferData, int BindingPoint)
{
    if (BufferData.size() == 0)
    {
        return;
    }

    size_t BufferSize = BufferData.size() * sizeof(glm::mat4);
    glBindBuffer(GL_UNIFORM_BUFFER, mUboBuffer);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, BufferSize, BufferData.data());
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, mUboBuffer, 0, BufferSize);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UniformBuffer::Cleanup()
{
    glDeleteBuffers(1, &mUboBuffer);
}
