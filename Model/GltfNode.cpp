#include "GltfNode.h"
#include <algorithm>
#include "../Tools/Logger.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

GltfNode::GltfNode()
{
}

GltfNode::~GltfNode() = default;

std::shared_ptr<GltfNode> GltfNode::GetParentNode()
{
    // converting weak_ptr to shared_ptr
    std::shared_ptr<GltfNode> ParentNode = mParentNode.lock();
    // pointer is set and not pending destroy?
    if (ParentNode)
    {
        return ParentNode;
    }

    return nullptr;
}

std::shared_ptr<GltfNode> GltfNode::CreateRoot(int RootNodeIndex)
{
    std::shared_ptr<GltfNode> mParentNode = std::make_shared<GltfNode>();
    mParentNode->mNodeIndex = RootNodeIndex;
    return mParentNode;
}

void GltfNode::AddChildren(const std::vector<int>& ChildrenNodes)
{
    for (const int childNode : ChildrenNodes)
    {
        std::shared_ptr<GltfNode> child = std::make_shared<GltfNode>();
        child->mNodeIndex = childNode;
        child->mParentNode = shared_from_this();
        mChildrenNodes.push_back(child);
    }
}

void GltfNode::GetChildren(std::vector<std::shared_ptr<GltfNode>>& OutChildren)
{
    OutChildren = mChildrenNodes;
}

int GltfNode::GetNodeIndex() const
{
    return mNodeIndex;
}

void GltfNode::GetNodeName(std::string &OutNodeName)
{
    OutNodeName = mNodeName;
}

void GltfNode::SetNodeName(const std::string& Name)
{
    mNodeName = Name;
}

void GltfNode::SetScale(const glm::vec3& Scale)
{
    mScale = Scale;
    mBlendScale = Scale;
}

void GltfNode::SetTranslation(const glm::vec3& Translation)
{
    mTranslation = Translation;
    mBlendTranslation = Translation;
}

void GltfNode::SetRotation(const glm::quat& Rotation)
{
    mRotation = Rotation;
    mBlendRotation = Rotation;
}

void GltfNode::BlendScale(const glm::vec3 &Scale, float BlendFactor)
{
    // Linear interp
    float Factor = std::clamp(BlendFactor, 0.0f, 1.0f);
    mBlendScale = Scale * Factor + mScale * (1.0f - Factor);
}

void GltfNode::BlendTranslation(const glm::vec3 &Translation, float BlendFactor)
{
    // Linear interp
    float Factor = std::clamp(BlendFactor, 0.0f, 1.0f);
    mBlendTranslation = Translation * Factor + mTranslation * (1.0f - Factor);
}

void GltfNode::BlendRotation(const glm::quat &Rotation, float BlendFactor)
{
    // Slerp
    float Factor = std::clamp(BlendFactor, 0.0f, 1.0f);
    mBlendRotation = glm::slerp(mRotation, Rotation, Factor);
}

void GltfNode::GetLocalRotation(glm::quat &OutRotation)
{
    OutRotation = mBlendRotation;
}

void GltfNode::GetGlobalRotation(glm::quat &OutRotation)
{
    glm::quat Orientation;
    glm::vec3 Scale;
    glm::vec3 Translation;
    glm::vec3 Skew;
    glm::vec4 Perspective;

    if (!glm::decompose(mNodeMatrix, Scale, Orientation,Translation, Skew, Perspective))
    {
        OutRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    }

    // returning the inverse since decompose outputs the conjugate of the orientation
    OutRotation = glm::inverse(Orientation);
}

void GltfNode::GetGlobalPosition(glm::vec3 &OutPosition)
{
    glm::quat Orientation;
    glm::vec3 Scale;
    glm::vec3 Translation;
    glm::vec3 Skew;
    glm::vec4 Perspective;

    if (!glm::decompose(mNodeMatrix, Scale, Orientation,Translation, Skew, Perspective))
    {
        OutPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    }

    OutPosition = Translation;
}

void GltfNode::CalculateLocalTRSMatrix()
{
    glm::mat4 sMatrix = glm::scale(glm::mat4(1.0f), mBlendScale);
    glm::mat4 rMatrix = glm::mat4_cast(mBlendRotation);
    glm::mat4 tMatrix = glm::translate(glm::mat4(1.0f), mBlendTranslation);
    mLocalTRSMatrix = tMatrix * rMatrix * sMatrix;
}

void GltfNode::CalculateNodeMatrix()
{
    CalculateLocalTRSMatrix();
    glm::mat4 ParentMatrix = glm::mat4(1.0f);
    std::shared_ptr<GltfNode> ParentNode = mParentNode.lock();
    if (ParentNode)
    {
        ParentNode->GetNodeMatrix(ParentMatrix);
    }
    mNodeMatrix = ParentMatrix * mLocalTRSMatrix;
}

void GltfNode::GetNodeMatrix(glm::mat4& OutNodeMatrix)
{
    OutNodeMatrix = mNodeMatrix;
}

void GltfNode::UpdateNodeAndChildMatrices()
{
    CalculateNodeMatrix();
    for (std::shared_ptr<GltfNode>& Child : mChildrenNodes)
    {
        if (Child)
        {
            Child->UpdateNodeAndChildMatrices();
        }
    }
}

void GltfNode::PrintTree()
{
    Logger::Log(1, "%s: ---- tree ----\n", __FUNCTION__);
    Logger::Log(1, "%s: parent : %i (%s)\n", __FUNCTION__, mNodeIndex, mNodeName.c_str());
    for (const auto& ChildNode : mChildrenNodes)
    {
        GltfNode::PrintNodes(ChildNode, 1);
    }
    Logger::Log(1, "%s: -- end tree --\n", __FUNCTION__);
}

void GltfNode::PrintNodes(std::shared_ptr<GltfNode> StartNode, int Indent)
{
    std::string IndentedString = "";
    for (int i = 0; i < Indent; ++i)
    {
        IndentedString += " ";
    }
    IndentedString += "-";
    Logger::Log(1, "%s: %s child : %i (%s)\n", __FUNCTION__, IndentedString.c_str(), StartNode->mNodeIndex, StartNode->mNodeName.c_str());

    for (const auto& childNode : StartNode->mChildrenNodes)
    {
        GltfNode::PrintNodes(childNode, Indent + 1);
    }
}
