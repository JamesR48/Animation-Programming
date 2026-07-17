#include "GltfInstance.h"

#include <chrono>

#include "GltfModel.h"
#include "GltfNode.h"
#include "GltfAnimationClip.h"
#include "IKSolver.h"
#include "../Tools/Logger.h"
#include <cstdlib> // rand
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/string_cast.hpp>

GltfInstance::GltfInstance(std::shared_ptr<GltfModel> Model, glm::vec3 WorldPos, bool bRandomize)
{
    if (Model == nullptr)
    {
        Logger::Log(1, "%s error: invalid glTF model\n", __FUNCTION__);
        return;
    }

    mGltfModel = Model;
    mModelSettings.msWorldPosition = WorldPos;
    std::string ModelName;
    mGltfModel->GetModelFilename(ModelName);

    Logger::Log(2, "%s: spawning model from glTF file '%s' on position %s\n", __FUNCTION__,
    ModelName.c_str(), glm::to_string(mModelSettings.msWorldPosition).c_str());

    mNodeCount = mGltfModel->GetNodeCount();
    mGltfModel->GetInverseBindMatrices(mInverseBindMatrices);
    mGltfModel->GetNodeToJoint(mNodeToJoint);

    mJointMatrices.resize(mInverseBindMatrices.size());
    mJointDualQuats.resize(mInverseBindMatrices.size());

    /* initializing with all nodes valid */
    mAdditiveAnimationMask.set();

    /* creating nodes tree */
    GltfNodeData NodeData;
    mGltfModel->GetGltfNodes(NodeData);
    mRootNode = NodeData.RootNode;
    mRootNode->SetWorldPosition(mModelSettings.msWorldPosition);

    mNodeList = NodeData.NodeList;

    /* reset skeleton split */
    mModelSettings.msSkelSplitNode = mNodeCount - 1;

    /* filling node names */
    for (const std::shared_ptr<GltfNode>& Node : mNodeList)
    {
        if (Node)
        {
            std::string NodeName;
            Node->GetNodeName(NodeName);
            mModelSettings.msSkelNodeNames.push_back(NodeName);
        }
        else
        {
            mModelSettings.msSkelNodeNames.push_back("(invalid)");
        }
    }

    UpdateNodeMatrices(mRootNode);

    // mRootNode->printTree();

    /* filling anim clips  */
    mGltfModel->GetAnimClips(mAnimClips);
    for (const std::shared_ptr<GltfAnimationClip>& Clip : mAnimClips)
    {
        mModelSettings.msClipNames.push_back(Clip->GetClipName());
    }
    unsigned int AnimClipSize = mAnimClips.size();

    /* randomize some settings */
    if (bRandomize)
    {
        int AnimClip = std::rand() % AnimClipSize;
        float AnimClipSpeed = (std::rand() % 100) / 100.0f + 0.5f;
        float InitRotation = std::rand() % 360 - 180;

        mModelSettings.msAnimClip = AnimClip;
        mModelSettings.msAnimSpeed = AnimClipSpeed;
        mModelSettings.msWorldRotation = glm::vec3(0.0f, InitRotation, 0.0f);
        mRootNode->SetWorldRotation(mModelSettings.msWorldRotation);
    }

    /* initializing the method-local variables (blending mode, skeleton split, and world position/rotation) */
    CheckForUpdates();

    /*  initializing line mesh for the skeleton and resizing vertices vector to be able to store
        two vertices for every bone of the skeleton */
    mSkeletonMesh = std::make_shared<OGLMesh>();
    mSkeletonMesh->Vertices.resize(mNodeCount * 2);

    mIKSolver = std::make_unique<IKSolver>();

    /* set values for inverse kinematics */
    /* hard-code right arm here for startup */
    mModelSettings.msIKEffectorNode = 19;
    mModelSettings.msIKRootNode = 26;
    SetInverseKinematicsNodes(mModelSettings.msIKEffectorNode, mModelSettings.msIKRootNode);
    SetNumIKIterations(mModelSettings.msIKIterations);

    glm::quat WorldQuat;
    GetWorldRotation(WorldQuat);
    mModelSettings.msIKTargetWorldPos = WorldQuat * mModelSettings.msIKTargetPos + WorldPos;
}

GltfInstance::~GltfInstance()
{
    if (mGltfModel == nullptr)
    {
        Logger::Log(1, "%s error: invalid glTF model\n", __FUNCTION__);
        return;
    }

    std::string ModelName;
    mGltfModel->GetModelFilename(ModelName);
    Logger::Log(2, "%s: model instance for '%s' removed\n", __FUNCTION__, ModelName.c_str());
}

