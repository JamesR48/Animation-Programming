#ifndef CPPANIMPROGRAMMING_GLTFANIMATIONCHANNEL_H
#define CPPANIMPROGRAMMING_GLTFANIMATIONCHANNEL_H

#include <vector>
#include <memory>
#include <algorithm>
#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace tinygltf
{
    class Model;
    struct Animation;
    struct AnimationChannel;
}

// node's (bone) property being changed
/* NOTE: there's a 4th path: weight. it's used for morph targets, a special
target type that adds displacements to the mesh...not used for now */
enum class ETargetPath {
    ROTATION,
    TRANSLATION,
    SCALE
  };

enum class EInterpolationType {
    STEP,
    LINEAR,
    CUBICSPLINE
  };

class GltfAnimationChannel
{
public:
    void LoadChannelData(const std::unique_ptr<tinygltf::Model>& Model, const tinygltf::Animation& Anim, const tinygltf::AnimationChannel& Channel);

    // returns node index
    int GetTargetNode() const;
    ETargetPath GetTargetPath() const;

    glm::vec3 GetScale(float Time);
    glm::vec3 GetTranslation(float Time);
    glm::quat GetRotation(float Time);

    /* returns max time of the channel's sampler INPUT time points.
      Useful  to find the correct end point of the animation clip. */
    float GetMaxTime() const;

private:
    // node index
    int mTargetNode = -1;
    ETargetPath mTargetPath = ETargetPath::ROTATION;
    EInterpolationType mInterType = EInterpolationType::LINEAR;

    std::vector<float> mTimings{};
    std::vector<glm::vec3> mScales{};
    std::vector<glm::vec3> mTranslations{};
    std::vector<glm::quat> mRotations{};

    void SetTimings(const std::vector<float>& Timings);
    void SetScales(const std::vector<glm::vec3>& Scales);
    void SetTranslations(const std::vector<glm::vec3>& Translations);
    void SetRotations(const std::vector<glm::quat>& Rotations);

    template<typename T>
    T GetInterpolatedValue(float Time, const std::vector<T>& DataElements, const T& DefaultValue)
    {
        if (DataElements.empty())
        {
            return DefaultValue;
        }

        if (Time < mTimings.at(0))
        {
            return DataElements.at(0);
        }
        if (Time > mTimings.at(mTimings.size() - 1))
        {
            return DataElements.at(DataElements.size() - 1);
        }

        // looking the 2 Time point indices right before and directly after the requested Time
        int NextTimeIndex = 0;
        int PrevTimeIndex = 0;
        // search the first timing element strictly GREATER than Time
        auto Iterator = std::upper_bound(mTimings.begin(), mTimings.end(), Time);
        // Time is at or past the very last keyframe
        if (Iterator == mTimings.end())
        {
            NextTimeIndex = mTimings.size() - 1;
        }
        else
        {
            // get the index of the found timing
            NextTimeIndex = std::distance(mTimings.begin(), Iterator);
        }

        PrevTimeIndex = NextTimeIndex - 1;

        // are they the same? Use any
        if (PrevTimeIndex == NextTimeIndex)
        {
            return DataElements.at(PrevTimeIndex);
        }

        T FinalValue = DefaultValue;
        switch(mInterType)
        {
            case EInterpolationType::STEP:
                // Step interp simply uses the prev value
                FinalValue = DataElements.at(PrevTimeIndex);
                break;
            case EInterpolationType::LINEAR:
            {
                /*
                Linear Interp:
                    interpolationValue =   ( currentTime − previousTime ) / ( nextTime − previousTime )
                    currentVec = previousVec + interpolationValue * ( nextVec − previousVec )
                */
                float InterpolatedTime = (Time - mTimings.at(PrevTimeIndex)) / (mTimings.at(NextTimeIndex) - mTimings.at(PrevTimeIndex));
                T PrevValue = DataElements.at(PrevTimeIndex);
                T NextValue = DataElements.at(NextTimeIndex);

                // compile-time check to use slerp for quats
                if constexpr (std::is_same_v<T, glm::quat>)
                {
                    FinalValue = glm::slerp(PrevValue, NextValue, InterpolatedTime);
                }
                else
                {
                    FinalValue = PrevValue + InterpolatedTime * (NextValue - PrevValue);
                }

                break;
            }
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
                    T PrevTangent = DeltaTime * DataElements.at(PrevTimeIndex * 3 + 2);
                    T NextTangent = DeltaTime * DataElements.at(NextTimeIndex * 3);

                    /*
                    a cubic spline needs the current interpolated time t as well as that ratio squared t^2 and cubed t^3
                    */
                    float InterpolatedTime = (Time - mTimings.at(PrevTimeIndex)) / (mTimings.at(NextTimeIndex) - mTimings.at(PrevTimeIndex));
                    float InterpolatedTimeSq = InterpolatedTime * InterpolatedTime;
                    float InterpolatedTimeCub = InterpolatedTimeSq * InterpolatedTime;

                    T PrevPoint = DataElements.at(PrevTimeIndex * 3 + 1);
                    T NextPoint = DataElements.at(NextTimeIndex * 3 + 1);

                    /*
                     Cubic Hermite Spline Interpolation (t is the interpolated time ratio [0.0, 1.0]:

                     Result = (h00(t) * P_prev) + (h10(t) * T_prev) + (h01(t) * P_next) + (h11(t) * T_next)

                     h00(t) =  2t³ - 3t² + 1     (Weight for the starting point)
                     h10(t) =   t³ -  2t² + t     (Weight for the starting out-tangent)
                     h01(t) = -2t³ + 3t²         (Weight for the ending point)
                     h11(t) =   t³ -   t²         (Weight for the ending in-tangent)
                     */
                    FinalValue =
                      (2 * InterpolatedTimeCub - 3 * InterpolatedTimeSq + 1) * PrevPoint +
                      (InterpolatedTimeCub - 2 * InterpolatedTimeSq + InterpolatedTime) * PrevTangent +
                      (-2 * InterpolatedTimeCub + 3 * InterpolatedTimeSq) * NextPoint +
                      (InterpolatedTimeCub - InterpolatedTimeSq) * NextTangent;

                    break;
                }
            }

            return FinalValue;
    };
};

#endif //CPPANIMPROGRAMMING_GLTFANIMATIONCHANNEL_H
