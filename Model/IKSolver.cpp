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
}

std::shared_ptr<GltfNode> IKSolver::GetIKChainRootNode()
{
    if (mNodes.empty())
    {
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
