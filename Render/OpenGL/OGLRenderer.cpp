#include "OGLRenderer.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <algorithm>
#include <vector>
#include <string>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui_impl_glfw.h>
#include <glm/gtx/spline.hpp>

#include "Framebuffer.h"
#include "VertexBuffer.h"
#include "UniformBuffer.h"
#include "ShaderStorageBuffer.h"
#include "Shader.h"
#include "UserInterface.h"
#include "../../Tools/Logger.h"
#include "../../Tools/Timer.h"
#include "../../Tools/Camera.h"
#include "../../Model/ArrowModel.h"
#include "../../Model/CoordArrowsModel.h"
#include "../../Model/GltfModel.h"
#include "../../Model/GltfInstance.h"
#include "../../Model/SplineModel.h"

OGLRenderer::OGLRenderer(GLFWwindow *Window)
{
    mRenderData.rdWindow = Window;
}

OGLRenderer::~OGLRenderer() = default;

bool OGLRenderer::Init(unsigned int Width, unsigned int Height)
{
    /* ensuring different random values at every start */
    std::srand(static_cast<int>(time(NULL)));

    /* required for perspective */
    mRenderData.rdWidth = Width;
    mRenderData.rdHeight = Height;

    /* initalize GLAD */
    if (!gladLoadGL(glfwGetProcAddress))
    {
        Logger::Log(1, "%s error: failed to initialize GLAD\n", __FUNCTION__);
        return false;
    }

    if (!GLAD_GL_VERSION_4_6)
    {
        Logger::Log(1, "%s error: failed to get at least OpenGL 4.6\n", __FUNCTION__);
        return false;
    }

    GLint majorVersion, minorVersion;
    glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
    Logger::Log(1, "%s: OpenGL %d.%d initializeed\n", __FUNCTION__, majorVersion, minorVersion);

    // Safely allocate objects on the heap after GL context initialization
    mLineShader = std::make_unique<Shader>();
    mGltfShader = std::make_unique<Shader>();
    mGltfGPUShader = std::make_unique<Shader>();
    mGltfGPUDualQuatShader = std::make_unique<Shader>();

    mFramebuffer = std::make_unique<Framebuffer>();
    mVertexBuffer = std::make_unique<VertexBuffer>();
    mUniformBuffer = std::make_unique<UniformBuffer>();
    mGltfShaderStorageBuffer = std::make_unique<ShaderStorageBuffer>();
    mGltfDualQuatSSBuffer = std::make_unique<ShaderStorageBuffer>();
    mUserInterface = std::make_unique<UserInterface>();
    mCamera = std::make_unique<Camera>();

    if (!mFramebuffer->Init(Width, Height))
    {
        Logger::Log(1, "%s error: could not init Framebuffer\n", __FUNCTION__);
        return false;
    }
    Logger::Log(1, "%s: framebuffer succesfully initialized\n", __FUNCTION__);

    mVertexBuffer->Init();
    Logger::Log(1, "%s: vertex buffer successfully created\n", __FUNCTION__);

    size_t UniformMatrixBufferSize = 2 * sizeof(glm::mat4);
    mUniformBuffer->Init(UniformMatrixBufferSize);
    Logger::Log(1, "%s: uniform buffer successfully created\n", __FUNCTION__);

    if (!mLineShader->LoadShaders( "Shaders/Line.vert", "Shaders/Line.frag"))
    {
        Logger::Log(1, "%s: line shader loading failed\n", __FUNCTION__);
        return false;
    }

    if (!mGltfShader->LoadShaders( "Shaders/gltf.vert", "Shaders/gltf.frag"))
    {
        Logger::Log(1, "%s: gltTF shader loading failed\n", __FUNCTION__);
        return false;
    }

    if (!mGltfGPUShader->LoadShaders( "Shaders/gltf_gpu.vert", "Shaders/gltf_gpu.frag"))
    {
        Logger::Log(1, "%s: gltTF GPU shader loading failed\n", __FUNCTION__);
        return false;
    }
    if (!mGltfGPUShader->GetUniformLocation("aModelStride"))
    {
        Logger::Log(1, "%s: failed to get model stride uniform for gltTF GPU shader\n", __FUNCTION__);
        return false;
    }

    if (!mGltfGPUDualQuatShader->LoadShaders( "Shaders/gltf_gpu_dualquat.vert", "Shaders/gltf_gpu_dualquat.frag"))
    {
        Logger::Log(1, "%s: glTF GPU dual quat shader loading failed\n", __FUNCTION__);
        return false;
    }
    if (!mGltfGPUDualQuatShader->GetUniformLocation("aModelStride"))
    {
        Logger::Log(1, "%s: failed to get model stride uniform for gltTF GPU dual quat shader\n", __FUNCTION__);
        return false;
    }
    Logger::Log(1, "%s: shaders succesfully loaded\n", __FUNCTION__);

    mUserInterface->Init(mRenderData);
    Logger::Log(1, "%s: user interface initialized\n", __FUNCTION__);

    /* adding backface culling and depth test */
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glLineWidth(3.0);

    /* disable sRGB framebuffer */
    glDisable(GL_FRAMEBUFFER_SRGB);

    /* loading 2x the woman and the dq model */
    mGltfModels.resize(3);

    std::string ModelFilename = "Resources/Assets/Woman.gltf";
    std::string ModelTexFilename = "Resources/Textures/Woman.png";
    for (int idx = 0; idx < 2; idx++)
    {
        if (idx == 1)
        {
            ModelTexFilename = "Resources/Textures/Woman2.png";
        }

        mGltfModels.at(idx) = std::make_shared<GltfModel>();
        if (!mGltfModels.at(idx)->LoadModel(mRenderData, ModelFilename, ModelTexFilename))
        {
            Logger::Log(1, "%s: loading glTF model '%s' failed\n", __FUNCTION__, ModelFilename.c_str());
            return false;
        }
        mGltfModels.at(idx)->UploadVertexBuffers();
        mGltfModels.at(idx)->UploadIndexBuffer();
        Logger::Log(1, "%s: glTF model '%s' succesfully loaded\n", __FUNCTION__, ModelFilename.c_str());
    }

    mGltfModels.at(2) = std::make_shared<GltfModel>();
    ModelFilename = "Resources/Assets/dq.gltf";
    ModelTexFilename = "Resources/Textures/dq.png";
    if (!mGltfModels.at(2)->LoadModel(mRenderData, ModelFilename, ModelTexFilename))
    {
        Logger::Log(1, "%s: loading glTF model '%s' failed\n", __FUNCTION__, ModelFilename.c_str());
        return false;
    }
    mGltfModels.at(2)->UploadVertexBuffers();
    mGltfModels.at(2)->UploadIndexBuffer();
    Logger::Log(1, "%s: glTF model '%s' succesfully loaded\n", __FUNCTION__, ModelFilename.c_str());

    int NumTriangles = 0;

    /* creating glTF instances from the model */
    for (int i = 0; i < 200; ++i)
    {
        int xPos = std::rand() % 40 - 20;
        int zPos = std::rand() % 40 - 20;
        int ModelIndex = std::rand() % 2;
        const glm::vec3 InstanceWorldPos = glm::vec3(static_cast<float>(xPos),0.0f, static_cast<float>(zPos));
        auto Instance = std::make_shared<GltfInstance>(mGltfModels.at(ModelIndex), InstanceWorldPos, true);
        mGltfInstances.emplace_back(Instance);
        NumTriangles += mGltfModels.at(ModelIndex)->GetTriangleCount();
    }

    for (int i = 0; i < 25; ++i)
    {
        int xPos = std::rand() % 50 - 25;
        int zPos = std::rand() % 20 - 50;
        const glm::vec3 InstanceWorldPos = glm::vec3(static_cast<float>(xPos),0.0f, static_cast<float>(zPos));
        auto Instance = std::make_shared<GltfInstance>(mGltfModels.at(2), InstanceWorldPos, true);
        mGltfInstances.emplace_back(Instance);
        NumTriangles += mGltfModels.at(2)->GetTriangleCount();
    }

    /* data to be displayed on the UI */
    mRenderData.rdTriangleCount = NumTriangles;
    mRenderData.rdNumberOfInstances = mGltfInstances.size();

    size_t ModelJointMatrixBufferSize = 0;
    size_t ModelJointDualQuatBufferSize = 0;
    int JointMatrixSize = 0;
    int JointQuatSize = 0;

    /* adding up the sizes of each instance for the SSBO's overall size */
    for (const std::shared_ptr<GltfInstance>& Instance : mGltfInstances)
    {
        JointMatrixSize += Instance->GetJointMatrixSize();
        ModelJointMatrixBufferSize += Instance->GetJointMatrixSize() * sizeof(glm::mat4);

        JointQuatSize += Instance->GetJointDualQuatsSize();
        ModelJointDualQuatBufferSize += Instance->GetJointDualQuatsSize() * sizeof(glm::mat2x4);
    }

    mGltfShaderStorageBuffer->Init(ModelJointMatrixBufferSize);
    Logger::Log(1, "%s: glTF joint matrix shader storage buffer (size %i bytes) successfully created\n", __FUNCTION__, ModelJointMatrixBufferSize);

    mGltfDualQuatSSBuffer->Init(ModelJointDualQuatBufferSize);
    Logger::Log(1, "%s: glTF joint dual quaternions shader storage buffer (size %i bytes) successfully created\n", __FUNCTION__, ModelJointDualQuatBufferSize);

    mFrameTimer = std::make_unique<Timer>();
    mMatrixGenerateTimer = std::make_unique<Timer>();
    mUploadToUBOTimer = std::make_unique<Timer>();
    mUploadToVBOTimer = std::make_unique<Timer>();
    mUIGenerateTimer = std::make_unique<Timer>();
    mUIDrawTimer = std::make_unique<Timer>();
    mIKTimer = std::make_unique<Timer>();

    mCoordArrowsModel = std::make_unique<CoordArrowsModel>();
    Logger::Log(1, "%s: coord arrows model mesh storage initialized\n", __FUNCTION__);

    /* valid, but emtpy */
    mLineMesh = std::make_unique<OGLMesh>();
    Logger::Log(1, "%s: line mesh storage initialized\n", __FUNCTION__);

    mFrameTimer->Start();
    return true;
}

