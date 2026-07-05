#include "OGLRenderer.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <algorithm>
#include <vector>
#include <string>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
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
#include "../../Model/SplineModel.h"

OGLRenderer::OGLRenderer(GLFWwindow *Window)
{
    mRenderData.rdWindow = Window;
}

OGLRenderer::~OGLRenderer() = default;

bool OGLRenderer::Init(unsigned int Width, unsigned int Height)
{
    mRenderData.rdWidth = Width;
    mRenderData.rdHeight = Height;

    if (!gladLoadGL(glfwGetProcAddress))
    {
        return false;
    }

    if (!GLAD_GL_VERSION_4_6)
    {
        return false;
    }

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

    mFrameTimer = std::make_unique<Timer>();
    mMatrixGenerateTimer = std::make_unique<Timer>();
    mUploadToUBOTimer = std::make_unique<Timer>();
    mUploadToVBOTimer = std::make_unique<Timer>();
    mUIGenerateTimer = std::make_unique<Timer>();
    mUIDrawTimer = std::make_unique<Timer>();

    mSplineModel = std::make_unique<SplineModel>();
    mArrowModel = std::make_unique<ArrowModel>();
    mCoordArrowsModel = std::make_unique<CoordArrowsModel>();
    Logger::Log(1, "%s: model mesh storage initialized\n", __FUNCTION__);
    mAllMeshes = std::make_unique<OGLMesh>();
    Logger::Log(1, "%s: global mesh storage initialized\n", __FUNCTION__);

    mGltfModel = std::make_unique<GltfModel>();
    std::string ModelFilename = "Resources/Assets/Woman.gltf";
    std::string ModelTexFilename = "Resources/Textures/Woman.png";
    if (!mGltfModel->LoadModel(mRenderData, ModelFilename, ModelTexFilename))
    {
        Logger::Log(1, "%s: loading glTF model '%s' failed\n", __FUNCTION__, ModelFilename.c_str());
        return false;
    }
    mGltfModel->UploadIndexBuffer();
    Logger::Log(1, "%s: glTF model '%s' succesfully loaded\n", __FUNCTION__, ModelFilename.c_str());

    if (!mFramebuffer->Init(Width, Height))
    {
        return false;
    }
    Logger::Log(1, "%s: framebuffer succesfully initialized\n", __FUNCTION__);

    mVertexBuffer->Init();
    Logger::Log(1, "%s: vertex buffer successfully created\n", __FUNCTION__);

    size_t UniformMatrixBufferSize = 2 * sizeof(glm::mat4);
    mUniformBuffer->Init(UniformMatrixBufferSize);
    Logger::Log(1, "%s: uniform buffer successfully created\n", __FUNCTION__);

    size_t ModelJointMatrixBufferSize = mGltfModel->GetJointMatrixSize() * sizeof(glm::mat4);
    mGltfShaderStorageBuffer->Init(ModelJointMatrixBufferSize);
    Logger::Log(1, "%s: glTF joint matrix shader storage buffer (size %i bytes) successfully created\n", __FUNCTION__, ModelJointMatrixBufferSize);

    if (!mLineShader->LoadShaders( "Shaders/Line.vert", "Shaders/Line.frag"))
    {
        return false;
    }

    if (!mGltfShader->LoadShaders( "Shaders/gltf.vert", "Shaders/gltf.frag"))
    {
        return false;
    }

    if (!mGltfGPUShader->LoadShaders( "Shaders/gltf_gpu.vert", "Shaders/gltf_gpu.frag"))
    {
        return false;
    }

    if (!mGltfGPUDualQuatShader->LoadShaders( "Shaders/gltf_gpu_dualquat.vert", "Shaders/gltf_gpu_dualquat.frag"))
    {
        return false;
    }

    size_t ModelJointDualQuatBufferSize = mGltfModel->GetJointDualQuatsSize() * sizeof(glm::mat2x4);
    mGltfDualQuatSSBuffer->Init(ModelJointDualQuatBufferSize);
    Logger::Log(1, "%s: glTF joint dual quaternions shader storage buffer (size %i bytes) successfully created\n", __FUNCTION__, ModelJointDualQuatBufferSize);

    mUserInterface->Init(mRenderData);
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

    mAllMeshes->Vertices.clear();

    // Let the framebuffer receive our vertex data.
    mFramebuffer->Bind();

    // Clear screen and depth buffer
    glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    mMatrixGenerateTimer->Start();
    // FOV, aspect ratio, near z distance, far z distance
    mProjectionMatrix = glm::perspective(glm::radians(mRenderData.rdFieldOfView),
    static_cast<float>(mRenderData.rdWidth) / static_cast<float>(mRenderData.rdHeight), 0.1f, 100.f);

    mViewMatrix = mCamera->GetViewMatrix(mRenderData);

    mRenderData.rdMatrixGenerateTime = mMatrixGenerateTimer->Stop();

    mUploadToUBOTimer->Start();
    std::vector<glm::mat4> UboMatrixData;
    UboMatrixData.push_back(mViewMatrix);
    UboMatrixData.push_back(mProjectionMatrix);
    mUniformBuffer->UploadUboData(UboMatrixData, 0);

    if (mRenderData.rdGPUVertexSkinning)
    {
        if (mRenderData.rdDualQuatVertexSkinning)
        {
            std::vector<glm::mat2x4> JointDualQuatMatrices;
            mGltfModel->GetJointDualQuats(JointDualQuatMatrices);
            mGltfDualQuatSSBuffer->UploadSsboData(JointDualQuatMatrices, 2);
        }
        else
        {
            std::vector<glm::mat4> JointMatrices;
            mGltfModel->GetJointMatrices(JointMatrices);
            mGltfShaderStorageBuffer->UploadSsboData(JointMatrices, 1);
        }
    }

    mRenderData.rdUploadToUBOTime = mUploadToUBOTimer->Stop();

    /* reset all values to zero when UI button is pressed */
    if (mRenderData.rdResetAnglesAndInterp)
    {
        mRenderData.rdResetAnglesAndInterp = false;

        mRenderData.rdRotXAngle = { 0, 0 };
        mRenderData.rdRotYAngle = { 0, 0 };
        mRenderData.rdRotZAngle = { 0, 0 };

        mRenderData.rdInterpValue = 0.0f;

        mRenderData.rdSplineStartVertex = glm::vec3(-4.0f, 1.0f, -2.0f);
        mRenderData.rdSplineStartTangent = glm::vec3(-10.0f, -8.0f, 8.0f);
        mRenderData.rdSplineEndVertex = glm::vec3(4.0f, 2.0f, -2.0f);
        mRenderData.rdSplineEndTangent = glm::vec3(-6.0f, 5.0f, -6.0f);

        mRenderData.rdDrawWorldCoordArrows = true;
        mRenderData.rdDrawModelCoordArrows = true;
        mRenderData.rdDrawSplineLines = true;
    }

    /* create quaternion from angles  */
    for (int i = 0; i < 2; ++i)
    {
        mQuatModelOrientation[i] = glm::normalize(glm::quat(glm::vec3(
          glm::radians(static_cast<float>(mRenderData.rdRotXAngle[i])),
          glm::radians(static_cast<float>(mRenderData.rdRotYAngle[i])),
          glm::radians(static_cast<float>(mRenderData.rdRotZAngle[i]))
        )));

        /* conjugate = same length, but opposite direction*/
        mQuatModelOrientationConjugate[i] = glm::conjugate(mQuatModelOrientation[i]);
    }

    /* interpolate between the two quaternions */
    mQuatMix = glm::slerp(mQuatModelOrientation[0],mQuatModelOrientation[1], mRenderData.rdInterpValue);
    mQuatMixConjugate = glm::conjugate(mQuatMix);

    /* position model on current spline position */
    glm::vec3 InterpolatedPosition = glm::hermite(
      mRenderData.rdSplineStartVertex, mRenderData.rdSplineStartTangent,
      mRenderData.rdSplineEndVertex, mRenderData.rdSplineEndTangent,
      mRenderData.rdInterpValue);

    /* draw a static coordinate system */
    mCoordArrowsMesh.Vertices.clear();
    if (mRenderData.rdDrawWorldCoordArrows)
    {
        mCoordArrowsMesh = mCoordArrowsModel->GetVertexData();
        std::for_each(mCoordArrowsMesh.Vertices.begin(), mCoordArrowsMesh.Vertices.end(),[=](OGLVertex& Vert)
        {
            Vert.Color /= 2.0f;
        });
        mAllMeshes->Vertices.insert(mAllMeshes->Vertices.end(),mCoordArrowsMesh.Vertices.begin(), mCoordArrowsMesh.Vertices.end());
    }

    mStartPosArrowMesh.Vertices.clear();
    mEndPosArrowMesh.Vertices.clear();
    mQuatPosArrowMesh.Vertices.clear();

    if (mRenderData.rdDrawModelCoordArrows)
    {
        /* start position arrow */
        mStartPosArrowMesh = mArrowModel->GetVertexData();
        std::for_each(mStartPosArrowMesh.Vertices.begin(), mStartPosArrowMesh.Vertices.end(),
          [=](OGLVertex& Vert){
            glm::quat Position = glm::quat(0.0f, Vert.Position.x, Vert.Position.y, Vert.Position.z);
            glm::quat NewPosition = mQuatModelOrientation[0] * Position * mQuatModelOrientationConjugate[0];
            Vert.Position.x = NewPosition.x;
            Vert.Position.y = NewPosition.y;
            Vert.Position.z = NewPosition.z;
            Vert.Position += Vert.Position + mRenderData.rdSplineStartVertex;
            Vert.Color = glm::vec3(0.0f, 0.8f, 0.8f);
        });
        mAllMeshes->Vertices.insert(mAllMeshes->Vertices.end(),mStartPosArrowMesh.Vertices.begin(),mStartPosArrowMesh.Vertices.end());

        /* end position arrow */
        mEndPosArrowMesh = mArrowModel->GetVertexData();
        std::for_each(mEndPosArrowMesh.Vertices.begin(), mEndPosArrowMesh.Vertices.end(),
          [=](OGLVertex& Vert){
            glm::quat Position = glm::quat(0.0f, Vert.Position.x, Vert.Position.y, Vert.Position.z);
            glm::quat NewPosition = mQuatModelOrientation[1] * Position * mQuatModelOrientationConjugate[1];
            Vert.Position.x = NewPosition.x + mRenderData.rdSplineEndVertex.x;
            Vert.Position.y = NewPosition.y + mRenderData.rdSplineEndVertex.y;
            Vert.Position.z = NewPosition.z + mRenderData.rdSplineEndVertex.z;
            Vert.Color = glm::vec3(0.80f, 0.8f, 0.0f);
        });
        mAllMeshes->Vertices.insert(mAllMeshes->Vertices.end(),mEndPosArrowMesh.Vertices.begin(),mEndPosArrowMesh.Vertices.end());

        /* quaternion orientation changes arrow */
        mQuatPosArrowMesh = mArrowModel->GetVertexData();
        std::for_each(mQuatPosArrowMesh.Vertices.begin(), mQuatPosArrowMesh.Vertices.end(),
          [=](OGLVertex& Vert){
            glm::quat Position = glm::quat(0.0f, Vert.Position.x, Vert.Position.y, Vert.Position.z);
            glm::quat NewPosition = mQuatMix * Position * mQuatMixConjugate;
            Vert.Position.x = NewPosition.x + InterpolatedPosition.x;
            Vert.Position.y = NewPosition.y + InterpolatedPosition.y;
            Vert.Position.z = NewPosition.z + InterpolatedPosition.z;
        });
        mAllMeshes->Vertices.insert(mAllMeshes->Vertices.end(),mQuatPosArrowMesh.Vertices.begin(),mQuatPosArrowMesh.Vertices.end());
    }

    /* draw spline */
    mSplineMesh.Vertices.clear();
    if (mRenderData.rdDrawSplineLines)
    {
        mSplineMesh = mSplineModel->CreateVertexData(25,
          mRenderData.rdSplineStartVertex, mRenderData.rdSplineStartTangent,
          mRenderData.rdSplineEndVertex, mRenderData.rdSplineEndTangent);

        mAllMeshes->Vertices.insert(mAllMeshes->Vertices.end(),mSplineMesh.Vertices.begin(), mSplineMesh.Vertices.end());
    }

#if 0
    /* draw the model itself */
    *mModelMesh = mModel->GetVertexData();
    mRenderData.rdTriangleCount = mModelMesh ->Vertices.size() / 3;
    std::for_each(mModelMesh ->Vertices.begin(), mModelMesh ->Vertices.end(),[=](OGLVertex& Vert)
    {
        glm::quat Position = glm::quat(0.0f, Vert.Position.x, Vert.Position.y, Vert.Position.z);
            glm::quat NewPosition = mQuatMix * Position * mQuatMixConjugate;
            Vert.Position.x = NewPosition.x + InterpolatedPosition.x;
            Vert.Position.y = NewPosition.y + InterpolatedPosition.y;
            Vert.Position.z = NewPosition.z + InterpolatedPosition.z;
    });
    mAllMeshes->Vertices.insert(mAllMeshes->Vertices.end(),mModelMesh ->Vertices.begin(), mModelMesh ->Vertices.end());
#endif

    /* upload vertex data */
    mUploadToVBOTimer->Start();

    /* upload lines and boxes */
    mVertexBuffer->UploadData(*mAllMeshes);

    /* upload required data only when switching GPU and CPU */
    static bool LastGPURenderState = mRenderData.rdGPUVertexSkinning;

    if (LastGPURenderState != mRenderData.rdGPUVertexSkinning)
    {
        mModelUploadRequired = true;
        LastGPURenderState = mRenderData.rdGPUVertexSkinning;
    }
    if (mModelUploadRequired)
    {
        mGltfModel->UploadVertexBuffers();
        mModelUploadRequired = false;
    }

    if (!mRenderData.rdGPUVertexSkinning)
    {
        /* glTF vertex skinning, overwrites position buffer, needs upload on every frame */
        mGltfModel->ApplyCPUVertexSkinning(mRenderData.rdDualQuatVertexSkinning);
    }

    mRenderData.rdUploadToVBOTime = mUploadToVBOTimer->Stop();

    mLineIndexCount = mStartPosArrowMesh.Vertices.size() + mEndPosArrowMesh.Vertices.size() +
    mQuatPosArrowMesh.Vertices.size() + mCoordArrowsMesh.Vertices.size() +
    mSplineMesh.Vertices.size();

    /* draw the lines first */
    if (mLineIndexCount > 0)
    {
        mLineShader->Use();
        mVertexBuffer->BindAndDraw(GL_LINES, 0, mLineIndexCount);
    }

    /* draw the glTF model */
    if (mRenderData.rdDrawGltfModel)
    {
        if (mRenderData.rdGPUVertexSkinning)
        {
            if (mRenderData.rdDualQuatVertexSkinning)
            {
                mGltfGPUDualQuatShader->Use();
            }
            else
            {
                mGltfGPUShader->Use();
            }
        }
        else
        {
            mGltfShader->Use();
        }
        mGltfModel->Draw();
    }

    mFramebuffer->Unbind();

    /* blit color buffer to screen */
    mFramebuffer->DrawToScreen();

    mUIGenerateTimer->Start();
    mUserInterface->CreateFrame(mRenderData);
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