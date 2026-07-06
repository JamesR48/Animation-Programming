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

// TODO: TURN THIS INTO A TEMPLATE FOR VEC3 (SCALE, TRANSLATION) AND QUAT (ROTATION)
glm::vec3 GltfAnimationChannel::GetScale(float Time)
{
    return GetInterpolatedValue<glm::vec3>(Time, mScales, glm::vec3(1.0));
#if 0
    if (mScales.size() == 0)
    {
        return glm::vec3(1.0f);
    }

    if (Time < mTimings.at(0))
    {
        return mScales.at(0);
    }
    if (Time > mTimings.at(mTimings.size() - 1))
    {
        return mScales.at(mScales.size() - 1);
    }

    // looking the 2 Time point indices right before and directly after the requested Time
    int PrevTimeIndex = 0;
    int NextTimeIndex = 0;
    for (int i = 0; i < mTimings.size(); ++i)
    {
        if (mTimings.at(i) > Time)
        {
            NextTimeIndex = i;
            break;
        }
        PrevTimeIndex = i;
    }

    // are they the same? Use any
    if (PrevTimeIndex == NextTimeIndex)
    {
        return mScales.at(PrevTimeIndex);
    }

    glm::vec3 FinalScale = glm::vec3(1.0f);
    switch(mInterType)
    {
        case EInterpolationType::STEP:
            // Step interp simply uses the prev value
            FinalScale = mScales.at(PrevTimeIndex);
            break;
        case EInterpolationType::LINEAR:
        {
            /*
             Linear Interp:
                interpolationValue =   ( currentTime − previousTime ) / ( nextTime − previousTime )
                currentVec = previousVec + interpolationValue * ( nextVec − previousVec )
             */
            float InterpolatedTime = (Time - mTimings.at(PrevTimeIndex)) / (mTimings.at(NextTimeIndex) - mTimings.at(PrevTimeIndex));
            glm::vec3 PrevScale = mScales.at(PrevTimeIndex);
            glm::vec3 NextScale = mScales.at(NextTimeIndex);
            FinalScale = PrevScale + InterpolatedTime * (NextScale - PrevScale);
        }
            break;
        case EInterpolationType::CUBICSPLINE:
        {
            /*
            tangents are stored as normalized vectors or normalized quaternions in glTF.
            To calculate the correct tangent for the spline, the normalized value must be scaled
            according to the time difference between the two time points
            */
            float DeltaTime = mTimings.at(NextTimeIndex) - mTimings.at(PrevTimeIndex);

            /*
            to calculate the correct index for the mScaling vector, we must multiply the two index variables,
            prevTimeIndex and nextTimeIndex, by 3 as each element of the mScaling vector contains three consecutive
            values for CUBICSPLINE interp – the in-tangent, the data value, and the out-tangent...
            PREVIOUS FRAME in-tanget (prevTimeIndex * 3) ------ NEXT FRAME in-tangent (NextTimeIndex * 3)
            PREVIOUS FRAME data value (PrevTimeIndex * 3 + 1) ----- NEXT FRAME data value (NextTimeIndex * 3 + 1)
            PREVIOUS FRAME out-tanget (PrevTimeIndex * 3 + 2) ----- NEXT FRAME out-tanget (NextTimeIndex * 3 + 2)

            When animating between 2 points in time (prev and next), the movement is dictated by leaving the
            prev-frame and arriving at the next-frame. So we get the prev-frame's out-tangent and next-frame's in-tangent
             */
            glm::vec3 PrevTangent = DeltaTime * mScales.at(PrevTimeIndex * 3 + 2);
            glm::vec3 NextTangent = DeltaTime * mScales.at(NextTimeIndex * 3);

            /*
            a cubic spline needs the current interpolated time t as well as that ratio squared t^2 and cubed t^3
            */
            float InterpolatedTime = (Time - mTimings.at(PrevTimeIndex)) / (mTimings.at(NextTimeIndex) - mTimings.at(PrevTimeIndex));
            float InterpolatedTimeSq = InterpolatedTime * InterpolatedTime;
            float InterpolatedTimeCub = InterpolatedTimeSq * InterpolatedTime;

            glm::vec3 PrevPoint = mScales.at(PrevTimeIndex * 3 + 1);
            glm::vec3 NextPoint = mScales.at(NextTimeIndex * 3 + 1);

            /*
             Cubic Hermite Spline Interpolation (t is the interpolated time ratio [0.0, 1.0]:

             Result = (h00(t) * P_prev) + (h10(t) * T_prev) + (h01(t) * P_next) + (h11(t) * T_next)

             h00(t) =  2t³ - 3t² + 1     (Weight for the starting point)
             h10(t) =   t³ -  2t² + t     (Weight for the starting out-tangent)
             h01(t) = -2t³ + 3t²         (Weight for the ending point)
             h11(t) =   t³ -   t²         (Weight for the ending in-tangent)
             */
            FinalScale =
              (2 * InterpolatedTimeCub - 3 * InterpolatedTimeSq + 1) * PrevPoint +
              (InterpolatedTimeCub - 2 * InterpolatedTimeSq + InterpolatedTime) * PrevTangent +
              (-2 * InterpolatedTimeCub + 3 * InterpolatedTimeSq) * NextPoint +
              (InterpolatedTimeCub - InterpolatedTimeSq) * NextTangent;
        }
            break;
    }

    return FinalScale;
#endif
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
