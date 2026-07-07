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
}

void UserInterface::CreateFrame(OGLRenderData& InOutRenderData)
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

    ImGui::Text("FPS:");
    ImGui::SameLine();
    ImGui::Text(std::to_string(mFramesPerSecond).c_str());

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
        ImGui::Text("Frame Time:");
        ImGui::SameLine();
        ImGui::Text("%s", std::to_string(InOutRenderData.rdFrameTime).c_str());
        ImGui::SameLine();
        ImGui::Text("ms");

        ImGui::Text("Model Upload Time:");
        ImGui::SameLine();
        ImGui::Text("%s", std::to_string(InOutRenderData.rdUploadToVBOTime).c_str());
        ImGui::SameLine();
        ImGui::Text("ms");

        ImGui::Text("Matrix Generation Time:");
        ImGui::SameLine();
        ImGui::Text("%s", std::to_string(InOutRenderData.rdMatrixGenerateTime).c_str());
        ImGui::SameLine();
        ImGui::Text("ms");

        ImGui::Text("Matrix Upload Time:");
        ImGui::SameLine();
        ImGui::Text("%s", std::to_string(InOutRenderData.rdUploadToUBOTime).c_str());
        ImGui::SameLine();
        ImGui::Text("ms");

        ImGui::Text("UI Generation Time:");
        ImGui::SameLine();
        ImGui::Text("%s", std::to_string(InOutRenderData.rdUIGenerateTime).c_str());
        ImGui::SameLine();
        ImGui::Text("ms");

        ImGui::Text("UI Draw Time:");
        ImGui::SameLine();
        ImGui::Text("%s", std::to_string(InOutRenderData.rdUIDrawTime).c_str());
        ImGui::SameLine();
        ImGui::Text("ms");
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

    if (ImGui::CollapsingHeader("SLERP + Spline"))
    {
        if (ImGui::Button("Reset All"))
        {
            InOutRenderData.rdResetAnglesAndInterp = true;
        }

        ImGui::Text("Interpolate");
        ImGui::SameLine();
        ImGui::SliderFloat("##Interp", &InOutRenderData.rdInterpValue, 0.0f, 1.0f, "%.3f", Flags);

        if (ImGui::CollapsingHeader("SLERP"))
        {
            ImGui::Checkbox("Draw World Coordinate Arrows", &InOutRenderData.rdDrawWorldCoordArrows);
            ImGui::Checkbox("Draw Model Coordinate Arrows", &InOutRenderData.rdDrawModelCoordArrows);

            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
            ImGui::Text("X Rotation ");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::SliderInt2("##ROTX", InOutRenderData.rdRotXAngle.data(), 0, 360, "%d", Flags);

            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
            ImGui::Text("Y Rotation ");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::SliderInt2("##ROTY", InOutRenderData.rdRotYAngle.data(), 0, 360, "%d", Flags);

            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 255, 255));
            ImGui::Text("Z Rotation ");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::SliderInt2("##ROTZ", InOutRenderData.rdRotZAngle.data(), 0, 360, "%d", Flags);
        }

        if (ImGui::CollapsingHeader("Spline"))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
            ImGui::Text("Start Vec  ");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::SliderFloat3("##STARTVEC", glm::value_ptr(InOutRenderData.rdSplineStartVertex), -10.0f, 10.0f, "%.3f", Flags);

            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
            ImGui::Text("Start Tang ");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::SliderFloat3("##STARTTANG", glm::value_ptr(InOutRenderData.rdSplineStartTangent), -10.0f, 10.0f, "%.3f", Flags);

            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
            ImGui::Text("End Vec    ");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::SliderFloat3("##ENDVEC", glm::value_ptr(InOutRenderData.rdSplineEndVertex), -10.0f, 10.0f, "%.3f", Flags);

            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
            ImGui::Text("End Tang   ");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::SliderFloat3("##ENDTANG", glm::value_ptr(InOutRenderData.rdSplineEndTangent), -10.0f, 10.0f, "%.3f", Flags);
        }
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

    if (ImGui::CollapsingHeader("glTF Model"))
    {
        ImGui::Checkbox("Draw Model", &InOutRenderData.rdDrawGltfModel);
        ImGui::Checkbox("Draw Skeleton", &InOutRenderData.rdDrawSkeleton);
        ImGui::Checkbox("Vertex Skinning Method:", &InOutRenderData.rdGPUVertexSkinning);
        ImGui::SameLine();
        if (InOutRenderData.rdGPUVertexSkinning)
        {
            ImGui::Text("GPU");
        }
        else
        {
            ImGui::Text("Code");
        }
        ImGui::Indent();
        ImGui::Checkbox("GPU Vertex Skinning Method:", &InOutRenderData.rdDualQuatVertexSkinning);
        ImGui::SameLine();
        if (InOutRenderData.rdDualQuatVertexSkinning)
        {
            ImGui::Text("Dual Quaternion");
        }
        else
        {
            ImGui::Text("Linear");
        }
    }

    if (ImGui::CollapsingHeader("glTF Animation")) {
        ImGui::Text("Clip No");
        ImGui::SameLine();
        ImGui::SliderInt("##Clip", &InOutRenderData.rdAnimClip, 0, InOutRenderData.rdAnimClipSize - 1, "%d", Flags);

        ImGui::Text("Clip Name: %s", InOutRenderData.rdClipName.c_str());

        ImGui::Checkbox("Play Animation", &InOutRenderData.rdPlayAnimation);

        ImGui::Checkbox("Animation Direction:", &InOutRenderData.rdPlayAnimationBackward);
        ImGui::SameLine();
        if (!InOutRenderData.rdPlayAnimationBackward)
        {
            ImGui::Text("Forward");
        }
        else
        {
            ImGui::Text("Backward");
        }

        if (!InOutRenderData.rdPlayAnimation)
        {
            ImGui::BeginDisabled();
        }
        ImGui::Text("Speed  ");
        ImGui::SameLine();
        ImGui::SliderFloat("##ClipSpeed", &InOutRenderData.rdAnimSpeed, 0.0f, 2.0f, "%.3f", Flags);
        if (!InOutRenderData.rdPlayAnimation)
        {
            ImGui::EndDisabled();
        }

        if (InOutRenderData.rdPlayAnimation)
        {
            ImGui::BeginDisabled();
        }
        ImGui::Text("Timepos");
        ImGui::SameLine();
        ImGui::SliderFloat("##ClipPos", &InOutRenderData.rdAnimTimePosition, 0.0f, InOutRenderData.rdAnimEndTime, "%.3f", Flags);

        if (InOutRenderData.rdPlayAnimation)
        {
            ImGui::EndDisabled();
        }
    }

    if (ImGui::CollapsingHeader("glTF Animation Blending"))
    {
        ImGui::Checkbox("Blending Type:", &InOutRenderData.rdCrossBlending);
        ImGui::SameLine();
        if (InOutRenderData.rdCrossBlending)
        {
            ImGui::Text("Cross");
        }
        else
        {
            ImGui::Text("Single");
        }

        if (InOutRenderData.rdCrossBlending)
        {
            ImGui::BeginDisabled();
        }

        ImGui::Text("Blend Factor");
        ImGui::SameLine();
        ImGui::SliderFloat("##BlendFactor", &InOutRenderData.rdAnimBlendFactor, 0.0f, 1.0f, "%.3f", Flags);

        if (InOutRenderData.rdCrossBlending)
        {
            ImGui::EndDisabled();
        }

        if (!InOutRenderData.rdCrossBlending)
        {
            ImGui::BeginDisabled();
        }

        ImGui::Text("Dest Clip   ");
        ImGui::SameLine();
        ImGui::SliderInt("##DestClip", &InOutRenderData.rdCrossBlendDestAnimClip, 0, InOutRenderData.rdAnimClipSize - 1, "%d", Flags);

        ImGui::Text("Dest Clip Name: %s", InOutRenderData.rdCrossBlendDestClipName.c_str());

        ImGui::Text("Cross Blend ");
        ImGui::SameLine();
        ImGui::SliderFloat("##CrossBlendFactor", &InOutRenderData.rdAnimCrossBlendFactor, 0.0f, 1.0f, "%.3f", Flags);

        if (!InOutRenderData.rdCrossBlending)
        {
            ImGui::EndDisabled();
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