void GltfInstance::ResetNodeData()
{
    if (mGltfModel == nullptr)
    {
        Logger::Log(1, "%s: Invalid Model ref\n", __FUNCTION__);
        return;
    }

    if (mRootNode == nullptr)
    {
        Logger::Log(1, "%s: Invalid RootNode\n", __FUNCTION__);
        return;
    }

    mGltfModel->ResetNodeData(mRootNode);
    UpdateNodeMatrices(mRootNode);
}

void GltfInstance::SetSkeletonSplitNode(const int NodeIndex)
{
    if (NodeIndex >= MAX_GLTF_NODES)
    {
        Logger::Log(1, "%s: Invalid Node Index\n", __FUNCTION__);
        return;
    }

    mAdditiveAnimationMask.set();
    UpdateAdditiveMask(mRootNode, NodeIndex);
}

std::shared_ptr<OGLMesh> GltfInstance::GetSkeleton()
{
    if (mSkeletonMesh == nullptr)
    {
        Logger::Log(1, "%s: Invalid SkeletonMesh\n", __FUNCTION__);
        return nullptr;
    }

    if (mRootNode == nullptr)
    {
        Logger::Log(1, "%s: Invalid RootNode\n", __FUNCTION__);
        return nullptr;
    }

    mSkeletonMesh->Vertices.clear();

    std::vector<std::shared_ptr<GltfNode>> Children;
    mRootNode->GetChildren(Children);

    if (Children.empty())
    {
        Logger::Log(1, "%s: Invalid Root Childrens\n", __FUNCTION__);
        return nullptr;
    }

    /* start from Armature child */
    GetSkeletonPerNode(Children.at(0));
    return mSkeletonMesh;

}

int GltfInstance::GetJointMatrixSize() const
{
    return mJointMatrices.size();
}

void GltfInstance::GetJointMatrices(std::vector<glm::mat4> &OutJointMatrices)
{
    OutJointMatrices = mJointMatrices;
}

int GltfInstance::GetJointDualQuatsSize() const
{
    return mJointDualQuats.size();
}

void GltfInstance::GetJointDualQuats(std::vector<glm::mat2x4> &OutJointDualQuats)
{
    OutJointDualQuats = mJointDualQuats;
}

void GltfInstance::SetInstanceSettings(const ModelSettings &InSettings)
{
    mModelSettings = InSettings;
}

void GltfInstance::GetInstanceSettings(ModelSettings &OutModelSettings)
{
    OutModelSettings = mModelSettings;
}

void GltfInstance::UpdateAnimation()
{
    const bool bWillBlend = mModelSettings.msBlendingMode != EBlendMode::FadeInOut;
    const int DestAnimClip = bWillBlend ? mModelSettings.msCrossBlendDestAnimClip : -1;
    const float BlendFactor = bWillBlend ? mModelSettings.msAnimCrossBlendFactor : mModelSettings.msAnimBlendFactor;
    if (mModelSettings.msPlayAnimation)
    {
        PlayAnimation(mModelSettings.msAnimClip, BlendFactor, DestAnimClip,
            mModelSettings.msAnimSpeed, mModelSettings.msAnimationPlayDirection);
    }
    else
    {
        mModelSettings.msAnimEndTime = GetAnimationEndTime(mModelSettings.msAnimClip);
        BlendAnimationFrame(mModelSettings.msAnimClip, mModelSettings.msAnimTimePosition,
              BlendFactor, DestAnimClip);
    }
}

