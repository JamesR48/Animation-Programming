#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Texture.h"
#include <GLFW/glfw3.h>
#include "../../Tools/Logger.h"

bool Texture::LoadTexture(std::string TextureFilename, bool bFlipImage)
{
    mTextureName = TextureFilename;

    /* flip the image on the vertical axis, as the coordinate systems
     * of the texture and the picture differ on the axis: the picture has its
     * (0,0) coordinate in the top-left corner, and the texture in the bottom left */
    stbi_set_flip_vertically_on_load(bFlipImage);

    /* creates a memory area, reads the file from the system, flips the image as
     * instructed before, and fills the width, height, and channels with the values found in the image. */
    unsigned char *TextureData = stbi_load(TextureFilename.c_str(), &mTexWidth,
        &mTexHeight, &mNumberOfChannels, 0);

    if (!TextureData)
    {
        Logger::Log(1, "%s error: could not load file '%s'\n", __FUNCTION__, mTextureName.c_str());
        // Freeing memory allocated by stb (careful with memory leaks!)
        stbi_image_free(TextureData);
        return false;
    }

    glGenTextures(1, &mTexture);
    glBindTexture(GL_TEXTURE_2D, mTexture);

    /* For minification, we use trilinear sampling; for the magnification,
     * there is only linear filtering available */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    /* repeat the texture outside the range of 0 to 1.
     * Is like using only the fractional part of the texture coordinate, ignoring the integer part. */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GLenum InternalFormat = 0;
    GLenum DataFormat = 0;

    if (mNumberOfChannels == 4)
    {
        InternalFormat = GL_RGBA8;
        DataFormat = GL_RGBA;
    }
    else if (mNumberOfChannels == 3)
    {
        InternalFormat = GL_RGB8;
        DataFormat = GL_RGB;
    }
    else if (mNumberOfChannels == 1)
    {
        InternalFormat = GL_R8;
        DataFormat = GL_RED;
    }

    /*  using the loaded byte data from stbi_load() and the width,height to push the data
     *  from system memory to the GPU. */
    glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, mTexWidth, mTexHeight,
        0, DataFormat, GL_UNSIGNED_BYTE, TextureData);

    // scaled-down version of the original image, halving the width and height in every step
    /* reduced images will be 1/4, 1/16, 1/256, and so on of the original size, until a configurable limit
     * is reached, or the size is 1x1 pixel. The mipmaps are used to increase rendering speed,
     * as less data is read for far away textures, also reduces artifacts. */
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(TextureData);

    Logger::Log(1, "%s: texture '%s' loaded (%dx%d, %d channels)\n", __FUNCTION__, mTextureName.c_str(), mTexWidth, mTexHeight, mNumberOfChannels);
    return true;
}

void Texture::Bind()
{
    glBindTexture(GL_TEXTURE_2D, mTexture);
}

void Texture::Unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::Cleanup()
{
    glDeleteTextures(1, &mTexture);
}