void OGLRenderer::SetSize(unsigned int Width, unsigned int Height)
{
    /* handle minimize */
    if (Width == 0 || Height == 0)
    {
        return;
    }

    mRenderData.rdWidth = Width;
    mRenderData.rdHeight = Height;

    mFramebuffer->Resize(Width, Height);
    glViewport(0, 0, Width, Height);

    Logger::Log(1, "%s: resized window to %dx%d\n", __FUNCTION__, Width, Height);
}

void OGLRenderer::Draw()
{
    /* handle minimize */
    while (mRenderData.rdWidth == 0 || mRenderData.rdHeight == 0)
    {
        glfwGetFramebufferSize(mRenderData.rdWindow, &mRenderData.rdWidth, &mRenderData.rdHeight);
        glfwWaitEvents();
    }

    /* get time difference for movement */
    double TickTime = glfwGetTime();
    mRenderData.rdDeltaTime = TickTime - mLastTickTime;

    mRenderData.rdFrameTime = mFrameTimer->Stop();
    mFrameTimer->Start();

    handleMovementKeys();

    // Let the framebuffer receive our vertex data.
    mFramebuffer->Bind();

    // Clear screen and depth buffer
    glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mMatrixGenerateTimer->Start();
    // FOV, aspect ratio, near z distance, far z distance
    mProjectionMatrix = glm::perspective(glm::radians(mRenderData.rdFieldOfView),
    static_cast<float>(mRenderData.rdWidth) / static_cast<float>(mRenderData.rdHeight), 0.1f, 100.f);

    mViewMatrix = mCamera->GetViewMatrix(mRenderData);

    /* animate and update inverse kinematics */
    mRenderData.rdIKTime = 0.0f;
    for (std::shared_ptr<GltfInstance>& Instance : mGltfInstances)
    {
        Instance->UpdateAnimation();

        mIKTimer->Start();
        Instance->SolveIK();
        mRenderData.rdIKTime += mIKTimer->Stop();
    }

    /* saving the value to avoid changes in the middle of the rendering process
      if another instance is selected in the user interface. */
    int SelectedInstance = mRenderData.rdCurrentSelectedInstance;
    glm::vec3 ModelInstanceWorldPos;
    mGltfInstances.at(SelectedInstance)->GetWorldPosition(ModelInstanceWorldPos);
    glm::quat ModelInstanceWorldQuat;
    mGltfInstances.at(SelectedInstance)->GetWorldRotation(ModelInstanceWorldQuat);

    mLineMesh->Vertices.clear();

    /* get gltTF skeleton */
    mSkeletonLineIndexCount = 0;
    for (const std::shared_ptr<GltfInstance>& Instance : mGltfInstances)
    {
        ModelSettings Settings;
        Instance->GetInstanceSettings(Settings);
        if (Settings.msDrawSkeleton)
        {
            std::shared_ptr<OGLMesh> Mesh = Instance->GetSkeleton();
            mSkeletonLineIndexCount += Mesh->Vertices.size();
            mLineMesh->Vertices.insert(mLineMesh->Vertices.begin(), Mesh->Vertices.begin(), Mesh->Vertices.end());
        }
    }

    /* coordinate arrows for the IK target of current instance only */
    mCoordArrowsLineIndexCount = 0;
    ModelSettings IKSettings;
    mGltfInstances.at(SelectedInstance)->GetInstanceSettings(IKSettings);
    if (IKSettings.msIKSolver != EIKSolver::None)
    {
        mCoordArrowsMesh = mCoordArrowsModel->GetVertexData();
        mCoordArrowsLineIndexCount += mCoordArrowsMesh.Vertices.size();
        std::for_each(mCoordArrowsMesh.Vertices.begin(), mCoordArrowsMesh.Vertices.end(), [=](auto &Vert)
        {
            Vert.Color *= 0.5f;
            Vert.Position = ModelInstanceWorldQuat * Vert.Position;
            Vert.Position += IKSettings.msIKTargetWorldPos;
        });

        mLineMesh->Vertices.insert(mLineMesh->Vertices.end(),
          mCoordArrowsMesh.Vertices.begin(), mCoordArrowsMesh.Vertices.end());
    }

    /* draw coordiante arrows*/
    mCoordArrowsMesh = mCoordArrowsModel->GetVertexData();
    mCoordArrowsLineIndexCount += mCoordArrowsMesh.Vertices.size();
    std::for_each(mCoordArrowsMesh.Vertices.begin(), mCoordArrowsMesh.Vertices.end(),[=](auto &Vert)
    {
        Vert.Color *= 0.5f;
        Vert.Position = ModelInstanceWorldQuat * Vert.Position;
        Vert.Position += ModelInstanceWorldPos;
    });

    mLineMesh->Vertices.insert(mLineMesh->Vertices.end(),
      mCoordArrowsMesh.Vertices.begin(), mCoordArrowsMesh.Vertices.end());

    mRenderData.rdMatrixGenerateTime = mMatrixGenerateTimer->Stop();

    mUploadToUBOTimer->Start();
    std::vector<glm::mat4> UboMatrixData;
    UboMatrixData.push_back(mViewMatrix);
    UboMatrixData.push_back(mProjectionMatrix);
    mUniformBuffer->UploadUboData(UboMatrixData, 0);

    mModelJointMatrices.clear();
    mModelJointDualQuats.clear();

    mGltfMatrixInstances.clear();
    mGltfDQInstances.clear();
    unsigned int NumTriangles = 0;

    for (const std::shared_ptr<GltfInstance>& Instance : mGltfInstances)
    {
        ModelSettings Settings;
        Instance->GetInstanceSettings(Settings);

        if (!Settings.msDrawModel)
        {
            continue;
        }

        if (Settings.msVertexSkinningMode == ESkinningMode::DualQuat)
        {
            std::vector<glm::mat2x4> Quats;
            Instance->GetJointDualQuats(Quats);
            mModelJointDualQuats.insert(mModelJointDualQuats.end(),Quats.begin(), Quats.end());
            mGltfDQInstances.emplace_back(Instance);
        }
        else
        {
            std::vector<glm::mat4> Mats;
            Instance->GetJointMatrices(Mats);
            mModelJointMatrices.insert(mModelJointMatrices.end(), Mats.begin(), Mats.end());
            mGltfMatrixInstances.emplace_back(Instance);
        }
        NumTriangles += Instance->GetGltfModel()->GetTriangleCount();
    }

    mRenderData.rdTriangleCount = NumTriangles;

    mGltfShaderStorageBuffer->UploadSsboData(mModelJointMatrices, 1);
    mGltfDualQuatSSBuffer->UploadSsboData(mModelJointDualQuats, 2);

    mRenderData.rdUploadToUBOTime = mUploadToUBOTimer->Stop();

    /* upload vertex data */
    mUploadToVBOTimer->Start();

    /* upload lines and boxes */
    if (!mLineMesh->Vertices.empty())
    {
        mVertexBuffer->UploadData(*mLineMesh);
    }

    mRenderData.rdUploadToVBOTime = mUploadToVBOTimer->Stop();

    /* draw the glTF model */
    unsigned int MatrixPos = 0;

    mGltfGPUShader->Use();
    for (const std::shared_ptr<GltfInstance>& Instance : mGltfMatrixInstances)
    {
        /* set position inside the SSBO */
        mGltfGPUShader->SetUniformValue(MatrixPos);
        Instance->GetGltfModel()->Draw();
        MatrixPos += Instance->GetJointMatrixSize();
    }

    unsigned int DualQuatPos = 0;

    mGltfGPUDualQuatShader->Use();
    for (const std::shared_ptr<GltfInstance>& Instance : mGltfDQInstances)
    {
        mGltfGPUDualQuatShader->SetUniformValue(DualQuatPos);
        Instance->GetGltfModel()->Draw();
        DualQuatPos += Instance->GetJointDualQuatsSize();
    }

    /* draw the coordinate arrow WITH depth buffer */
    if (mCoordArrowsLineIndexCount > 0)
    {
        mLineShader->Use();
        mVertexBuffer->BindAndDraw(GL_LINES, mSkeletonLineIndexCount, mCoordArrowsLineIndexCount);
    }

    /* draw the skeleton, disable depth test to overlay */
    if (mSkeletonLineIndexCount > 0)
    {
        glDisable(GL_DEPTH_TEST);
        mLineShader->Use();
        mVertexBuffer->BindAndDraw(GL_LINES, 0, mSkeletonLineIndexCount);
        glEnable(GL_DEPTH_TEST);
    }

    mFramebuffer->Unbind();

    /* blit color buffer to screen */
    mFramebuffer->DrawToScreen();

    mUIGenerateTimer->Start();

    ModelSettings Settings;
    mGltfInstances.at(SelectedInstance)->GetInstanceSettings(Settings);
    mUserInterface->CreateFrame(mRenderData, Settings);
    mGltfInstances.at(SelectedInstance)->SetInstanceSettings(Settings);
    mGltfInstances.at(SelectedInstance)->CheckForUpdates();

    mRenderData.rdUIGenerateTime = mUIGenerateTimer->Stop();

    mUIDrawTimer->Start();
    mUserInterface->Render();
    mRenderData.rdUIDrawTime = mUIDrawTimer->Stop();

    mLastTickTime = TickTime;
}