void GltfInstance::CheckForUpdates()
{
  static EBlendMode LastBlendMode = mModelSettings.msBlendingMode;
  static int SkelSplitNode = mModelSettings.msSkelSplitNode;
  static glm::vec3 WorldPos = mModelSettings.msWorldPosition;
  static glm::vec3 WorldRot = mModelSettings.msWorldRotation;
  static glm::vec3 IKTargetPos = mModelSettings.msIKTargetPos;
  static EIKSolver LastIkMode = mModelSettings.msIKSolver;
  static int NumIKIterations = mModelSettings.msIKIterations;
  static int IKEffectorNode = mModelSettings.msIKEffectorNode;
  static int IKRootNode = mModelSettings.msIKRootNode;

  if (SkelSplitNode != mModelSettings.msSkelSplitNode)
  {
    SetSkeletonSplitNode(mModelSettings.msSkelSplitNode);
    SkelSplitNode = mModelSettings.msSkelSplitNode;
    ResetNodeData();
  }

  if (LastBlendMode != mModelSettings.msBlendingMode)
  {
    LastBlendMode = mModelSettings.msBlendingMode;
    if (mModelSettings.msBlendingMode != EBlendMode::Additive)
    {
      mModelSettings.msSkelSplitNode = mNodeCount - 1;
    }
    ResetNodeData();
  }

  if (WorldPos != mModelSettings.msWorldPosition)
  {
      mRootNode->SetWorldPosition(mModelSettings.msWorldPosition);
      WorldPos = mModelSettings.msWorldPosition;

      glm::quat WorldQuat;
      GetWorldRotation(WorldQuat);
      mModelSettings.msIKTargetWorldPos = WorldQuat * mModelSettings.msIKTargetPos + WorldPos;
  }

  if (WorldRot != mModelSettings.msWorldRotation)
  {
      mRootNode->SetWorldRotation(mModelSettings.msWorldRotation);
      WorldRot = mModelSettings.msWorldRotation;
      glm::quat WorldQuat;
      GetWorldRotation(WorldQuat);
      mModelSettings.msIKTargetWorldPos = WorldQuat * mModelSettings.msIKTargetPos + WorldPos;
  }

  if (IKTargetPos != mModelSettings.msIKTargetPos)
  {
      IKTargetPos = mModelSettings.msIKTargetPos;
      glm::quat WorldQuat;
      GetWorldRotation(WorldQuat);
      mModelSettings.msIKTargetWorldPos = WorldQuat * mModelSettings.msIKTargetPos + WorldPos;
  }

  if (LastIkMode != mModelSettings.msIKSolver)
  {
    ResetNodeData();
    LastIkMode = mModelSettings.msIKSolver;
  }

  if (NumIKIterations != mModelSettings.msIKIterations)
  {
      SetNumIKIterations(mModelSettings.msIKIterations);
      ResetNodeData();
      NumIKIterations = mModelSettings.msIKIterations;
  }

  if (IKEffectorNode != mModelSettings.msIKEffectorNode || IKRootNode != mModelSettings.msIKRootNode)
  {
      SetInverseKinematicsNodes(mModelSettings.msIKEffectorNode, mModelSettings.msIKRootNode);
      ResetNodeData();
      IKEffectorNode = mModelSettings.msIKEffectorNode;
      IKRootNode = mModelSettings.msIKRootNode;
  }
}

void GltfInstance::GetWorldPosition(glm::vec3 &OutWorldPos)
{
    OutWorldPos = mModelSettings.msWorldPosition;
}

void GltfInstance::GetWorldRotation(glm::quat &OutWorldQuat)
{
    OutWorldQuat = glm::normalize(glm::quat(glm::vec3(
    glm::radians(mModelSettings.msWorldRotation.x),
    glm::radians(mModelSettings.msWorldRotation.y),
    glm::radians(mModelSettings.msWorldRotation.z)
    )));
}

void GltfInstance::PlayAnimation(const int SourceAnimIndex, const float BlendFactor, const int DestAnimIndex,
    const float PlaybackSpeed, const EPlaybackDirection PlaybackDirection)
{
    if (mAnimClips.empty() || (SourceAnimIndex < 0) || (SourceAnimIndex >= mAnimClips.size()))
    {
        Logger::Log(1, "%s: no valid animations to play\n", __FUNCTION__);
        return;
    }

    const float ClipEndTime = mAnimClips.at(SourceAnimIndex)->GetClipEndTime();
    double CurrentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    double AnimTime = std::fmod((CurrentTime / 1000.0) * PlaybackSpeed, ClipEndTime);
    if (PlaybackDirection == EPlaybackDirection::Backward)
    {
        AnimTime = ClipEndTime - AnimTime;
    }
    BlendAnimationFrame(SourceAnimIndex, AnimTime, BlendFactor, DestAnimIndex);
}

void GltfInstance::BlendAnimationFrame(const int SourceAnimIndex, const float Time, const float BlendFactor,
    const int DestAnimIndex)
{
    if (mAnimClips.empty() || (SourceAnimIndex < 0) || (SourceAnimIndex >= mAnimClips.size()))
    {
        Logger::Log(1, "%s: no valid animations\n", __FUNCTION__);
        return;
    }

    mAnimClips.at(SourceAnimIndex)->BlendAnimationFrame(mNodeList, mAdditiveAnimationMask, Time, BlendFactor);
    UpdateNodeMatrices(mRootNode);
}

