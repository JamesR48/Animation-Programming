#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

#include "GltfAnimationChannel.h"
#include "../Tools/Logger.h"

void GltfAnimationChannel::LoadChannelData(const std::unique_ptr<tinygltf::Model>& Model, const tinygltf::Animation& Anim,
    const tinygltf::AnimationChannel& Channel)
{
    if (Model == nullptr)
    {
        Logger::Log(1, "%s: Invalid Model pointer\n", __FUNCTION__);
        return;
    }

    mTargetNode = Channel.target_node;

    const tinygltf::AnimationSampler Sampler = Anim.samplers.at(Channel.sampler);

    // getting timing values
    const int SamplerInputIndex = Sampler.input;
    const tinygltf::Accessor &InputAccessor = Model->accessors.at(SamplerInputIndex);
    const tinygltf::BufferView &InputBufferView = Model->bufferViews.at(InputAccessor.bufferView);
    const tinygltf::Buffer &InputBuffer = Model->buffers.at(InputBufferView.buffer);

    std::vector<float> Timings;
    Timings.resize(InputAccessor.count);
    std::memcpy(Timings.data(), &InputBuffer.data.at(0) + InputBufferView.byteOffset, InputBufferView.byteLength);
    SetTimings(Timings);

    // getting interp type
    if (Sampler.interpolation.compare("STEP") == 0)
    {
        mInterType = EInterpolationType::STEP;
    }
    else if (Sampler.interpolation.compare("LINEAR") == 0)
    {
        mInterType = EInterpolationType::LINEAR;
    }
    else
    {
        mInterType = EInterpolationType::CUBICSPLINE;
    }

    // getting altered transformation values
    const int SamplerOutputIndex = Sampler.output;
    const tinygltf::Accessor &OutputAccessor = Model->accessors.at(SamplerOutputIndex);
    const tinygltf::BufferView &OutputBufferView = Model->bufferViews.at(OutputAccessor.bufferView);
    const tinygltf::Buffer &OutputBuffer = Model->buffers.at(OutputBufferView.buffer);

    if (Channel.target_path.compare("rotation") == 0)
    {
        mTargetPath = ETargetPath::ROTATION;
        std::vector<glm::quat> Rotations;
        Rotations.resize(OutputAccessor.count);

        std::memcpy(Rotations.data(), &OutputBuffer.data.at(0) + OutputBufferView.byteOffset, OutputBufferView.byteLength);
        SetRotations(Rotations);
    }
    else if (Channel.target_path.compare("translation") == 0)
    {
        mTargetPath = ETargetPath::TRANSLATION;
        std::vector<glm::vec3> Translations;
        Translations.resize(OutputAccessor.count);

        std::memcpy(Translations.data(), &OutputBuffer.data.at(0) + OutputBufferView.byteOffset, OutputBufferView.byteLength);
        SetTranslations(Translations);
    }
    else
    {
        mTargetPath = ETargetPath::SCALE;
        std::vector<glm::vec3> Scale;
        Scale.resize(OutputAccessor.count);

        std::memcpy(Scale.data(), &OutputBuffer.data.at(0) + OutputBufferView.byteOffset, OutputBufferView.byteLength);
        SetScales(Scale);
    }
}

int GltfAnimationChannel::GetTargetNode() const
{
    return mTargetNode;
}

ETargetPath GltfAnimationChannel::GetTargetPath() const
{
    return mTargetPath;
}

glm::vec3 GltfAnimationChannel::GetScale(float Time)
{
    return GetInterpolatedValue<glm::vec3>(Time, mScales, glm::vec3(1.0));
}

glm::vec3 GltfAnimationChannel::GetTranslation(float Time)
{
    return GetInterpolatedValue<glm::vec3>(Time, mTranslations, glm::vec3(1.0));
}

glm::quat GltfAnimationChannel::GetRotation(float Time)
{
    return GetInterpolatedValue<glm::quat>(Time, mRotations, glm::identity<glm::quat>());
}

float GltfAnimationChannel::GetMaxTime() const
{
    if (mTimings.empty())
    {
       return 0.0f;
    }

    return mTimings.at(mTimings.size() - 1);
}

void GltfAnimationChannel::SetTimings(const std::vector<float>& Timings)
{
    mTimings = Timings;
}

void GltfAnimationChannel::SetScales(const std::vector<glm::vec3>& Scales)
{
    mScales = Scales;
}

void GltfAnimationChannel::SetTranslations(const std::vector<glm::vec3>& Translations)
{
    mTranslations = Translations;
}

void GltfAnimationChannel::SetRotations(const std::vector<glm::quat>& Rotations)
{
    mRotations = Rotations;
}