void OGLRenderer::Cleanup()
{
    mLineShader->Cleanup();
    mLineShader.reset();
    mGltfShader->Cleanup();
    mGltfShader.reset();
    mGltfGPUShader->Cleanup();
    mGltfGPUShader.reset();
    mGltfGPUDualQuatShader->Cleanup();
    mGltfGPUDualQuatShader.reset();

    mVertexBuffer->Cleanup();
    mVertexBuffer.reset();
    mFramebuffer->Cleanup();
    mFramebuffer.reset();
    mUniformBuffer->Cleanup();
    mUniformBuffer.reset();
    mGltfShaderStorageBuffer->Cleanup();
    mGltfShaderStorageBuffer.reset();
    mGltfDualQuatSSBuffer->Cleanup();
    mGltfDualQuatSSBuffer.reset();
    mUserInterface->Cleanup();
    mUserInterface.reset();

    mLineMesh.reset();
    mCoordArrowsModel.reset();

    mFrameTimer.reset();
    mMatrixGenerateTimer.reset();
    mUploadToVBOTimer.reset();
    mUploadToUBOTimer.reset();
    mUIGenerateTimer.reset();
    mUIDrawTimer.reset();
    mIKTimer.reset();
}

void OGLRenderer::HandleKeyEvents(int Key, int Scancode, int Action, int Mods)
{
}