float GltfInstance::GetAnimationEndTime(const int AnimIndex) const
{
    if (mAnimClips.empty() || (AnimIndex < 0) || (AnimIndex >= mAnimClips.size()))
    {
        Logger::Log(1, "%s: no valid animations\n", __FUNCTION__);
        return 0.0f;
    }

    return mAnimClips.at(AnimIndex)->GetClipEndTime();
}

void GltfInstance::GetSkeletonPerNode(std::shared_ptr<GltfNode> TreeNode)
{
    if (TreeNode == nullptr)
    {
        Logger::Log(1, "%s: Invalid TreeNode\n", __FUNCTION__);
        return;
    }

    glm::vec3 ParentPos = glm::vec3(0.0f);
    glm::mat4 NodeMatrix;
    TreeNode->GetNodeMatrix(NodeMatrix);
    ParentPos = glm::vec3(NodeMatrix[3]);
    OGLVertex parentVertex;
    parentVertex.Position = ParentPos;
    parentVertex.Color = glm::vec3(0.0f, 1.0f, 1.0f);

    std::vector<std::shared_ptr<GltfNode>> ChildrenNodes;
    TreeNode->GetChildren(ChildrenNodes);
    for (const auto& ChildNode : ChildrenNodes)
    {
        glm::vec3 childPos = glm::vec3(0.0f);
        glm::mat4 NodeMatrix;
        ChildNode->GetNodeMatrix(NodeMatrix);
        childPos = glm::vec3(NodeMatrix[3]);
        OGLVertex childVertex;
        childVertex.Position = childPos;
        childVertex.Color = glm::vec3(0.0f, 0.0f, 1.0f);
        mSkeletonMesh->Vertices.emplace_back(parentVertex);
        mSkeletonMesh->Vertices.emplace_back(childVertex);

        GetSkeletonPerNode(ChildNode);
    }
}

void GltfInstance::UpdateNodeMatrices(std::shared_ptr<GltfNode> &TreeNode)
{
    if (TreeNode == nullptr)
    {
        Logger::Log(1, "%s: Invalid TreeNode\n", __FUNCTION__);
        return;
    }

    TreeNode->CalculateNodeMatrix();

    if (mModelSettings.msVertexSkinningMode == ESkinningMode::Linear)
    {
        UpdateJointMatrices(TreeNode);
    }
    else
    {
        UpdateJointDualQuats(TreeNode);
    }

    glm::mat4 TreeNodeMatrix;
    TreeNode->GetNodeMatrix(TreeNodeMatrix);

    std::vector<std::shared_ptr<GltfNode>> ChildrenNodes;
    TreeNode->GetChildren(ChildrenNodes);
    for (auto& ChildNode : ChildrenNodes)
    {
        UpdateNodeMatrices(ChildNode);
    }
}

void GltfInstance::UpdateJointMatrices(std::shared_ptr<GltfNode> &TreeNode)
{
    if (TreeNode == nullptr || mInverseBindMatrices.empty() || mNodeToJoint.empty() || mJointMatrices.empty())
    {
        Logger::Log(1, "%s: Invalid TreeNode\n", __FUNCTION__);
        return;
    }

    glm::mat4 TreeNodeMatrix;
    TreeNode->GetNodeMatrix(TreeNodeMatrix);

    /* multiply the node matrix and the inverse bind matrix to create the final transformation matrix
    for the positional change of every vertex of a node appearing in the joints array */
    const int NodeIndex = TreeNode->GetNodeIndex();
    mJointMatrices.at(mNodeToJoint.at(NodeIndex)) = TreeNodeMatrix * mInverseBindMatrices.at(mNodeToJoint.at(NodeIndex));
}

void GltfInstance::UpdateJointDualQuats(std::shared_ptr<GltfNode> &TreeNode)
{
    if (TreeNode == nullptr || mInverseBindMatrices.empty() || mNodeToJoint.empty() || mJointMatrices.empty())
    {
        Logger::Log(1, "%s: Invalid TreeNode\n", __FUNCTION__);
        return;
    }

    const int NodeIndex = TreeNode->GetNodeIndex();

    glm::mat4 NodeJointMat;
    TreeNode->GetNodeMatrix(NodeJointMat);
    NodeJointMat *= mInverseBindMatrices.at(mNodeToJoint.at(NodeIndex));

    /* extract components from node matrix */
    glm::quat Orientation;
    glm::vec3 Scale;
    glm::vec3 Translation;
    glm::vec3 Skew;
    glm::vec4 Perspective;
    glm::dualquat DualQuat;

    /* create dual quaternion */
    if (glm::decompose(NodeJointMat, Scale, Orientation,Translation, Skew, Perspective))
    {
        // qTranslation = 1/2 * t * qRotation
        DualQuat[0] = Orientation;
        DualQuat[1] = glm::quat(0.0, Translation.x, Translation.y, Translation.z) * Orientation * 0.5f;
        mJointDualQuats.at(mNodeToJoint.at(NodeIndex)) = glm::mat2x4_cast(DualQuat);

        //mJointCPUDualQuats.at(mNodeToJoint.at(NodeIndex)) = DualQuat;
    }
    else
    {
        Logger::Log(1, "%s error: could not decompose matrix for node %i\n", __FUNCTION__, NodeIndex);
    }
}

