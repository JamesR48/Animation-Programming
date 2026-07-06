#ifndef CPPANIMPROGRAMMING_GLTFANIMATIONCLIP_H
#define CPPANIMPROGRAMMING_GLTFANIMATIONCLIP_H

#include <memory>
#include <string>
#include <vector>

namespace tinygltf
{
    class Model;
    struct Animation;
    struct AnimationChannel;
}

class GltfNode;
class GltfAnimationChannel;

class GltfAnimationClip
{
public:
    GltfAnimationClip(const std::string& Name);

    void AddChannel(const std::unique_ptr<tinygltf::Model>& Model, const tinygltf::Animation& Anim, const tinygltf::AnimationChannel& Channel);

    // updates the model to a specified point in time with the data of the current channel
    void SetAnimationFrame(std::vector<std::shared_ptr<GltfNode>>& Nodes, float Time);
    float GetClipEndTime();
    std::string GetClipName();

private:
    std::vector<std::shared_ptr<GltfAnimationChannel>> mAnimationChannels{};

    std::string mClipName;
};

#endif //CPPANIMPROGRAMMING_GLTFANIMATIONCLIP_H
