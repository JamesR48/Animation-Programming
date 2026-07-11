#include "IKSolver.h"
#include "GltfNode.h"
#include "../Tools/Logger.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>


IKSolver::IKSolver()
{
}

IKSolver::IKSolver(unsigned int Iterations)
{
    mIterations = Iterations;
}

void IKSolver::SetNodes(std::vector<std::shared_ptr<GltfNode>> Nodes)
{
    mNodes = Nodes;

    for (const std::shared_ptr<GltfNode>& Node : mNodes)
    {
        if (Node != nullptr)
        {
            std::string NodeName;
            Node->GetNodeName(NodeName);
            Logger::Log(2, "%s: added node %s to IK solver\n", __FUNCTION__, NodeName.c_str());
        }
    }

    CalculateBoneLengths();
    mFABRIKNodePositions.resize(mNodes.size());
}

std::shared_ptr<GltfNode> IKSolver::GetIKChainRootNode()
{
    if (mNodes.empty())
    {
        Logger::Log(1, "%s: no valid nodes in the chain\n", __FUNCTION__);
        return nullptr;
    }

    return mNodes.at(mNodes.size() - 1);
}

void IKSolver::SetNumIterations(unsigned int Iterations)
{
    mIterations = Iterations;
}

bool IKSolver::SolveCCD(glm::vec3 Target)
{
    if (mNodes.empty())
    {
        Logger::Log(1, "%s: no valid nodes in the chain\n", __FUNCTION__);
        return false;
    }

    const float ThresholdSqr = mThreshold * mThreshold;
    const int NumNodes = mNodes.size();
    std::shared_ptr<GltfNode> EffectorNode = mNodes[0];
    for (unsigned int i = 0; i < mIterations; i++)
    {
        glm::vec3 EffectorPos;
        EffectorNode->GetGlobalPosition(EffectorPos);
        if (glm::length2(Target - EffectorPos) < ThresholdSqr)
        {
            return true;
        }

        for (unsigned int j = 1; j < NumNodes; j++)
        {
            std::shared_ptr<GltfNode> Node = mNodes[j];
            if (Node == nullptr)
            {
                continue;
            }

            glm::vec3 NodePos;
            Node->GetGlobalPosition(NodePos);
            glm::quat NodeRot;
            Node->GetGlobalRotation(NodeRot);

            glm::vec3 toEffector = EffectorPos - NodePos;
            glm::vec3 toTarget = Target - NodePos;
            if (glm::length2(toEffector) < mThreshold || glm::length2(toTarget) < mThreshold)
            {
                continue;
            }
            glm::vec3 NodeToEffector = glm::normalize(toEffector);
            glm::vec3 NodeToTarget = glm::normalize(toTarget);

            // shortest arc rotation needed to align NodeToEffector with NodeToTarget
            glm::quat EffectorToTargetRot = glm::rotation(NodeToEffector, NodeToTarget);

            /*
             Q(local) = Q(world_orientation) * Q(world_rotation) * Q(world_orientation)^-1
 
            transforming the world-space rotation to local-space. reorient the quaternion by appending it to
            the global rotation orientation of the node, and then undoing the global orientation by rotating around
            its conjugate. The resulting local quaternion contains the rotation of an imaginary unit quaternion
            around the local object axis, but with the same amount that the effectorToTarget quaternion has.
            This is done since bones in a skeleton are animated in local space (relative to their parent bone)
             */
            glm::quat EffectorToTargetLocalRot = NodeRot * EffectorToTargetRot * glm::conjugate(NodeRot);

            glm::quat NodeLocalRot;
            Node->GetLocalRotation(NodeLocalRot);
            glm::quat NewLocalRotation = glm::normalize(NodeLocalRot * EffectorToTargetLocalRot);
            Node->BlendRotation(NewLocalRotation, 1.0f);

            /* update the node matrices, current node to effector
            to reflect the local changes down the chain */
            Node->UpdateNodeAndChildMatrices();

            glm::vec3 NewEffectorPos;
            EffectorNode->GetGlobalPosition(NewEffectorPos);
            if (glm::length2(Target - NewEffectorPos) < ThresholdSqr)
            {
                return true;
            }

            /* update effector position for the next joint in the chain */
            EffectorPos = NewEffectorPos;
        }
    }

    return false;
}

bool IKSolver::SolveFABRIK(glm::vec3 Target)
{
    if (mNodes.empty())
    {
        Logger::Log(1, "%s: no valid nodes in the chain\n", __FUNCTION__);
        return false;
    }

    const int NumNodes = mNodes.size();
    for (int i = 0; i < NumNodes; i++)
    {
        glm::vec3 NodePos;
        mNodes[i]->GetGlobalPosition(NodePos);
        mFABRIKNodePositions.at(i) = NodePos;
    }

    glm::vec3 BaseOriginalPos;
    GetIKChainRootNode()->GetGlobalPosition(BaseOriginalPos);

    const float ThresholdSqr = mThreshold * mThreshold;
    for (unsigned int i = 0; i < mIterations; i++)
    {
        glm::vec3 EffectorPos = mFABRIKNodePositions.at(0);

        // close enough to the target? Stop iterating and adjust a little
        if (glm::length2(Target - EffectorPos) < ThresholdSqr)
        {
            AdjustFABRIKNodes();
            return true;
        }

        SolveFABRIKForward(Target);
        SolveFABRIKBackward(BaseOriginalPos);
    }

    AdjustFABRIKNodes();

    glm::vec3 EffectorPos;
    mNodes.at(0)->GetGlobalPosition(EffectorPos);
    return (glm::length2(Target - EffectorPos) < ThresholdSqr);
}

