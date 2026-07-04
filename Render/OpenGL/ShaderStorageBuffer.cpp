#include "ShaderStorageBuffer.h"

// Shader Storage Buffer Objects: can be seen as a mix between a uniform buffer and a texture
/* SSBOs can be much larger; the minimum guaranteed size is 128 MB (UBO: 16 KB)
• SSBOs are writable (UBOs are read-only)
• SSBOs can store arrays of arbitrary length (UBOs have a fixed size) */
void ShaderStorageBuffer::Init(size_t BufferSize)
{
    mBufferSize = BufferSize;

    glGenBuffers(1, &mShaderStorageBuffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mShaderStorageBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, mBufferSize, NULL, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ShaderStorageBuffer::UploadSsboData(const std::vector<glm::mat4> &BufferData, const int BindingPoint)
{
    if (BufferData.size() == 0)
    {
        return;
    }

    size_t BufferSize = BufferData.size() * sizeof(glm::mat4);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mShaderStorageBuffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, BufferSize, BufferData.data());
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, BindingPoint, mShaderStorageBuffer, 0, BufferSize);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ShaderStorageBuffer::UploadSsboData(const std::vector<glm::mat2x4> &BufferData, const int BindingPoint)
{
    if (BufferData.size() == 0)
    {
        return;
    }

    size_t BufferSize = BufferData.size() * sizeof(glm::mat2x4);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mShaderStorageBuffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, BufferSize, BufferData.data());
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, BindingPoint, mShaderStorageBuffer, 0, BufferSize);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ShaderStorageBuffer::Cleanup()
{
    glDeleteBuffers(1, &mShaderStorageBuffer);
}
