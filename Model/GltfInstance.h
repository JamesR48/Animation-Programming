#ifndef CPPANIMPROGRAMMING_GLTFINSTANCE_H
#define CPPANIMPROGRAMMING_GLTFINSTANCE_H

#include <bitset>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

#include "ModelSettings.h"

struct OGLMesh;
struct ModelSettings;
class GltfModel;
class GltfNode;
class GltfAnimationClip;
class IKSolver;

class GltfInstance
{
public:

    GltfInstance(std::shared_ptr<GltfModel> Model, glm::vec3 WorldPos, bool bRandomize = false);
    ~GltfInstance();

    void ResetNodeData();
    void SetSkeletonSplitNode(const int NodeIndex);
    std::shared_ptr<OGLMesh> GetSkeleton();
    int GetJointMatrixSize() const;
    void GetJointMatrices(std::vector<glm::mat4>& OutJointMatrices);
    int GetJointDualQuatsSize() const;
    void GetJointDualQuats(std::vector<glm::mat2x4>& OutJointDualQuats);

    void SetInstanceSettings(const ModelSettings& InSettings);
    void GetInstanceSettings(ModelSettings& OutModelSettings);

    void UpdateAnimation();
    void SolveIK();

    /* updating main properties once at the end of the frame:
     world position/rotation, blending mode, split node */
    void CheckForUpdates();

    void GetWorldPosition(glm::vec3& OutWorldPos);
    void GetWorldRotation(glm::quat& OutWorldQuat);

private:

    // Passing a DestAnimIndex > -1 will apply cross-blending between both animations instead of the binding pose
    void PlayAnimation(const int SourceAnimIndex, const float BlendFactor, const int DestAnimIndex = -1, const float PlaybackSpeed = 1.0f, const EPlaybackDirection PlaybackDirection = EPlaybackDirection::Forward);
    void BlendAnimationFrame(const int SourceAnimIndex, const float Time, const float BlendFactor, const int DestAnimIndex = -1);
    float GetAnimationEndTime(const int AnimIndex) const;

    void GetSkeletonPerNode(std::shared_ptr<GltfNode> TreeNode);
    void UpdateNodeMatrices(std::shared_ptr<GltfNode>& TreeNode);
    void UpdateJointMatrices(std::shared_ptr<GltfNode>& TreeNode);
    void UpdateJointDualQuats(std::shared_ptr<GltfNode>& TreeNode);
    void UpdateAdditiveMask(const std::shared_ptr<GltfNode>& TreeNode, const int SplitNodeIndex);

    void SetInverseKinematicsNodes(int EffectorNodeIndex, int IKChainRootNodeIndex);
    void SetNumIKIterations(int Iterations);

    std::shared_ptr<GltfNode> mRootNode = nullptr;
    std::vector<std::shared_ptr<GltfNode>> mNodeList;

    std::vector<glm::mat4> mJointMatrices{};
    //  GLSL shaders don’t support quaternions or dual quaternions, must use a 2x4 matrix to transport the data
    std::vector<glm::mat2x4> mJointDualQuats{};
    // To calculate the binding pose of the model
    std::vector<glm::mat4> mInverseBindMatrices{};

    // look up table of joints per vertex indices
    std::vector<int> mNodeToJoint{};

    std::bitset<MAX_GLTF_NODES> mAdditiveAnimationMask{0};

    std::unique_ptr<IKSolver> mIKSolver = nullptr;
    std::shared_ptr<OGLMesh> mSkeletonMesh = nullptr;
    std::shared_ptr<GltfModel> mGltfModel = nullptr;

    std::vector<std::shared_ptr<GltfAnimationClip>> mAnimClips{};

    unsigned int mNodeCount = 0;
    ModelSettings mModelSettings{};
};

#endif //CPPANIMPROGRAMMING_GLTFINSTANCE_H