void IKSolver::CalculateBoneLengths()
{
    if (mNodes.empty())
    {
        Logger::Log(1, "%s: no valid nodes in the chain\n", __FUNCTION__);
        return;
    }

    const int NumNodes = mNodes.size() - 1;
    mBoneLengths.resize(NumNodes);

    for (int i = 0; i < NumNodes; i++)
    {
        const std::shared_ptr<GltfNode>& Node = mNodes[i];
        const std::shared_ptr<GltfNode>& NextNode = mNodes[i+1];
        if (Node == nullptr)
        {
            continue;
        }

        glm::vec3 Position;
        Node->GetGlobalPosition(Position);
        glm::vec3 NextPosition;
        NextNode->GetGlobalPosition(NextPosition);

        const float BoneLength = glm::length(NextPosition - Position);
        mBoneLengths.at(i) = BoneLength;
        Logger::Log(2, "%s: bone %i has length %f\n", __FUNCTION__, i, BoneLength);
    }
}

/* moving bones forward, closer to target */
void IKSolver::SolveFABRIKForward(glm::vec3 Target)
{
    if (mFABRIKNodePositions.empty())
    {
        Logger::Log(1, "%s: no valid FABRIK node positions\n", __FUNCTION__);
        return;
    }

    /* setting effector to target */
    mFABRIKNodePositions.at(0) = Target;

    const int NumPositions = mFABRIKNodePositions.size();
    for (int i = 1; i < NumPositions; i++)
    {
        const glm::vec3 PrevNodePos = mFABRIKNodePositions[i-1];
        const glm::vec3 CurrentNodePos = mFABRIKNodePositions[i];
        const glm::vec3 BoneDirection = glm::normalize(CurrentNodePos - PrevNodePos);

        const float PrevToCurrentLength = mBoneLengths[i-1];
        const glm::vec3 PrevToCurrentOffset = BoneDirection * PrevToCurrentLength;
        mFABRIKNodePositions.at(i) = PrevNodePos + PrevToCurrentOffset;
    }
}

/* moving bones backward, back to reach base */
void IKSolver::SolveFABRIKBackward(glm::vec3 Base)
{
    if (mFABRIKNodePositions.empty())
    {
        Logger::Log(1, "%s: no valid FABRIK node positions\n", __FUNCTION__);
        return;
    }

    const int NumPositions = mFABRIKNodePositions.size();

    /* setting root node back to (saved) base */
    mFABRIKNodePositions.at(NumPositions - 1) = Base;

    for (int i = NumPositions - 2; i >= 0; i--)
    {
        const glm::vec3 CurrentNodePos = mFABRIKNodePositions[i];
        const glm::vec3 PrevNodePos = mFABRIKNodePositions[i+1];
        const glm::vec3 BoneDirection = glm::normalize(CurrentNodePos - PrevNodePos);

        const float PrevToCurrentLength = mBoneLengths[i];
        const glm::vec3 PrevToCurrentOffset = BoneDirection * PrevToCurrentLength;
        mFABRIKNodePositions[i] = PrevNodePos + PrevToCurrentOffset;
    }
}

/* rotating the bones, starting with the root node */
void IKSolver::AdjustFABRIKNodes()
{
    if (mFABRIKNodePositions.empty())
    {
        Logger::Log(1, "%s: no valid FABRIK node positions\n", __FUNCTION__);
        return;
    }

    if (mNodes.empty())
    {
        Logger::Log(1, "%s: no valid nodes in the chain\n", __FUNCTION__);
        return;
    }

    const int NumPositions = mFABRIKNodePositions.size();

    // walking the node chain backward again, from the root node to the effector node
    for (int i = NumPositions - 1; i > 0; --i)
    {
        std::shared_ptr<GltfNode> CurrentNode = mNodes[i];
        std::shared_ptr<GltfNode> NextNode = mNodes[i-1];

        glm::vec3 CurrentPos;
        CurrentNode->GetGlobalPosition(CurrentPos);
        glm::vec3 NextPos;
        NextNode->GetGlobalPosition(NextPos);

        // current orientation of the original bone
        const glm::vec3 ToNext = glm::normalize(NextPos - CurrentPos);

        // orientation of the same bone after the FABRIK solver changed the copied positions of the nodes
        const glm::vec3 ToDesired = glm::normalize(mFABRIKNodePositions[i-1] - mFABRIKNodePositions[i]);

        // shortest arc rotation needed to align the current bone orientation with its desired one
        glm::quat NodeLocalRotation = glm::rotation(ToNext, ToDesired);

        glm::quat CurrentWorldRot;
        CurrentNode->GetGlobalRotation(CurrentWorldRot);

        // Q(local) = Q(world_orientation) * Q(world_rotation) * Q(world_orientation)^-1
        glm::quat NewLocalRotation = CurrentWorldRot * NodeLocalRotation * glm::conjugate(CurrentWorldRot);

        glm::quat CurrentLocalRot;
        CurrentNode->GetLocalRotation(CurrentLocalRot);
        CurrentNode->BlendRotation(CurrentLocalRot * NewLocalRotation, 1.0f);

        /* updating the node matrices, current node to effector to reflect the local changes down the chain */
        CurrentNode->UpdateNodeAndChildMatrices();
    }
}
