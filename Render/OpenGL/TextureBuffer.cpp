#include "TextureBuffer.h"

void TextureBuffer::Init(size_t BufferSize)
{
    mBufferSize = BufferSize;

    /* creating buffer */
    glGenBuffers(1, &mTextureBuffer);
    glBindBuffer(GL_TEXTURE_BUFFER, mTextureBuffer);
    glBufferData(GL_TEXTURE_BUFFER, BufferSize, NULL, GL_STATIC_DRAW);

    /* creating texture */
    glGenTextures(1, &mTexture);
    glBindTexture(GL_TEXTURE_BUFFER, mTexture);

    /*  attaching the TBO to the mTexture texture. Any data uploaded into the buffer defined by
        mTextureBuffer appears as a texture with four 32-bit float components, usable in the vertex shader. */
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, mTextureBuffer);

    glBindTexture(GL_TEXTURE_BUFFER, 0);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
}

void TextureBuffer::UploadTboData(const std::vector<glm::mat4>& BufferData, int BindingPoint)
{
    if (BufferData.size() == 0)
    {
        return;
    }
    mTexIndex = BindingPoint;
    size_t BufferSize = BufferData.size() * sizeof(glm::mat4);
    glBindBuffer(GL_TEXTURE_BUFFER, mTextureBuffer);
    glBufferSubData(GL_TEXTURE_BUFFER, 0, BufferSize, BufferData.data());
    glBindBufferRange(GL_TEXTURE_BUFFER, BindingPoint, mTextureBuffer, 0, BufferSize);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
}

void TextureBuffer::Bind()
{
    /* activating texture unit */
    glActiveTexture(GL_TEXTURE0 + mTexIndex);

    glBindTexture(GL_TEXTURE_BUFFER, mTexture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void TextureBuffer::Cleanup()
{
    glDeleteTextures(1, &mTexture);
    glDeleteBuffers(1, &mTextureBuffer);
}
