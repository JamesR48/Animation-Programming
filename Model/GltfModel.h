#ifndef CPPANIMPROGRAMMING_GLTFMODEL_H
#define CPPANIMPROGRAMMING_GLTFMODEL_H

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <glad/gl.h>
#include <glm/fwd.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/dual_quaternion.hpp>

namespace tinygltf
{
    class Model;
};

class OGLRenderData;
class Texture;
class GltfNode;
class GltfAnimationClip;
struct OGLMesh;

class GltfModel
{
public:
    GltfModel();
    ~GltfModel();

    bool LoadModel(OGLRenderData& RenderData, std::string ModelFilename, std::string TextureFilename);
    void Draw();
    void Cleanup();
    void UploadVertexBuffers();
    void UploadIndexBuffer();
    void ApplyCPUVertexSkinning(bool bEnableDualQuats);
    std::shared_ptr<OGLMesh> GetSkeleton();
    int GetJointMatrixSize() const;
    void GetJointMatrices(std::vector<glm::mat4>& OutJointMatrices);
    int GetJointDualQuatsSize() const;
    void GetJointDualQuats(std::vector<glm::mat2x4>& OutJointDualQuats);

    void PlayAnimation(const int AnimIndex, const float PlaybackSpeed);
    void SetAnimationFrame(const int AnimIndex, float Time);
    float GetAnimationEndTime(const int AnimIndex) const;
    void GetClipName(const int AnimIndex, std::string& Name);

private:
    void CreateVertexBuffers();
    void CreateIndexBuffer();
    int GetTriangleCount() const;

    void GetSkeletonPerNode(std::shared_ptr<GltfNode> TreeNode);
    void GetJointData();
    void GetWeightData();
    void GetInvBindMatrices();

    /* reads the children from the corresponding node in the glTF model file and adds the correct number
    of – as yet empty – child nodes. Next, it reads the node matrix from the node given as a parameter
    and calls getNodeData() and getNodes() for every created child, traversing and creating a tree of the nodes. */
    void GetNodes(std::shared_ptr<GltfNode>& TreeNode);

    // sets the node values for translation, rotation, scale. Triggers the calculation of the LocalTRS and Node matrices
    void GetNodeData(std::shared_ptr<GltfNode>& TreeNode, const glm::mat4& ParentNodeMatrix);

    void UpdateNodeMatrices(std::shared_ptr<GltfNode>& TreeNode, const glm::mat4& ParentNodeMatrix);
    void UpdateJointMatricesAndQuats(std::shared_ptr<GltfNode>& TreeNode);

    void GetAnimations();

    std::vector<glm::tvec4<uint16_t>> mJointVec{};
    std::vector<glm::vec4> mWeightVec{};
    std::vector<glm::mat4> mJointMatrices{};
    //  GLSL shaders don’t support quaternions or dual quaternions, must use a 2x4 matrix to transport the data
    std::vector<glm::mat2x4> mJointDualQuats{};
    // To calculate the binding pose of the model
    std::vector<glm::mat4> mInverseBindMatrices{};

    std::vector<glm::dualquat> mJointCPUDualQuats{};

    std::vector<int> mAttribAccessors{};
    // look up table of joints per vertex indices
    std::vector<int> mNodeToJoint{};

    std::vector<glm::vec3> mAlteredPositions{};

    std::shared_ptr<OGLMesh> mSkeletonMesh = nullptr;
    std::shared_ptr<GltfNode> mRootNode = nullptr;

    std::vector<std::shared_ptr<GltfNode>> mNodeList;

    std::vector<std::shared_ptr<GltfAnimationClip>> mAnimClips{};

    std::unique_ptr<tinygltf::Model> mModel = nullptr;
    std::unique_ptr<Texture> mTex = nullptr;

    GLuint mVAO = 0;
    std::vector<GLuint> mVertexVBO{};
    GLuint mIndexVBO = 0;

    // for matching the glTF model’s primitive field and the vertex attribute position
    // The order matches the input variables in the shader too
    std::map<std::string, GLint> Attributes =
    {{"POSITION", 0}, {"NORMAL", 1}, {"TEXCOORD_0", 2}, {"JOINTS_0", 3}, {"WEIGHTS_0", 4}};
};

#endif //CPPANIMPROGRAMMING_GLTFMODEL_H
