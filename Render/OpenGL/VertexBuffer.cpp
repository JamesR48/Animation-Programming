#include "VertexBuffer.h"
#include <vector>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include "../../Tools/Logger.h"

void VertexBuffer::Init()
{
    // Create a new vertex array object (containing the vertex buffer)
    glGenVertexArrays(1, &mVAO);
    // create new vertex buffer object (containing vertex and texture data)
    glGenBuffers(1, &mVertexVBO);

    glBindVertexArray(mVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexVBO);

    /* configuring the buffer object – it has input location 0 in shaders,
     * and it has three non normalized elements of the float type; they are packed
     * tight with a stride (offset between 2 consecutive elements) of the size of the
     * vertex struct we created, consisting of the position and the texture coordinate.
     * The last parameter is the offset inside the OGLVertex struct; the C++ offsetof macro
     * gets the offsets of the position and the texture coordinates elements. offset values
     * are casted to void* to match the signature of the call. */
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
sizeof(OGLVertex), (void*) offsetof(OGLVertex, Position));

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
sizeof(OGLVertex), (void*) offsetof(OGLVertex, Color));

    /*  A similar initialization is made for the texture data, but it uses location 1 with only two floats */
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
    sizeof(OGLVertex), (void*) offsetof(OGLVertex, UV));

    // enabling the vertex buffers of 0 and 1, which we just configured.
    // The enabled status of the arrays will be stored in the vertex array object too
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    // unbinding the array buffer and the vertex array
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    Logger::Log(1, "%s: VAO and VBO initialized\n", __FUNCTION__);
}

void VertexBuffer::UploadData(OGLMesh VertexData)
{
    glBindVertexArray(mVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexVBO);

    /* uploads the vertex data to the OpenGL buffer; calculates the size
     * by multiplying the number of elements in the vector by the size of our
     * custom vertex data type. And it needs the starting address for the memory
     * copy, given by the address of the first vertex element. GL_DYNAMIC_DRAW is
     * a hint for the driver that the data will be written and used multiple times,
     * but it's just a hint, the driver will decide where to store the data internally.
     * The same upload follows for the texture data */
    glBufferData(GL_ARRAY_BUFFER, VertexData.Vertices.size() * sizeof(OGLVertex), &VertexData.Vertices.at(0),
      GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void VertexBuffer::Bind()
{
    glBindVertexArray(mVAO);
}

void VertexBuffer::Unbind()
{
    glBindVertexArray(0);
}

void VertexBuffer::Draw(GLuint Mode, unsigned int Start, unsigned int NumVertices)
{
    /* draw the vertex array from the currently bound vertex
     * array object, starting at the start index and rendering
     * num vertices. They are drawn in a rendering mode (GL_TRIANGLES in this case).  */
    glDrawArrays(Mode, Start, NumVertices);
}

void VertexBuffer::BindAndDraw(GLuint Mode, unsigned int Start, unsigned int Num)
{
    Bind();
    glDrawArrays(Mode, Start, Num);
    Unbind();
}

void VertexBuffer::Cleanup()
{
    glDeleteBuffers(1, &mVertexVBO);
    glDeleteVertexArrays(1, &mVAO);
}
