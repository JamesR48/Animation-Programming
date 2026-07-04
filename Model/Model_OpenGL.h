#ifndef CPPANIMPROGRAMMING_MODEL_H
#define CPPANIMPROGRAMMING_MODEL_H

#include "../Render/OpenGL/OGLRenderData.h"

class Model_OpenGL
{
public:
    void Init();
    OGLMesh GetVertexData();

private:
    OGLMesh mVertexData;
};

#endif //CPPANIMPROGRAMMING_MODEL_H
