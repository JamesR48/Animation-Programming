#ifndef CPPANIMPROGRAMMING_USERINTERFACE_H
#define CPPANIMPROGRAMMING_USERINTERFACE_H

#include "OGLRenderData.h"

class UserInterface
{
public:
    void Init(const OGLRenderData& RenderData);
    void CreateFrame(OGLRenderData& InOutRenderData);
    void Render();
    void Cleanup();

private:
    float mFramesPerSecond = 0.0f;
    /* averaging speed */
    float mAveragingAlpha = 0.96f;
};

#endif //CPPANIMPROGRAMMING_USERINTERFACE_H
