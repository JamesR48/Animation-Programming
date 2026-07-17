#include <string>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include "UserInterface.h"

#include <glm/gtc/type_ptr.hpp>

#include "../../Tools/Logger.h"

void UserInterface::Init(const OGLRenderData& RenderData)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(RenderData.rdWindow, true);
    const char *glslVersion = "#version 460 core";
    ImGui_ImplOpenGL3_Init(glslVersion);
    ImGui::StyleColorsDark();
    Logger::Log(1, "%s: user interface initialized\n", __FUNCTION__);

    /* init plot vectors */
    mFPSValues.resize(mNumFPSValues);
    mFrameTimeValues.resize(mNumFrameTimeValues);
    mModelUploadValues.resize(mNumModelUploadValues);
    mMatrixGenerationValues.resize(mNumMatrixGenerationValues);
    mMatrixUploadValues.resize(mNumMatrixUploadValues);
    mUiGenValues.resize(mNumUiGenValues);
    mUiDrawValues.resize(mNumUiDrawValues);
    mIKValues.resize(mNumIKValues);
}

void UserInterface::CreateFrame(OGLRenderData& InOutRenderData, ModelSettings& InOutModelSettings)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    /* creating overlay window */

    ImGuiWindowFlags imguiWindowFlags = 0;
    //imguiWindowFlags |= ImGuiWindowFlags_NoCollapse;
    //imguiWindowFlags |= ImGuiWindowFlags_NoResize;
    //imguiWindowFlags |= ImGuiWindowFlags_NoMove;

    ImGui::SetNextWindowBgAlpha(0.8f);

    ImGui::Begin("Control", nullptr, imguiWindowFlags);

    static float NewFps = 0.0f;
    if (InOutRenderData.rdFrameTime > 0.0)
    {
        NewFps = (1.0f / InOutRenderData.rdFrameTime) * 1000.0f;
    }
    mFramesPerSecond = (mAveragingAlpha * mFramesPerSecond) + (1.0f - mAveragingAlpha) * NewFps;

    /* clamp manual input on all sliders to min/max */
    ImGuiSliderFlags Flags = ImGuiSliderFlags_ClampOnInput;

    //  hold a timestamp of the last update of the plot data
    static double UpdateTime = 0.0;

    /// initializing for the first run
    if (UpdateTime < 0.000001)
    {
        UpdateTime = ImGui::GetTime();
    }

    // initializing ring buffer offsets
    static int FPSOffset = 0;
    static int FrameTimeOffset = 0;
    static int ModelUploadOffset = 0;
    static int MatrixGenOffset = 0;
    static int MatrixUploadOffset = 0;
    static int UIGenOffset = 0;
    static int UIDrawOffset = 0;
    static int IKOffset = 0;

    while (UpdateTime < ImGui::GetTime())
    {
        /* ring buffer */
        // using % to wrap the buffer back to 0 once the configured number of values is exceed
        mFPSValues.at(FPSOffset) = mFramesPerSecond;
        FPSOffset = ++FPSOffset % mNumFPSValues;

        mFrameTimeValues.at(FrameTimeOffset) = InOutRenderData.rdFrameTime;
        FrameTimeOffset = ++FrameTimeOffset % mNumFrameTimeValues;

        mModelUploadValues.at(ModelUploadOffset) = InOutRenderData.rdUploadToVBOTime;
        ModelUploadOffset = ++ModelUploadOffset % mNumModelUploadValues;

        mMatrixGenerationValues.at(MatrixGenOffset) = InOutRenderData.rdMatrixGenerateTime;
        MatrixGenOffset = ++MatrixGenOffset % mNumMatrixGenerationValues;

        mMatrixUploadValues.at(MatrixUploadOffset) = InOutRenderData.rdUploadToUBOTime;
        MatrixUploadOffset = ++MatrixUploadOffset % mNumMatrixUploadValues;

        mUiGenValues.at(UIGenOffset) = InOutRenderData.rdUIGenerateTime;
        UIGenOffset = ++UIGenOffset % mNumUiGenValues;

        mUiDrawValues.at(UIDrawOffset) = InOutRenderData.rdUIDrawTime;
        UIDrawOffset = ++UIDrawOffset % mNumUiDrawValues;

        mIKValues.at(IKOffset) = InOutRenderData.rdIKTime;
        IKOffset = ++IKOffset % mNumIKValues;

        // advancing plot data update ~33ms into the future
        UpdateTime += 1.0 / 30.0;
    }

    ImGui::BeginGroup();
    ImGui::Text("FPS:");
    ImGui::SameLine();
    ImGui::Text("%s", std::to_string(mFramesPerSecond).c_str());
    ImGui::EndGroup();

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        float AverageFPS = 0.0f;
        for (const auto value : mFPSValues)
        {
            AverageFPS += value;
        }
        AverageFPS /= static_cast<float>(mNumFPSValues);
        std::string fpsOverlay = "now:     " + std::to_string(mFramesPerSecond) + "\n30s avg: " + std::to_string(AverageFPS);
        ImGui::Text("FPS");
        ImGui::SameLine();
        ImGui::PlotLines("##FrameTimes", mFPSValues.data(), mFPSValues.size(), FPSOffset,
            fpsOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
        ImGui::EndTooltip();
    }

    if (ImGui::CollapsingHeader("Info"))
    {
        ImGui::Text("Triangles:");
        ImGui::SameLine();
        ImGui::Text("%s", std::to_string(InOutRenderData.rdTriangleCount + InOutRenderData.rdGltfTriangleCount).c_str());

        std::string windowDims = std::to_string(InOutRenderData.rdWidth) + "x" + std::to_string(InOutRenderData.rdHeight);
        ImGui::Text("Window Dimensions:");
        ImGui::SameLine();
        ImGui::Text("%s", windowDims.c_str());

        std::string imgWindowPos = std::to_string(static_cast<int>(ImGui::GetWindowPos().x)) + "/" + std::to_string(static_cast<int>(ImGui::GetWindowPos().y));
        ImGui::Text("ImGui Window Position:");
        ImGui::SameLine();
        ImGui::Text("%s", imgWindowPos.c_str());
    }

    if (ImGui::CollapsingHeader("Timers"))
    {
        ImGui::BeginGroup();
        ImGui::Text("Frame Time:");
        ImGui::SameLine();
        ImGui::Text("%s", std::to_string(InOutRenderData.rdFrameTime).c_str());
        ImGui::SameLine();
        ImGui::Text("ms");
        ImGui::EndGroup();

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            float averageFrameTime = 0.0f;
            for (const auto value: mFrameTimeValues)
            {
                averageFrameTime += value;
            }
            averageFrameTime /= static_cast<float>(mNumMatrixGenerationValues);
            std::string frameTimeOverlay = "now:     " + std::to_string(InOutRenderData.rdFrameTime)
                                           + " ms\n30s avg: " + std::to_string(averageFrameTime) + " ms";
            ImGui::Text("Frame Time       ");
            ImGui::SameLine();
            ImGui::PlotLines("##FrameTime", mFrameTimeValues.data(), mFrameTimeValues.size(), FrameTimeOffset,
                             frameTimeOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
            ImGui::EndTooltip();
        }

        ImGui::BeginGroup();
        ImGui::Text("Model Upload Time:");
        ImGui::SameLine();
        ImGui::Text("%s", std::to_string(InOutRenderData.rdUploadToVBOTime).c_str());
        ImGui::SameLine();
        ImGui::Text("ms");
        ImGui::EndGroup();

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            float averageModelUpload = 0.0f;
            for (const auto value: mModelUploadValues)
            {
                averageModelUpload += value;
            }
            averageModelUpload /= static_cast<float>(mNumModelUploadValues);
            std::string modelUploadOverlay = "now:     " + std::to_string(InOutRenderData.rdUploadToVBOTime)
                                             + " ms\n30s avg: " + std::to_string(averageModelUpload) + " ms";
            ImGui::Text("VBO Upload");
            ImGui::SameLine();
            ImGui::PlotLines("##ModelUploadTimes", mModelUploadValues.data(), mModelUploadValues.size(),
                             ModelUploadOffset,
                             modelUploadOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
            ImGui::EndTooltip();
        }

        ImGui::BeginGroup();
        ImGui::Text("Matrix Generation Time:");
        ImGui::SameLine();
        ImGui::Text("%s", std::to_string(InOutRenderData.rdMatrixGenerateTime).c_str());
        ImGui::SameLine();
        ImGui::Text("ms");
        ImGui::EndGroup();

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            float averageMatGen = 0.0f;
            for (const auto value: mMatrixGenerationValues)
            {
                averageMatGen += value;
            }
            averageMatGen /= static_cast<float>(mNumMatrixGenerationValues);
            std::string matrixGenOverlay = "now:     " + std::to_string(InOutRenderData.rdMatrixGenerateTime)
                                           + " ms\n30s avg: " + std::to_string(averageMatGen) + " ms";
            ImGui::Text("Matrix Generation");
            ImGui::SameLine();
            ImGui::PlotLines("##MatrixGenTimes", mMatrixGenerationValues.data(), mMatrixGenerationValues.size(),
                             MatrixGenOffset,
                             matrixGenOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
            ImGui::EndTooltip();
        }

        ImGui::BeginGroup();
        ImGui::Text("Matrix Upload Time:");
        ImGui::SameLine();
        ImGui::Text("%s", std::to_string(InOutRenderData.rdUploadToUBOTime).c_str());
        ImGui::SameLine();
        ImGui::Text("ms");
        ImGui::EndGroup();

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            float averageMatrixUpload = 0.0f;
            for (const auto value: mMatrixUploadValues)
            {
                averageMatrixUpload += value;
            }
            averageMatrixUpload /= static_cast<float>(mNumMatrixUploadValues);
            std::string matrixUploadOverlay = "now:     " + std::to_string(InOutRenderData.rdUploadToVBOTime)
                                              + " ms\n30s avg: " + std::to_string(averageMatrixUpload) + " ms";
            ImGui::Text("UBO Upload");
            ImGui::SameLine();
            ImGui::PlotLines("##MatrixUploadTimes", mMatrixUploadValues.data(), mMatrixUploadValues.size(),
                             MatrixUploadOffset,
                             matrixUploadOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
            ImGui::EndTooltip();
        }

        ImGui::BeginGroup();
        ImGui::Text("UI Generation Time:");
        ImGui::SameLine();
        ImGui::Text("%s", std::to_string(InOutRenderData.rdUIGenerateTime).c_str());
        ImGui::SameLine();
        ImGui::Text("ms");
        ImGui::EndGroup();

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            float averageUiGen = 0.0f;
            for (const auto value: mUiGenValues)
            {
                averageUiGen += value;
            }
            averageUiGen /= static_cast<float>(mNumUiGenValues);
            std::string uiGenOverlay = "now:     " + std::to_string(InOutRenderData.rdUIGenerateTime)
                                       + " ms\n30s avg: " + std::to_string(averageUiGen) + " ms";
            ImGui::Text("UI Generation");
            ImGui::SameLine();
            ImGui::PlotLines("##UIGenTimes", mUiGenValues.data(), mUiGenValues.size(), UIGenOffset,
                             uiGenOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
            ImGui::EndTooltip();
        }

        ImGui::BeginGroup();
        ImGui::Text("UI Draw Time:");
        ImGui::SameLine();
        ImGui::Text("%s", std::to_string(InOutRenderData.rdUIDrawTime).c_str());
        ImGui::SameLine();
        ImGui::Text("ms");
        ImGui::EndGroup();

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            float averageUiDraw = 0.0f;
            for (const auto value: mUiDrawValues)
            {
                averageUiDraw += value;
            }
            averageUiDraw /= static_cast<float>(mNumUiDrawValues);
            std::string uiDrawOverlay = "now:     " + std::to_string(InOutRenderData.rdUIDrawTime)
                                        + " ms\n30s avg: " + std::to_string(averageUiDraw) + " ms";
            ImGui::Text("UI Draw");
            ImGui::SameLine();
            ImGui::PlotLines("##UIDrawTimes", mUiDrawValues.data(), mUiDrawValues.size(), UIDrawOffset,
                             uiDrawOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
            ImGui::EndTooltip();
        }

        ImGui::BeginGroup();
        ImGui::Text("(IK Generation Time)  :");
        ImGui::SameLine();
        ImGui::Text("%s", std::to_string(InOutRenderData.rdIKTime).c_str());
        ImGui::SameLine();
        ImGui::Text("ms");
        ImGui::EndGroup();

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            float averageIKTime = 0.0f;
            for (const auto value : mIKValues)
            {
                averageIKTime += value;
            }
            averageIKTime /= static_cast<float>(mNumIKValues);
            std::string ikOverlay = "now:     " + std::to_string(InOutRenderData.rdIKTime)
              + " ms\n30s avg: " + std::to_string(averageIKTime) + " ms";
            ImGui::Text("(IK Generation)");
            ImGui::SameLine();
            ImGui::PlotLines("##IKTimes", mIKValues.data(), mIKValues.size(), IKOffset,
              ikOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
            ImGui::EndTooltip();
        }

    }

    if (ImGui::CollapsingHeader("Camera"))
    {
        ImGui::Text("Camera Position:");
        ImGui::SameLine();
        ImGui::Text("%s",glm::to_string(InOutRenderData.rdCameraWorldPosition).c_str());
        ImGui::Text("View Azimuth:");
        ImGui::SameLine();
        ImGui::Text("%s", std::to_string(InOutRenderData.rdViewAzimuth).c_str());
        ImGui::Text("View Elevation:");
        ImGui::SameLine();
        ImGui::Text("%s", std::to_string(InOutRenderData.rdViewElevation).c_str());
        ImGui::Text("Field of View");
        ImGui::SameLine();
        ImGui::SliderFloat("##FOV", &InOutRenderData.rdFieldOfView, 40, 150, "%.3f", Flags);
    }

    if (ImGui::CollapsingHeader("Lighting"))
    {
        ImGui::Text("LightPosition X");
        ImGui::SameLine();
        ImGui::SliderFloat("##LightPosX", &InOutRenderData.rdLightPostion.x, -360.0f, 360.0f, "%.3f", Flags);

        ImGui::Text("LightPosition Y");
        ImGui::SameLine();
        ImGui::SliderFloat("##LightPosY", &InOutRenderData.rdLightPostion.y, -360.0f, 360.0f, "%.3f", Flags);

        ImGui::Text("LightPosition Z");
        ImGui::SameLine();
        ImGui::SliderFloat("##LightPosZ", &InOutRenderData.rdLightPostion.z, -360.0f, 360.0f, "%.3f", Flags);
    }

    if (ImGui::CollapsingHeader("glTF Instances"))
    {
        ImGui::Text("Model Instances  : %d", InOutRenderData.rdNumberOfInstances);

        ImGui::Text("Selected Instance:");
        ImGui::SameLine();
        ImGui::PushButtonRepeat(true);
        if (ImGui::ArrowButton("##LEFT", ImGuiDir_Left) && InOutRenderData.rdCurrentSelectedInstance > 0)
        {
            InOutRenderData.rdCurrentSelectedInstance--;
        }
        ImGui::SameLine();
        ImGui::PushItemWidth(30);
        ImGui::DragInt("##SELINST", &InOutRenderData.rdCurrentSelectedInstance, 1, 0,
          InOutRenderData.rdNumberOfInstances - 1, "%3d", Flags);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::ArrowButton("##RIGHT", ImGuiDir_Right) && InOutRenderData.rdCurrentSelectedInstance < (InOutRenderData.rdNumberOfInstances - 1))
        {
            InOutRenderData.rdCurrentSelectedInstance++;
        }
        ImGui::PopButtonRepeat();

        ImGui::Text("World Pos  :");
        ImGui::SameLine();
        ImGui::SliderFloat3("##WORLDPOS", glm::value_ptr(InOutModelSettings.msWorldPosition),
          -25.0f, 25.0f, "%.1f", Flags);

        ImGui::Text("World Rotation   :");
        ImGui::SameLine();
        ImGui::SliderFloat("##WORLDROT", &InOutModelSettings.msWorldRotation.y,
          -180.0f, 180.0f, "%.0f", Flags);
    }

    if (ImGui::CollapsingHeader("glTF Model"))
    {
        ImGui::Checkbox("Draw Model", &InOutModelSettings.msDrawModel);
        ImGui::Checkbox("Draw Skeleton", &InOutModelSettings.msDrawSkeleton);

        ImGui::Text("Vertex Skinning:");
        ImGui::SameLine();
        if (ImGui::RadioButton("Linear",InOutModelSettings.msVertexSkinningMode == ESkinningMode::Linear))
        {
            InOutModelSettings.msVertexSkinningMode = ESkinningMode::Linear;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Dual Quaternion",InOutModelSettings.msVertexSkinningMode == ESkinningMode::DualQuat))
        {
            InOutModelSettings.msVertexSkinningMode = ESkinningMode::DualQuat;
        }
    }

    if (ImGui::CollapsingHeader("glTF Animation"))
    {
        ImGui::Checkbox("Play Animation", &InOutModelSettings.msPlayAnimation);

        if (!InOutModelSettings.msPlayAnimation)
        {
            ImGui::BeginDisabled();
        }

        ImGui::Text("Animation Direction:");
        ImGui::SameLine();
        if (ImGui::RadioButton("Forward", InOutModelSettings.msAnimationPlayDirection == EPlaybackDirection::Forward))
        {
            InOutModelSettings.msAnimationPlayDirection = EPlaybackDirection::Forward;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Backward",InOutModelSettings.msAnimationPlayDirection == EPlaybackDirection::Backward))
        {
            InOutModelSettings.msAnimationPlayDirection = EPlaybackDirection::Backward;
        }

        if (!InOutModelSettings.msPlayAnimation)
        {
            ImGui::EndDisabled();
        }

        ImGui::Text("Clip ");
        ImGui::SameLine();
        const std::string CurrentClip = InOutModelSettings.msClipNames.at(InOutModelSettings.msAnimClip);
        if (ImGui::BeginCombo("##ClipCombo",CurrentClip.c_str()))
        {
            const int ClipCount = InOutModelSettings.msClipNames.size();
            for (int i = 0; i < ClipCount; ++i)
            {
                const bool bIsSelected = (InOutModelSettings.msAnimClip == i);
                const std::string SelectedClip = InOutModelSettings.msClipNames.at(i);
                if (ImGui::Selectable(SelectedClip.c_str(), bIsSelected))
                {
                    InOutModelSettings.msAnimClip = i;
                }

                if (bIsSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (InOutModelSettings.msPlayAnimation)
        {
            ImGui::Text("Speed  ");
            ImGui::SameLine();
            ImGui::SliderFloat("##ClipSpeed", &InOutModelSettings.msAnimSpeed, 0.0f, 2.0f, "%.3f", Flags);
        }
        else
        {
            ImGui::Text("Timepos");
            ImGui::SameLine();
            ImGui::SliderFloat("##ClipPos", &InOutModelSettings.msAnimTimePosition, 0.0f, InOutModelSettings.msAnimEndTime, "%.3f", Flags);
        }
    }

    if (ImGui::CollapsingHeader("glTF Animation Blending"))
    {
        ImGui::Text("Blending Type:");
        ImGui::SameLine();
        if (ImGui::RadioButton("Crossfading",InOutModelSettings.msBlendingMode == EBlendMode::CrossFade))
        {
            InOutModelSettings.msBlendingMode = EBlendMode::CrossFade;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Additive",InOutModelSettings.msBlendingMode == EBlendMode::Additive))
        {
            InOutModelSettings.msBlendingMode = EBlendMode::Additive;
        }

        const bool bIsCrossBlending = InOutModelSettings.msBlendingMode != EBlendMode::FadeInOut;
        if (bIsCrossBlending)
        {
            ImGui::Text("Dest Clip   ");
            ImGui::SameLine();

            const std::string CurrentDestClip = InOutModelSettings.msClipNames.at(InOutModelSettings.msCrossBlendDestAnimClip);
            if (ImGui::BeginCombo("##DestClipCombo",CurrentDestClip.c_str()))
            {
                const int ClipCount = InOutModelSettings.msClipNames.size();
                for (int i = 0; i < ClipCount; ++i)
                {
                    const bool bIsSelected = (InOutModelSettings.msCrossBlendDestAnimClip == i);
                    const std::string SelectedClip = InOutModelSettings.msClipNames.at(i);
                    if (ImGui::Selectable(SelectedClip.c_str(), bIsSelected))
                    {
                        InOutModelSettings.msCrossBlendDestAnimClip = i;
                    }

                    if (bIsSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::Text("Cross Blend ");
            ImGui::SameLine();
            ImGui::SliderFloat("##CrossBlendFactor", &InOutModelSettings.msAnimCrossBlendFactor, 0.0f, 1.0f, "%.3f", Flags);
        }
        else
        {
            ImGui::Text("Blend Factor");
            ImGui::SameLine();
            ImGui::SliderFloat("##BlendFactor", &InOutModelSettings.msAnimBlendFactor, 0.0f, 1.0f, "%.3f", Flags);
        }

        if (InOutModelSettings.msBlendingMode == EBlendMode::Additive)
        {
            ImGui::Text("Split Node  ");
            ImGui::SameLine();

            const std::string CurrentSplitNode = InOutModelSettings.msSkelNodeNames.at(InOutModelSettings.msSkelSplitNode);
            if (ImGui::BeginCombo("##SplitNodeCombo",CurrentSplitNode.c_str()))
            {
                const int SplitNodeCount = InOutModelSettings.msSkelNodeNames.size();
                for (int i = 0; i < SplitNodeCount; ++i)
                {
                    if (InOutModelSettings.msSkelNodeNames.at(i).compare("(invalid)") != 0)
                    {
                        const bool bIsSelected = (InOutModelSettings.msSkelSplitNode == i);
                        if (ImGui::Selectable(InOutModelSettings.msSkelNodeNames.at(i).c_str(), bIsSelected))
                        {
                            InOutModelSettings.msSkelSplitNode = i;
                        }

                        if (bIsSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                }
                ImGui::EndCombo();
            }
        }
    }

    if (ImGui::CollapsingHeader("glTF Inverse Kinematic"))
    {
        ImGui::Text("Inverse Kinematics");
        ImGui::SameLine();
        if (ImGui::RadioButton("None", InOutModelSettings.msIKSolver == EIKSolver::None))
        {
            InOutModelSettings.msIKSolver = EIKSolver::None;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("CCD",InOutModelSettings.msIKSolver == EIKSolver::CCD))
        {
            InOutModelSettings.msIKSolver = EIKSolver::CCD;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("FABRIK",InOutModelSettings.msIKSolver == EIKSolver::FABRIK))
        {
            InOutModelSettings.msIKSolver = EIKSolver::FABRIK;
        }

        if (InOutModelSettings.msIKSolver != EIKSolver::None)
        {
            ImGui::Text("IK Iterations  :");
            ImGui::SameLine();
            ImGui::SliderInt("##IKITER", &InOutModelSettings.msIKIterations, 0, 15, "%d", Flags);
            ImGui::Text("Target Position:");
            ImGui::SameLine();
            ImGui::SliderFloat3("##IKTargetPOS", glm::value_ptr(InOutModelSettings.msIKTargetPos), -10.0f, 10.0f, "%.3f", Flags);
            ImGui::Text("Effector Node  :");
            ImGui::SameLine();
            if (ImGui::BeginCombo("##EffectorNodeCombo",
            InOutModelSettings.msSkelNodeNames.at(InOutModelSettings.msIKEffectorNode).c_str()))
            {
                for (int i = 0; i < InOutModelSettings.msSkelNodeNames.size(); ++i)
                {
                    const bool isSelected = (InOutModelSettings.msIKEffectorNode == i);
                    if (ImGui::Selectable(InOutModelSettings.msSkelNodeNames.at(i).c_str(), isSelected))
                    {
                        InOutModelSettings.msIKEffectorNode = i;
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::Text("IK Root Node   :");
            ImGui::SameLine();
            if (ImGui::BeginCombo("##RootNodeCombo",
            InOutModelSettings.msSkelNodeNames.at(InOutModelSettings.msIKRootNode).c_str()))
            {
                for (int i = 0; i < InOutModelSettings.msSkelNodeNames.size(); ++i)
                {
                    const bool isSelected = (InOutModelSettings.msIKRootNode == i);
                    if (ImGui::Selectable(InOutModelSettings.msSkelNodeNames.at(i).c_str(), isSelected))
                    {
                        InOutModelSettings.msIKRootNode = i;
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        }
    }

    ImGui::End();
}

void UserInterface::Render()
{
    // drawing the windows and widgets internally into an ImGui buffer.
    ImGui::Render();
    // copying the data of the internal framebuffer to the currently active framebuffer in the application
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UserInterface::Cleanup()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
