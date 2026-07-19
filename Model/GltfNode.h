#ifndef CPPANIMPROGRAMMING_GLTFNODE_H
#define CPPANIMPROGRAMMING_GLTFNODE_H

#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class GltfNode : public std::enable_shared_from_this<GltfNode>
{
public:
    GltfNode();
    ~GltfNode();

    std::shared_ptr<GltfNode> GetParentNode();
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

    void SetWorldPosition(const glm::vec3& Position);
    void GetWorldPosition(glm::vec3& OutWorldPos);
    void SetWorldRotation(const glm::vec3& InRotation);

    void GetLocalRotation(glm::quat& OutRotation);
    void GetGlobalRotation(glm::quat& OutRotation);
    void GetGlobalPosition(glm::vec3& OutPosition);

    void CalculateLocalTRSMatrix();
    void CalculateNodeMatrix();
    void GetNodeMatrix(glm::mat4& OutNodeMatrix);
    void UpdateNodeAndChildMatrices();

    void PrintTree();

private:
    void PrintNodes(std::shared_ptr<GltfNode> StartNode, int Indent);

    int mNodeIndex = 0;
    std::string mNodeName;
    std::vector<std::shared_ptr<GltfNode>> mChildrenNodes{};

    /*
    using a weak_ptr to avoid circular dependencies – the parent node stores its children nodes already as shared_ptr,
    and if shared_ptr is used for the parent node too, the reference counter of both pointers would never reach zero,
    as each node waits for its counterpart to be destroyed first.
    A weak_ptr breaks such a circular dependency by not counting toward the shared reference counter
     */
    std::weak_ptr<GltfNode> mParentNode;

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

    glm::vec3 mWorldPosition = glm::vec3(0.0f);
    glm::vec3 mWorldRotation = glm::vec3(0.0f);

    // TRS: Translation * Rotation * Scale
    glm::mat4 mLocalTRSMatrix = glm::mat4(1.0f);

    // parentNodeMatrix * mLocalTRSMatrix
    glm::mat4 mNodeMatrix = glm::mat4(1.0f);

    glm::mat4 mInverseBindMatrix = glm::mat4(1.0f);
};

#endif //CPPANIMPROGRAMMING_GLTFNODE_H
