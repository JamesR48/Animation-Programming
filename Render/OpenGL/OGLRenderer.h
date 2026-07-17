#ifndef CPPANIMPROGRAMMING_OGLRENDERER_H
#define CPPANIMPROGRAMMING_OGLRENDERER_H

#include "OGLRenderData.h"
#include <memory>
#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>

struct GLFWwindow;
class Shader;
class Framebuffer;
class VertexBuffer;
class UniformBuffer;
class ShaderStorageBuffer;
class UserInterface;
class Camera;
class Timer;
class Model_OpenGL;
class CoordArrowsModel;
class ArrowModel;
class SplineModel;
class GltfModel;
class GltfInstance;

class OGLRenderer
{
public:
    OGLRenderer(GLFWwindow *Window);
    ~OGLRenderer(); // Required explicitly for std::unique_ptr with forward declarations

    bool Init(unsigned int Width, unsigned int Height);
    void SetSize(unsigned int Width, unsigned int Height);
    void Draw();
    void Cleanup();

    void HandleKeyEvents(int Key, int Scancode, int Action, int Mods);
    void HandleMouseButtonEvents(int Button, int Action, int Mods);
    void HandleMousePositionEvents(double XPos, double YPos);

private:
    // subsystems
    std::unique_ptr<Framebuffer> mFramebuffer = nullptr;
    std::unique_ptr<VertexBuffer> mVertexBuffer = nullptr;
    std::unique_ptr<UniformBuffer> mUniformBuffer = nullptr;
    std::unique_ptr<ShaderStorageBuffer> mGltfShaderStorageBuffer = nullptr;
    std::unique_ptr<ShaderStorageBuffer> mGltfDualQuatSSBuffer = nullptr;
    std::unique_ptr<UserInterface> mUserInterface = nullptr;
    std::unique_ptr<Camera> mCamera = nullptr;

    // timers
    std::unique_ptr<Timer> mFrameTimer = nullptr;
    std::unique_ptr<Timer> mMatrixGenerateTimer = nullptr;
    std::unique_ptr<Timer> mUploadToVBOTimer = nullptr;
    std::unique_ptr<Timer> mUploadToUBOTimer = nullptr;
    std::unique_ptr<Timer> mUIGenerateTimer = nullptr;
    std::unique_ptr<Timer> mUIDrawTimer = nullptr;
    std::unique_ptr<Timer> mIKTimer = nullptr;

    // shaders
    std::unique_ptr<Shader> mLineShader = nullptr;
    std::unique_ptr<Shader> mGltfShader = nullptr;
    std::unique_ptr<Shader> mGltfGPUShader = nullptr;
    std::unique_ptr<Shader> mGltfGPUDualQuatShader = nullptr;

    // models
    std::unique_ptr<CoordArrowsModel> mCoordArrowsModel = nullptr;
    OGLMesh mCoordArrowsMesh{};

    std::unique_ptr<OGLMesh> mLineMesh = nullptr;
    unsigned int mSkeletonLineIndexCount = 0;
    unsigned int mCoordArrowsLineIndexCount = 0;

    std::vector<std::shared_ptr<GltfModel>> mGltfModels{};

    std::vector<std::shared_ptr<GltfInstance>> mGltfInstances{};
    /* instances using joint matrices */
    std::vector<std::shared_ptr<GltfInstance>> mGltfMatrixInstances{};
    /* instances using dual quats */
    std::vector<std::shared_ptr<GltfInstance>> mGltfDQInstances{};

    std::vector<glm::mat4> mModelJointMatrices{};
    std::vector<glm::mat2x4> mModelJointDualQuats{};

    OGLRenderData mRenderData{};

    bool mMouseLock = false;
    int mMouseXPos = 0;
    int mMouseYPos = 0;

    double mLastTickTime = 0.0;
    glm::mat4 mViewMatrix = glm::mat4(1.0f);
    glm::mat4 mProjectionMatrix = glm::mat4(1.0f);

    void handleMovementKeys();
};

#endif //CPPANIMPROGRAMMING_OGLRENDERER_H