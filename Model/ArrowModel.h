/* a simple, line based arrow */
#ifndef CPPANIMPROGRAMMING_ARROW_MODEL_H
#define CPPANIMPROGRAMMING_ARROW_MODEL_H

#include "../Render/OpenGL/OGLRenderData.h"

class ArrowModel {
  public:
    OGLMesh GetVertexData();

  private:
    void Init();
    OGLMesh mVertexData;
};

#endif