void GltfInstance::UpdateAdditiveMask(const std::shared_ptr<GltfNode> &TreeNode, const int SplitNodeIndex)
{
    if (TreeNode == nullptr)
    {
        Logger::Log(1, "%s: Invalid TreeNode\n", __FUNCTION__);
        return;
    }

    const int NodeIndex = TreeNode->GetNodeIndex();
    if((NodeIndex >= MAX_GLTF_NODES) || (NodeIndex == SplitNodeIndex))
    {
        Logger::Log(1, "%s: Invalid Node Index\n", __FUNCTION__);
        return;
    }

    /* if the current node is above the split node, it will no longer be part of the animation clip,
     set the mask for the current node to false */
    //   mAdditiveAnimationMask.at(treeNode->getNodeNum()) = false;
    mAdditiveAnimationMask.set(NodeIndex, false);

    std::vector<std::shared_ptr<GltfNode>> ChildrenNodes;
    TreeNode->GetChildren(ChildrenNodes);
    for (std::shared_ptr<GltfNode>& ChildNode : ChildrenNodes)
    {
        UpdateAdditiveMask(ChildNode, SplitNodeIndex);
    }
}

void GltfInstance::SetInverseKinematicsNodes(int EffectorNodeIndex, int IKChainRootNodeIndex)
{
    if (mIKSolver == nullptr)
    {
        Logger::Log(1, "%s: Invalid IK Solver\n", __FUNCTION__);
        return;
    }

    const int NumNodes = mNodeList.size();
    if (EffectorNodeIndex < 0 || EffectorNodeIndex > (NumNodes - 1))
    {
        Logger::Log(1, "%s error: effector node %i is out of range\n", __FUNCTION__, EffectorNodeIndex);
        return;
    }

    if (IKChainRootNodeIndex < 0 || IKChainRootNodeIndex > (NumNodes - 1))
    {
        Logger::Log(1, "%s error: IK chaine root node %i is out of range\n", __FUNCTION__, IKChainRootNodeIndex);
        return;
    }

    std::vector<std::shared_ptr<GltfNode>> IKNodes{};
    IKNodes.insert(IKNodes.begin(), mNodeList.at(EffectorNodeIndex));

    int CurrentNodeIndex = EffectorNodeIndex;
    while (CurrentNodeIndex != IKChainRootNodeIndex)
    {
        std::shared_ptr<GltfNode> Node = mNodeList.at(CurrentNodeIndex);
        if (Node != nullptr)
        {
            std::shared_ptr<GltfNode> ParentNode = Node->GetParentNode();
            if (ParentNode != nullptr)
            {
                CurrentNodeIndex = ParentNode->GetNodeIndex();
                IKNodes.push_back(ParentNode);
            }
            else
            {
                Logger::Log(1, "%s error: reached skeleton root node, stopping\n", __FUNCTION__);
                break;
            }
        }
    }

    mIKSolver->SetNodes(IKNodes);
}

void GltfInstance::SetNumIKIterations(int Iterations)
{
    if (mIKSolver == nullptr)
    {
        Logger::Log(1, "%s: Invalid IK Solver\n", __FUNCTION__);
        return;
    }

    mIKSolver->SetNumIterations(Iterations);
}

void GltfInstance::SolveIK()
{
    if (mIKSolver == nullptr)
    {
        Logger::Log(1, "%s: Invalid IK Solver\n", __FUNCTION__);
        return;
    }

    switch (mModelSettings.msIKSolver)
    {
        case EIKSolver::CCD:
        {
            mIKSolver->SolveCCD(mModelSettings.msIKTargetWorldPos);
            break;
        }
        case EIKSolver::FABRIK:
        {
            mIKSolver->SolveFABRIK(mModelSettings.msIKTargetWorldPos);
            break;
        }
        default:
            break;
    }

    std::shared_ptr<GltfNode> IKRootRef = mIKSolver->GetIKChainRootNode();
    // updating the vertex skinning matrices
    UpdateNodeMatrices(IKRootRef);
}
