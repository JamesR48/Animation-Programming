#ifndef CPPANIMPROGRAMMING_GLTFNODE_H
#define CPPANIMPROGRAMMING_GLTFNODE_H

#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class GltfNode
{
public:
    GltfNode();
    ~GltfNode();

    static std::shared_ptr<GltfNode> CreateRoot(int RootNodeIndex);
    void AddChildren(const std::vector<int>& ChildrenNodes);
    void GetChildren(std::vector<std::shared_ptr<GltfNode>>& OutChildren);
    int GetNodeIndex() const;

    void GetNodeName(std::string& OutNodeName);
    void SetNodeName(const std::string& Name);

    void SetScale(const glm::vec3& Scale);
    void SetTranslation(const glm::vec3& Translation);
    void SetRotation(const glm::quat& Rotation);

    void BlendScale(const glm::vec3& Scale, float BlendFactor);
    void BlendTranslation(const glm::vec3& Translation, float BlendFactor);
    void BlendRotation(const glm::quat& Rotation, float BlendFactor);

    void CalculateLocalTRSMatrix();
    void CalculateNodeMatrix(const glm::mat4& ParentNodeMatrix);
    void GetNodeMatrix(glm::mat4& OutNodeMatrix);

    void PrintTree();

private:
    void PrintNodes(std::shared_ptr<GltfNode> StartNode, int Indent);

    int mNodeIndex = 0;
    std::string mNodeName;
    std::vector<std::shared_ptr<GltfNode>> mChildrenNodes{};

    // storing the node's binding pose data
    glm::vec3 mScale = glm::vec3(1.0f);
    glm::vec3 mTranslation = glm::vec3(0.0f);
    glm::quat mRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    glm::vec3 mBlendScale = glm::vec3(1.0f);
    glm::vec3 mBlendTranslation = glm::vec3(0.0f);
    glm::quat mBlendRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    glm::vec3 mSourceBlendScale = glm::vec3(1.0f);
    glm::vec3 mSourceBlendTranslation = glm::vec3(0.0f);
    glm::quat mSourceBlendRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    glm::vec3 mDestBlendScale = glm::vec3(1.0f);
    glm::vec3 mDestBlendTranslation = glm::vec3(0.0f);
    glm::quat mDestBlendRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    // TRS: Translation * Rotation * Scale
    glm::mat4 mLocalTRSMatrix = glm::mat4(1.0f);

    // parentNodeMatrix * mLocalTRSMatrix
    glm::mat4 mNodeMatrix = glm::mat4(1.0f);

    glm::mat4 mInverseBindMatrix = glm::mat4(1.0f);
};

#endif //CPPANIMPROGRAMMING_GLTFNODE_H
