#ifndef CPPANIMPROGRAMMING_MODELSETTINGS_H
#define CPPANIMPROGRAMMING_MODELSETTINGS_H

#include "../Render/OpenGL/OGLRenderData.h"
#include <glm/glm.hpp>

struct ModelSettings
{
    glm::vec3 msWorldPosition = glm::vec3(0.0f);
    glm::vec3 msWorldRotation = glm::vec3(0.0f);

    /* rendering */
    bool msDrawModel = true;
    bool msDrawSkeleton = false;
    ESkinningMode msVertexSkinningMode = ESkinningMode::Linear;

    /* animation */
    bool msPlayAnimation = true;
    EPlaybackDirection msAnimationPlayDirection = EPlaybackDirection::Forward;
    int msAnimClip = 0;
    float msAnimSpeed = 1.0f;
    float msAnimTimePosition = 0.0f;
    float msAnimEndTime = 0.0f;

    /* blending */
    EBlendMode msBlendingMode = EBlendMode::FadeInOut;
    float msAnimBlendFactor = 1.0f;
    int msCrossBlendDestAnimClip = 0;
    float msAnimCrossBlendFactor = 0.0f;
    int msSkelSplitNode = 0;

    /* IK */
    EIKSolver msIKSolver = EIKSolver::None;
    int msIKIterations = 10;
    // relative to local model origin
    glm::vec3 msIKTargetPos = glm::vec3(0.0f, 3.0f, 1.0f);
    int msIKEffectorNode = 0;
    int msIKRootNode = 0;
    glm::vec3 msIKTargetWorldPos = glm::vec3(0.0f, 0.0f,1.0f);

    /* names stored for displaying them on the UI */
    std::vector<std::string> msClipNames{};
    std::vector<std::string> msSkelNodeNames{};
};

#endif