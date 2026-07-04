#ifndef CPPANIMPROGRAMMING_VERTEXBUFFER_H
#define CPPANIMPROGRAMMING_VERTEXBUFFER_H

#include <glad/gl.h>
#include "OGLRenderData.h"

class VertexBuffer
{
public:
    void Init();
    void UploadData(OGLMesh VertexData);
    void Bind();
    void Unbind();
    void Draw(GLuint Mode, unsigned int Start, unsigned int NumVertices);
    void BindAndDraw(GLuint Mode, unsigned int Start, unsigned int Num);
    void Cleanup();

private:
    // OpenGL handles for storing vertex array and vertex buffer, for the vertex data
    GLuint mVAO = 0;
    GLuint mVertexVBO = 0;
};


#endif //CPPANIMPROGRAMMING_VERTEXBUFFER_H
