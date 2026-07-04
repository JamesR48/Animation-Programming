#ifndef CPPANIMPROGRAMMING_COORD_ARROWS_MODEL_H
#define CPPANIMPROGRAMMING_COORD_ARROWS_MODEL_H

#include "../Render/OpenGL/OGLRenderData.h"

class CoordArrowsModel {
  public:
    OGLMesh GetVertexData();

  private:
    void Init();

    OGLMesh mVertexData;
};

#endif
