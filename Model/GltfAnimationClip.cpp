#include "GltfAnimationClip.h"
#include "GltfAnimationChannel.h"
#include "GltfNode.h"
#include "../Tools/Logger.h"

#include <tiny_gltf.h>


GltfAnimationClip::GltfAnimationClip(const std::string& Name) : mClipName(Name) {}

void GltfAnimationClip::AddChannel(const std::unique_ptr<tinygltf::Model>& Model, const tinygltf::Animation &Anim,
    const tinygltf::AnimationChannel &Channel)
{
    std::shared_ptr<GltfAnimationChannel> Chan = std::make_shared<GltfAnimationChannel>();
    Chan->LoadChannelData(Model, Anim, Channel);
    mAnimationChannels.push_back(Chan);

}

void GltfAnimationClip::SetAnimationFrame(std::vector<std::shared_ptr<GltfNode>> &Nodes, const float Time)
{
    if (Nodes.empty())
    {
        Logger::Log(1, "%s: no Nodes to update!\n", __FUNCTION__);
        return;
    }

    // setting the new translation, rotation, scale of the node
    for (const std::shared_ptr<GltfAnimationChannel>& Channel : mAnimationChannels)
    {
        const float ClampedTime = std::clamp(Time, 0.0f, Channel->GetMaxTime());
        const int TargetNode = Channel->GetTargetNode();
        switch(Channel->GetTargetPath())
        {
            case ETargetPath::ROTATION:
                Nodes.at(TargetNode)->SetRotation(Channel->GetRotation(ClampedTime));
                break;
            case ETargetPath::TRANSLATION:
                Nodes.at(TargetNode)->SetTranslation(Channel->GetTranslation(ClampedTime));
                break;
            case ETargetPath::SCALE:
                Nodes.at(TargetNode)->SetScale(Channel->GetScale(ClampedTime));
                break;
        }
    }

    // TODO: optimize this! (maybe a dirty flag)
    /* updating all nodes in a single run */
    for (std::shared_ptr<GltfNode>& Node : Nodes)
    {
        if (Node)
        {
            Node->CalculateLocalTRSMatrix();
        }
    }
}

void GltfAnimationClip::BlendAnimationFrame(std::vector<std::shared_ptr<GltfNode>>& Nodes, const float Time, const float BlendFactor)
{
    if (Nodes.empty())
    {
        Logger::Log(1, "%s: no Nodes to update!\n", __FUNCTION__);
        return;
    }

    // setting the new translation, rotation, scale of the node
    for (const std::shared_ptr<GltfAnimationChannel>& Channel : mAnimationChannels)
    {
        const float ClampedTime = std::clamp(Time, 0.0f, Channel->GetMaxTime());
        const int TargetNode = Channel->GetTargetNode();
        switch(Channel->GetTargetPath())
        {
            case ETargetPath::ROTATION:
                Nodes.at(TargetNode)->BlendRotation(Channel->GetRotation(ClampedTime), BlendFactor);
                break;
            case ETargetPath::TRANSLATION:
                Nodes.at(TargetNode)->BlendTranslation(Channel->GetTranslation(ClampedTime), BlendFactor);
                break;
            case ETargetPath::SCALE:
                Nodes.at(TargetNode)->BlendScale(Channel->GetScale(ClampedTime), BlendFactor);
                break;
        }
    }

    // TODO: optimize this! (maybe a dirty flag)
    /* updating all nodes in a single run */
    for (std::shared_ptr<GltfNode>& Node : Nodes)
    {
        if (Node)
        {
            Node->CalculateLocalTRSMatrix();
        }
    }
}

float GltfAnimationClip::GetClipEndTime()
{
    if (mAnimationChannels.empty())
    {
        return 0.0f;
    }

    return mAnimationChannels.at(0)->GetMaxTime();
}

std::string GltfAnimationClip::GetClipName()
{
    return mClipName;
}
