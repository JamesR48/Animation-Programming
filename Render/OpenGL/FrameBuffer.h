#ifndef CPPANIMPROGRAMMING_FRAMEBUFFER_H
#define CPPANIMPROGRAMMING_FRAMEBUFFER_H

#include <glad/gl.h>

class Framebuffer
{
public:
    bool Init(unsigned int Width, unsigned int Height);
    bool Resize(unsigned int NewWidth, unsigned int NewHeight);
    // enable/disable the drawing to the framebuffers
    void Bind();
    void Unbind();

    /* copies the data to the GLFW window.
     * it's drawn internally to a separate buffer and not directly to the screen */
    void DrawToScreen();
    void Cleanup();

private:
    unsigned int mBufferWidth = 640;
    unsigned int mBufferHeight = 480;

    // overall framebuffer we draw to
    GLuint mBuffer = 0;
    //  color texture used as data storage for the framebuffer
    GLuint mColorTex = 0;
    /* Distance from the viewer to every pixel to ensure
     * that only the color value nearest to the viewer will be drawn */
    GLuint mDepthBuffer = 0;

    // Check if the framebuffer contains all components required to draw
    bool CheckComplete();
};


#endif //CPPANIMPROGRAMMING_FRAMEBUFFER_H