void OGLRenderer::HandleMouseButtonEvents(int Button, int Action, int Mods)
{
    ImGuiIO& IO = ImGui::GetIO();
    if ((Button >= 0) && (Button < ImGuiMouseButton_COUNT))
    {
        IO.AddMouseButtonEvent(Button, Action == GLFW_PRESS);
    }

    /* mouse is over an UI panel, don't do anything */
    if (IO.WantCaptureMouse)
    {
        return;
    }

    if (Button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        mMouseLock = !mMouseLock;

        if(Action == GLFW_PRESS)
        {
            if (mMouseLock)
            {
                glfwSetInputMode(mRenderData.rdWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                /* enable raw mode if possible */
                if (glfwRawMouseMotionSupported())
                {
                    glfwSetInputMode(mRenderData.rdWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
                }
            }
        }
        else
        {
            glfwSetInputMode(mRenderData.rdWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

void OGLRenderer::HandleMousePositionEvents(double XPos, double YPos)
{
    ImGuiIO& IO = ImGui::GetIO();
    IO.AddMousePosEvent((float)XPos, (float)YPos);

    /* mouse is over an UI panel, don't do anything */
    if (IO.WantCaptureMouse)
    {
        return;
    }

    /* calculate relative movement from last position */
    int MouseMoveRelX = static_cast<int>(XPos) - mMouseXPos;
    int MouseMoveRelY = static_cast<int>(YPos) - mMouseYPos;

    if (mMouseLock)
    {
        mRenderData.rdViewAzimuth += (MouseMoveRelX / 10.0);
        /* keep between 0 and 360 degree */
        if (mRenderData.rdViewAzimuth < 0.0) {
            mRenderData.rdViewAzimuth += 360.0;
        }
        if (mRenderData.rdViewAzimuth >= 360.0) {
            mRenderData.rdViewAzimuth -= 360.0;
        }

        mRenderData.rdViewElevation -= (MouseMoveRelY / 10.0);
        /* keep between -89 and +89 degree */
        if (mRenderData.rdViewElevation > 89.0) {
            mRenderData.rdViewElevation = 89.0;
        }
        if (mRenderData.rdViewElevation < -89.0) {
            mRenderData.rdViewElevation = -89.0;
        }
    }

    /* save old values*/
    mMouseXPos = static_cast<int>(XPos);
    mMouseYPos = static_cast<int>(YPos);
}

void OGLRenderer::handleMovementKeys()
{
    mRenderData.rdMoveForward = 0;
    if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_W) == GLFW_PRESS)
    {
        mRenderData.rdMoveForward += 1;
    }
    if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_S) == GLFW_PRESS)
    {
        mRenderData.rdMoveForward -= 1;
    }

    mRenderData.rdMoveRight = 0;
    if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_A) == GLFW_PRESS)
    {
        mRenderData.rdMoveRight -= 1;
    }
    if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_D) == GLFW_PRESS)
    {
        mRenderData.rdMoveRight += 1;
    }

    mRenderData.rdMoveUp = 0;
    if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_E) == GLFW_PRESS)
    {
        mRenderData.rdMoveUp += 1;
    }
    if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_Q) == GLFW_PRESS)
    {
        mRenderData.rdMoveUp -= 1;
    }

    /* speed up movement with shift */
    if ((glfwGetKey(mRenderData.rdWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ||
        (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS))
    {
        mRenderData.rdMoveForward *= 4;
        mRenderData.rdMoveRight *= 4;
        mRenderData.rdMoveUp *= 4;
    }
}