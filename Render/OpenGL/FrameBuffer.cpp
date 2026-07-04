#include "FrameBuffer.h"
#include <GLFW/glfw3.h>
#include "../../Tools/Logger.h"

bool Framebuffer::Init(unsigned int Width, unsigned int Height)
{
    mBufferWidth = Width;
    mBufferHeight = Height;
    glGenFramebuffers(1, &mBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, mBuffer);

    /* color texture */
    glGenTextures(1, &mColorTex);
    glBindTexture(GL_TEXTURE_2D, mColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width,
    Height,  0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    /* handling of downscaling (minification) the texture if it is drawn far away,
     * or upscaling (magnification) when it is close to the viewer.
     * GL_NEAREST means no filtering at all */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    /* What happens on the positive and negative edges of the texture when
     * drawing outside the defined area of the texture. Edge-clamping sets
     * the value of the texture data of its x or y position to 0.0 if
     * requested position is <0, and the texture data of the position to 1.0
     if we are requesting a position of >1 */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Unbinding the texture (passing invalid 0 value)
    glBindTexture(GL_TEXTURE_2D, 0);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mColorTex, 0);
    Logger::Log(1, "%s: added color buffer\n", __FUNCTION__);

    /* render buffer as depth buffer */
    /* While a pixel in the color attachment is about to be written,
     * the depth buffer will be checked to see whether the pixel is closer
     * to the viewer compared to a pixel already in that position (if any).
     * If the new pixel is from a triangle closer to the viewer position,
     * the depth buffer will be updated with the new, nearer value and the
     * color attachment will be drawn. If it is further away, both writes are discarded. */
    glGenRenderbuffers(1, &mDepthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, Width, Height);

    // binding the created renderbuffer as a depth attachment  so OpenGL knows it is a depth buffer instead of a color buffer
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBuffer);

    // Unbinding renderbuffer
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    Logger::Log(1, "%s: added depth renderbuffer\n", __FUNCTION__);

    // Unbinding framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return CheckComplete();
}

bool Framebuffer::Resize(unsigned int NewWidth, unsigned int NewHeight)
{
    Logger::Log(1, "%s: resizing framebuffer from %dx%d to %dx%d\n", __FUNCTION__, mBufferWidth, mBufferHeight, NewWidth, NewHeight);
    mBufferWidth = NewWidth;
    mBufferHeight = NewHeight;

    // unbinding framebuffer, and removing the created OpenGL objects for it for reinitialization
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glDeleteTextures(1, &mColorTex);
    glDeleteRenderbuffers(1, &mDepthBuffer);
    glDeleteFramebuffers(1, &mBuffer);

    return Init(NewWidth, NewHeight);
}

void Framebuffer::Bind()
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mBuffer);
}

void Framebuffer::Unbind()
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void Framebuffer::DrawToScreen()
{
    /* binding the framebuffer we draw to as the framebuffer to read from,
     * and the window as the output (draw) framebuffer. Then, we “blit”
     * the contents of the internal framebuffer to the window.
     * Blitting is a fast memory copy the contents of one framebuffer to another */
    glBindFramebuffer(GL_READ_FRAMEBUFFER, mBuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, mBufferWidth, mBufferHeight, 0,
    0, mBufferWidth, mBufferHeight, GL_COLOR_BUFFER_BIT,
    GL_NEAREST);

    // unbinding the internal framebuffer to stop reading from it
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

void Framebuffer::Cleanup()
{
    Unbind();
    glDeleteTextures(1, &mColorTex);
    glDeleteRenderbuffers(1, &mDepthBuffer);
    glDeleteFramebuffers(1, &mBuffer);
}

bool Framebuffer::CheckComplete()
{
    glBindFramebuffer(GL_FRAMEBUFFER, mBuffer);

    /* verifying that the framebuffer has all the data needed to work
     and that all buffer types are complete and correct. */
    GLenum Result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (Result != GL_FRAMEBUFFER_COMPLETE)
    {
        Logger::Log(1, "%s error: framebuffer is NOT complete\n", __FUNCTION__);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    Logger::Log(1, "%s: framebuffer is complete\n", __FUNCTION__);
    return true;
}
