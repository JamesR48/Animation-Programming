#ifndef CPPANIMPROGRAMMING_GLTFMODEL_H
#define CPPANIMPROGRAMMING_GLTFMODEL_H

#include <memory>
#include <string>
#include <vector>
#include <bitset>
#include <map>
#include <glad/gl.h>
#include <glm/fwd.hpp>
#include "../Render/OpenGL/OGLRenderData.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/dual_quaternion.hpp>

namespace tinygltf
{
    class Model;
};

class Texture;
class IKSolver;
class GltfNode;
class GltfAnimationClip;
struct OGLMesh;

struct GltfNodeData
{
    std::shared_ptr<GltfNode> RootNode;
    std::vector<std::shared_ptr<GltfNode>> NodeList;
};

class GltfModel
{
public:

    GltfModel();
    ~GltfModel();

    bool LoadModel(OGLRenderData& RenderData, std::string ModelFilename, std::string TextureFilename);
    /* if InstanceCount > 0, drawing will be performed with glDrawElementsInstanced */
    void Draw(int InstanceCount = 0);
    void Cleanup();
    void UploadVertexBuffers();
    void UploadIndexBuffer();
    void ResetNodeData(std::shared_ptr<GltfNode>& InOutTreeNode);

    void GetModelFilename(std::string& OutModelName);
    int GetTriangleCount() const;
    int GetNodeCount() const;
    void GetGltfNodes(GltfNodeData& OutNodeData);
    void GetNodeList(std::vector<std::shared_ptr<GltfNode>>& InOutNodeList, int NodeIndex);

    void GetInverseBindMatrices(std::vector<glm::mat4>& OutMatrices);
    void GetNodeToJoint(std::vector<int>& OutNodeToJoint);
    void GetAnimClips(std::vector<std::shared_ptr<GltfAnimationClip>>& OutAnimClips);
private:

    void CreateVertexBuffers();
    void CreateIndexBuffer();

    void GetJointData();
    void GetWeightData();
    void GetInvBindMatrices();

    /* reads the children from the corresponding node in the glTF model file and adds the correct number
    of – as yet empty – child nodes. Next, it reads the node matrix from the node given as a parameter
    and calls getNodeData() and getNodes() for every created child, traversing and creating a tree of the nodes. */
    void GetNodes(std::shared_ptr<GltfNode>& TreeNode);

    // sets the node values for translation, rotation, scale. Triggers the calculation of the LocalTRS and Node matrices
    void GetNodeData(std::shared_ptr<GltfNode>& TreeNode);

    void GetAnimations();

    std::vector<glm::tvec4<uint16_t>> mJointVec{};
    std::vector<glm::vec4> mWeightVec{};
    // To calculate the binding pose of the model
    std::vector<glm::mat4> mInverseBindMatrices{};

    std::vector<glm::dualquat> mJointCPUDualQuats{};

    std::vector<int> mAttribAccessors{};
    // look up table of joints per vertex indices
    std::vector<int> mNodeToJoint{};

    std::vector<glm::vec3> mAlteredPositions{};

    std::vector<std::shared_ptr<GltfNode>> mNodeList;

    std::vector<std::shared_ptr<GltfAnimationClip>> mAnimClips{};

    std::unique_ptr<tinygltf::Model> mModel = nullptr;
    std::unique_ptr<Texture> mTex = nullptr;

    std::string mModelFilename;
    int mNodeCount = 0;

    GLuint mVAO = 0;
    std::vector<GLuint> mVertexVBO{};
    GLuint mIndexVBO = 0;

    // for matching the glTF model’s primitive field and the vertex attribute position
    // The order matches the input variables in the shader too
    std::map<std::string, GLint> Attributes =
    {{"POSITION", 0}, {"NORMAL", 1}, {"TEXCOORD_0", 2}, {"JOINTS_0", 3}, {"WEIGHTS_0", 4}};
};

#endif //CPPANIMPROGRAMMING_GLTFMODEL_H
