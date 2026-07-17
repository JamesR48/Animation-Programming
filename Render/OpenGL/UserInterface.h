#ifndef CPPANIMPROGRAMMING_USERINTERFACE_H
#define CPPANIMPROGRAMMING_USERINTERFACE_H

#include "OGLRenderData.h"
#include "../../Model/ModelSettings.h"

class UserInterface
{
public:
    void Init(const OGLRenderData& RenderData);
    void CreateFrame(OGLRenderData& InOutRenderData, ModelSettings& InOutModelSettings);
    void Render();
    void Cleanup();

private:
    float mFramesPerSecond = 0.0f;
    /* averaging speed */
    float mAveragingAlpha = 0.96f;

    std::vector<float> mFPSValues{};
    int mNumFPSValues = 90;

    std::vector<float> mFrameTimeValues{};
    int mNumFrameTimeValues = 90;

    std::vector<float> mModelUploadValues{};
    int mNumModelUploadValues = 90;

    std::vector<float> mMatrixGenerationValues{};
    int mNumMatrixGenerationValues = 90;

    std::vector<float> mMatrixUploadValues{};
    int mNumMatrixUploadValues = 90;

    std::vector<float> mUiGenValues{};
    int mNumUiGenValues = 90;

    std::vector<float> mUiDrawValues{};
    int mNumUiDrawValues = 90;

    std::vector<float> mIKValues{};
    int mNumIKValues = 90;
};

#endif //CPPANIMPROGRAMMING_USERINTERFACE_H
