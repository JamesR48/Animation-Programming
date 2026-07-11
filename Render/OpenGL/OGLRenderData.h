#ifndef CPPANIMPROGRAMMING_OGLRENDERDATA_H
#define CPPANIMPROGRAMMING_OGLRENDERDATA_H

#include <string>
#include <vector>
#include <glm/glm.hpp>

constexpr size_t MAX_GLTF_NODES = 256;

enum class ESkinningMode
{
    Linear = 0,
    DualQuat
};

enum class EPlaybackDirection
{
    Forward = 0,
    Backward
};

enum class EBlendMode
{
    FadeInOut = 0,
    CrossFade,
    Additive
};

enum class EIKSolver
{
    None = 0,
    CCD,
    FABRIK
};

struct OGLVertex
{
    glm::vec3 Position;
    glm::vec3 Color;
    glm::vec2 UV;
};

struct OGLMesh
{
    std::vector<OGLVertex> Vertices;
};

struct GLFWwindow;

struct OGLRenderData
{
    GLFWwindow *rdWindow = nullptr;
    int rdWidth = 0;
    int rdHeight = 0;

    unsigned int rdTriangleCount = 0;
    unsigned int rdGltfTriangleCount = 0;

    float rdFieldOfView = 90.0f;
    bool rdUseChangedShader = false;

    float rdFrameTime = 0.0f;
    float rdMatrixGenerateTime = 0.0f;
    float rdUploadToVBOTime = 0.0f;
    float rdUploadToUBOTime = 0.0f;
    float rdUIGenerateTime = 0.0f;
    float rdUIDrawTime = 0.0f;
    float rdIKTime = 0.0f;

    // clockwise rotation around an imaginary vertical line pointing upward from the center of coordinate system
    // a.k.a. Yaw
    float rdViewAzimuth = 0.0f;
    //angle of the height of the object, as seen from the center of coordinate system (Pitch)
    float rdViewElevation = -16.5f;

    // +1 forward, -1 backward
    int rdMoveForward = 0;
    // +1 left, -1 right
    int rdMoveRight = 0;
    // +1 up, -1 down
    int rdMoveUp = 0;
    float rdDeltaTime = 0.0f;
    glm::vec3 rdCameraWorldPosition = glm::vec3(0.0f, 4.2f, 4.5f);
    glm::vec3 rdLightPostion = glm::vec3(4.0f, 5.0f, -3.0f);

    bool rdDrawWorldCoordArrows = true;
    bool rdDrawModelCoordArrows = true;
    bool rdDrawSplineLines = true;
    bool rdResetAnglesAndInterp = false;

    std::vector<int> rdRotXAngle = { 0, 0 };
    std::vector<int> rdRotYAngle = { 0, 0 };
    std::vector<int> rdRotZAngle = { 0, 0 };

    glm::vec3 rdSplineStartVertex = glm::vec3(0.0f);
    glm::vec3 rdSplineStartTangent = glm::vec3(0.0f);
    glm::vec3 rdSplineEndVertex = glm::vec3(0.0f);
    glm::vec3 rdSplineEndTangent = glm::vec3(0.0f);

    float rdInterpValue = 0.0f;

    ESkinningMode rdSkinningMode = ESkinningMode::Linear;
    EBlendMode rdBlendingMode = EBlendMode::FadeInOut;
    EPlaybackDirection rdAnimationPlayDirection = EPlaybackDirection::Forward;
    EIKSolver rdIKSolver = EIKSolver::None;

    bool rdDrawGltfModel = true;
    bool rdDrawSkeleton = true;
    bool rdGPUVertexSkinning = true;

    bool rdPlayAnimation = true;
    std::vector<std::string> rdClipNames{};
    int rdAnimClip = 0;
    int rdAnimClipSize = 0;
    float rdAnimSpeed = 1.0f;
    float rdAnimTimePosition = 0.0f;
    float rdAnimEndTime = 0.0f;
    float rdAnimBlendFactor = 1.0f;
    int rdModelNodeCount = 0;

    int rdCrossBlendDestAnimClip = 0;
    float rdAnimCrossBlendFactor = 0.0f;

    int rdSkelSplitNode = 0;
    std::vector<std::string> rdSkelNodeNames{};

    int rdIkIterations = 10;
    glm::vec3 rdIkTargetPos = glm::vec3(0.0f, 3.0f, 1.0f);
    int rdIkEffectorNode = 0;
    int rdIkRootNode = 0;
};

#endif //CPPANIMPROGRAMMING_OGLRENDERDATA_H
