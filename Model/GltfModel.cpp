// Implementing TinyGltf and making it skip its stb implementations (Texture class is using them too!)
//#define TINYGLTF_IMPLEMENTATION
//#define TINYGLTF_NO_STB_IMAGE
//#define TINYGLTF_NO_STB_IMAGE_WRITE

#include "GltfModel.h"

#include <chrono>
#include <cmath>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "GltfAnimationClip.h"
#include "GltfNode.h"
#include "IKSolver.h"
#include "../Render/OpenGL/Texture.h"
#include "../Tools/Logger.h"

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

GltfModel::GltfModel()
{
    mTex = std::make_unique<Texture>();
}

GltfModel::~GltfModel() = default;

bool GltfModel::LoadModel(OGLRenderData &RenderData, std::string ModelFilename, std::string TextureFilename)
{
    if (!mTex->LoadTexture(TextureFilename, false))
    {
        return false;
    }
    Logger::Log(1, "%s: glTF model texture '%s' successfully loaded\n", __FUNCTION__, ModelFilename.c_str());

    mModel = std::make_unique<tinygltf::Model>();

    tinygltf::TinyGLTF gltfLoader;
    std::string LoaderErrors;
    std::string LoaderWarnings;
    bool bResult = gltfLoader.LoadASCIIFromFile(mModel.get(),&LoaderErrors, &LoaderWarnings, ModelFilename);

    if (!LoaderWarnings.empty())
    {
        Logger::Log(1, "%s: warnings while loading glTF model:\n%s\n", __FUNCTION__, LoaderWarnings.c_str());
    }

    if (!LoaderErrors.empty())
    {
        Logger::Log(1, "%s: errors while loading glTF model:\n%s\n", __FUNCTION__, LoaderErrors.c_str());
    }

    if (!bResult)
    {
        Logger::Log(1, "%s error: could not load file '%s'\n", __FUNCTION__, ModelFilename.c_str());
        return false;
    }

    mModelFilename = ModelFilename;

    glGenVertexArrays(1, &mVAO);
    glBindVertexArray(mVAO);

    /* extract position, normal, texture coords, and indices */
    CreateVertexBuffers();
    CreateIndexBuffer();

    glBindVertexArray(0);

    /* extract joints, weights, and invers bind matrices*/
    GetJointData();
    GetWeightData();
    GetInvBindMatrices();

    mNodeCount = mModel->nodes.size();

    /* extract animation data */
    GetAnimations();

    return true;
}

void GltfModel::Draw()
{
    const tinygltf::Primitive &Primitives = mModel->meshes.at(0).primitives.at(0);
    const tinygltf::Accessor &IndexAccessor = mModel->accessors.at(Primitives.indices);

    GLuint DrawMode = GL_TRIANGLES;
    switch (Primitives.mode)
    {
        case TINYGLTF_MODE_TRIANGLES:
            DrawMode = GL_TRIANGLES;
            break;
        default:
            Logger::Log(1, "%s error: unknown draw mode %i\n", __FUNCTION__, Primitives.mode);
            break;
    }

    mTex->Bind();
    // vertex array object contains the vertex buffers and the index buffer, so no need to bind them separately
    glBindVertexArray(mVAO);

    // using glDrawElements() instead of glDrawArrays() as we have indexed geometry in the model
    // componentType is defined with the same internal value as in OpenGL
    glDrawElements(DrawMode, IndexAccessor.count, IndexAccessor.componentType, nullptr);

    glBindVertexArray(0);
    mTex->Unbind();
}

void GltfModel::Cleanup()
{
    glDeleteBuffers(mVertexVBO.size(), mVertexVBO.data());
    glDeleteBuffers(1, &mVAO);
    glDeleteBuffers(1, &mIndexVBO);
    mTex->Cleanup();
    mTex.reset();
    mModel.reset();
}

void GltfModel::UploadVertexBuffers()
{
    if (mModel == nullptr)
    {
        Logger::Log(1, "%s: Invalid Model pointer\n", __FUNCTION__);
        return;
    }

    // Accessor 0 points to the buffer with the vertex position data
    // Accessor 1 points to the normal data
    // Accessor 2 points to the texture coordinates
    // 3 to joints
    // 4 to weights
    for (int i = 0; i < 5; ++i)
    {
        const tinygltf::Accessor& Accessor = mModel->accessors.at(mAttribAccessors.at(i));
        const tinygltf::BufferView& BufferView = mModel->bufferViews[Accessor.bufferView];
        const tinygltf::Buffer& Buffer = mModel->buffers[BufferView.buffer];

        /* The vertex buffer with the current indices is bound, and by using the byteLength and the
        byteOffset values of the bufferView variable, the corresponding part of the data in the
        gltf buffer is copied to the GPU */
        glBindBuffer(GL_ARRAY_BUFFER, mVertexVBO[i]);
        glBufferData(GL_ARRAY_BUFFER, BufferView.byteLength, &Buffer.data.at(0) + BufferView.byteOffset,GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void GltfModel::UploadIndexBuffer()
{
    if (mModel == nullptr)
    {
        Logger::Log(1, "%s: Invalid Model pointer\n", __FUNCTION__);
        return;
    }

    /* buffer for vertex indices */
    const tinygltf::Primitive& Primitives = mModel->meshes.at(0).primitives.at(0);
    const tinygltf::Accessor& IndexAccessor = mModel->accessors.at(Primitives.indices);
    const tinygltf::BufferView& IndexBufferView = mModel->bufferViews.at(IndexAccessor.bufferView);
    const tinygltf::Buffer& IndexBuffer = mModel->buffers.at(IndexBufferView.buffer);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexVBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, IndexBufferView.byteLength,&IndexBuffer.data.at(0) + IndexBufferView.byteOffset, GL_STATIC_DRAW);
}

void GltfModel::CreateVertexBuffers()
{
    if (mModel == nullptr)
    {
        Logger::Log(1, "%s: Invalid Model pointer\n", __FUNCTION__);
        return;
    }

    const tinygltf::Primitive &Primitives = mModel->meshes.at(0).primitives.at(0);
    mVertexVBO.resize(Primitives.attributes.size());
    mAttribAccessors.resize(Primitives.attributes.size());

    for (const auto& Attrib : Primitives.attributes)
    {
        const std::string AttribType = Attrib.first;
        const int AccessorIndex = Attrib.second;

        const tinygltf::Accessor &Accessor = mModel->accessors.at(AccessorIndex);
        const tinygltf::BufferView &BufferView = mModel->bufferViews[Accessor.bufferView];
        const tinygltf::Buffer &Buffer = mModel->buffers[BufferView.buffer];

        if ((AttribType.compare("POSITION") != 0) && (AttribType.compare("NORMAL") != 0)
        && (AttribType.compare("TEXCOORD_0") != 0) && (AttribType.compare("JOINTS_0") != 0
        && (AttribType.compare("WEIGHTS_0") != 0)))
        {
            Logger::Log(1, "%s: skipping attribute type %s\n", __FUNCTION__, AttribType.c_str());
            continue;
        }

        Logger::Log(1, "%s: data for %s uses accessor %i\n", __FUNCTION__, AttribType.c_str(), AccessorIndex);
        if (AttribType.compare("POSITION") == 0)
        {
            int NumPositionEntries = Accessor.count;
            mAlteredPositions.resize(NumPositionEntries);
            Logger::Log(1, "%s: loaded %i vertices from glTF file\n", __FUNCTION__, NumPositionEntries);
        }

        mAttribAccessors.at(Attributes.at(AttribType)) = AccessorIndex;

        //  ensuring the correct number of elements for the OpenGL vertex buffers from the accessors data types
        int DataSize = 1;
        switch(Accessor.type) {
            case TINYGLTF_TYPE_SCALAR:
                DataSize = 1;
                break;
            case TINYGLTF_TYPE_VEC2:
                DataSize = 2;
                break;
            case TINYGLTF_TYPE_VEC3:
                DataSize = 3;
                break;
            case TINYGLTF_TYPE_VEC4:
                DataSize = 4;
                break;
            default:
                Logger::Log(1, "%s error: accessor %i uses data size %i\n", __FUNCTION__, AccessorIndex, Accessor.type);
                break;
        }

        GLuint DataType = GL_FLOAT;
        switch(Accessor.componentType)
        {
            case TINYGLTF_COMPONENT_TYPE_FLOAT:
                DataType = GL_FLOAT;
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                DataType = GL_UNSIGNED_SHORT;
                break;
            default:
                Logger::Log(1, "%s error: accessor %i uses unknown data type %i\n", __FUNCTION__, AccessorIndex, Accessor.componentType);
                break;
        }//

        /* buffers for position, normal and tex coordinates */
        glGenBuffers(1, &mVertexVBO.at(Attributes.at(AttribType)));
        glBindBuffer(GL_ARRAY_BUFFER, mVertexVBO.at(Attributes.at(AttribType)));

        glVertexAttribPointer(Attributes.at(AttribType), DataSize, DataType, GL_FALSE,0, (void*) 0);
        glEnableVertexAttribArray(Attributes.at(AttribType));

        glBindBuffer(GL_ARRAY_BUFFER, 0);

    }
}

void GltfModel::CreateIndexBuffer()
{
    glGenBuffers(1, &mIndexVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexVBO);

    /* Do NOT UNBIND the element buffer here. The index buffer must be in the bound state;
     unbinding it will lead to a crash during the Draw() call. */
}

int GltfModel::GetTriangleCount() const
{
    if (mModel == nullptr)
    {
        Logger::Log(1, "%s: Invalid Model pointer\n", __FUNCTION__);
        return 0;
    }

    const tinygltf::Primitive &Primitives = mModel->meshes.at(0).primitives.at(0);
    const tinygltf::Accessor &IndicesAccessor = mModel->accessors.at(Primitives.indices);

    unsigned int Triangles = 0;
    switch (Primitives.mode)
    {
        case TINYGLTF_MODE_TRIANGLES:
            Triangles = IndicesAccessor.count / 3;
            break;
        default:
            Logger::Log(1, "%s error: unknown draw mode %i\n", __FUNCTION__, Primitives.mode);
            break;
    }
    return Triangles;
}

int GltfModel::GetNodeCount() const
{
    return mNodeCount;
}

void GltfModel::GetGltfNodes(GltfNodeData &OutNodeData)
{
    if (mModel == nullptr || mModel->scenes.empty() || mModel->scenes.at(0).nodes.empty())
    {
        Logger::Log(1, "%s: Couldn't get GltfNodes\n", __FUNCTION__);
        return;
    }

    GltfNodeData NodeData{};

    const int RootNodeIndex = mModel->scenes.at(0).nodes.at(0);
    Logger::Log(2, "%s: model has %i nodes, root node is %i\n", __FUNCTION__,
      mNodeCount, RootNodeIndex);

    NodeData.RootNode = GltfNode::CreateRoot(RootNodeIndex);

    GetNodeData(NodeData.RootNode);
    GetNodes(NodeData.RootNode);

    NodeData.NodeList.resize(mNodeCount);
    NodeData.NodeList.at(RootNodeIndex) = NodeData.RootNode;
    GetNodeList(NodeData.NodeList, RootNodeIndex);

    OutNodeData = NodeData;
}

void GltfModel::GetNodeList(std::vector<std::shared_ptr<GltfNode>> &InOutNodeList, int NodeIndex)
{
    if (InOutNodeList.empty() || NodeIndex < 0 || NodeIndex >= InOutNodeList.size())
    {
        Logger::Log(1, "%s: Couldn't get NodeList\n", __FUNCTION__);
        return;
    }

    std::vector<std::shared_ptr<GltfNode>> ChildrenNodes;
    InOutNodeList.at(NodeIndex)->GetChildren(ChildrenNodes);
    for (std::shared_ptr<GltfNode>& ChildNode : ChildrenNodes)
    {
        int ChildNodeIndex = ChildNode->GetNodeIndex();
        InOutNodeList.at(ChildNodeIndex) = ChildNode;
        GetNodeList(InOutNodeList, ChildNodeIndex);
    }
}

void GltfModel::GetInverseBindMatrices(std::vector<glm::mat4> &OutMatrices)
{
    OutMatrices = mInverseBindMatrices;
}

void GltfModel::GetNodeToJoint(std::vector<int> &OutNodeToJoint)
{
    OutNodeToJoint = mNodeToJoint;
}

void GltfModel::GetAnimClips(std::vector<std::shared_ptr<GltfAnimationClip>> &OutAnimClips)
{
    OutAnimClips = mAnimClips;
}

void GltfModel::GetJointData()
{
    if (mModel == nullptr)
    {
        return;
    }

    std::string JointsAccessorAttrib = "JOINTS_0";
    const int JointsAccessor = mModel->meshes.at(0).primitives.at(0).attributes.at(JointsAccessorAttrib);
    Logger::Log(1, "%s: using accessor %i to get %s\n", __FUNCTION__, JointsAccessor, JointsAccessorAttrib.c_str());

    const tinygltf::Accessor &Accessor = mModel->accessors.at(JointsAccessor);
    const tinygltf::BufferView &BufferView = mModel->bufferViews.at(Accessor.bufferView);
    const tinygltf::Buffer &Buffer = mModel->buffers.at(BufferView.buffer);

    const int JointVecSize = Accessor.count;
    Logger::Log(1, "%s: %i short vec4 in JOINTS_0\n", __FUNCTION__, JointVecSize);
    mJointVec.resize(JointVecSize);

    std::memcpy(mJointVec.data(), &Buffer.data.at(0) + BufferView.byteOffset, BufferView.byteLength);

    mNodeToJoint.resize(mModel->nodes.size());
    const tinygltf::Skin &Skin = mModel->skins.at(0);
    const int JointsCount = Skin.joints.size();
    for (int i = 0; i < JointsCount; i++)
    {
        const int DestinationNodeIndex = Skin.joints.at(i);
        mNodeToJoint.at(DestinationNodeIndex) = i;
        Logger::Log(2, "%s: joint %i affects node %i\n", __FUNCTION__, i, DestinationNodeIndex);
    }
}

void GltfModel::GetWeightData()
{
    if (mModel == nullptr)
    {
        return;
    }

    std::string WeightsAccessorAttrib = "WEIGHTS_0";
    const int WeightsAccessor = mModel->meshes.at(0).primitives.at(0).attributes.at(WeightsAccessorAttrib);
    Logger::Log(1, "%s: using accessor %i to get %s\n", __FUNCTION__, WeightsAccessor, WeightsAccessorAttrib.c_str());

    const tinygltf::Accessor &Accessor = mModel->accessors.at(WeightsAccessor);
    const tinygltf::BufferView &BufferView = mModel->bufferViews.at(Accessor.bufferView);
    const tinygltf::Buffer &Buffer = mModel->buffers.at(BufferView.buffer);

    const int WeightsVecSize = Accessor.count;
    Logger::Log(1, "%s: %i vec4 in WEIGHTS_0\n", __FUNCTION__, WeightsVecSize);
    mWeightVec.resize(WeightsVecSize);

    std::memcpy(mWeightVec.data(), &Buffer.data.at(0) + BufferView.byteOffset, BufferView.byteLength);
}

void GltfModel::GetInvBindMatrices()
{
    if (mModel == nullptr)
    {
        return;
    }

    const tinygltf::Skin &Skin = mModel->skins.at(0);
    const int InvBindMatAccessor = Skin.inverseBindMatrices;

    const tinygltf::Accessor &Accessor = mModel->accessors.at(InvBindMatAccessor);
    const tinygltf::BufferView &BufferView = mModel->bufferViews.at(Accessor.bufferView);
    const tinygltf::Buffer &Buffer = mModel->buffers.at(BufferView.buffer);

    mInverseBindMatrices.resize(Skin.joints.size());

    mJointCPUDualQuats.resize(Skin.joints.size());

    std::memcpy(mInverseBindMatrices.data(), &Buffer.data.at(0) + BufferView.byteOffset, BufferView.byteLength);
}

void GltfModel::GetNodes(std::shared_ptr<GltfNode>& TreeNode)
{
    if (TreeNode == nullptr || mModel == nullptr)
    {
        return;
    }

    const int NodeIndex = TreeNode->GetNodeIndex();
    std::vector<int> Children = mModel->nodes.at(NodeIndex).children;

    /* remove the child node with skin/mesh metadata, confuses skeleton */
    auto RemoveIter = std::remove_if(Children.begin(), Children.end(),
   [&](int Index) { return mModel->nodes.at(Index).skin != -1; });
    Children.erase(RemoveIter, Children.end());

    TreeNode->AddChildren(Children);

    glm::mat4 TreeNodeMatrix;
    TreeNode->GetNodeMatrix(TreeNodeMatrix);

    std::vector<std::shared_ptr<GltfNode>> ChildrenNodes;
    TreeNode->GetChildren(ChildrenNodes);

    for (std::shared_ptr<GltfNode>& ChildNode : ChildrenNodes)
    {
        GetNodeData(ChildNode);
        GetNodes(ChildNode);
    }
}

void GltfModel::GetNodeData(std::shared_ptr<GltfNode>& TreeNode)
{
    if (TreeNode == nullptr || mModel == nullptr)
    {
        return;
    }

    int NodeIndex = TreeNode->GetNodeIndex();
    const tinygltf::Node &Node = mModel->nodes.at(NodeIndex);
    TreeNode->SetNodeName(Node.name);

    if (!Node.translation.empty())
    {
        TreeNode->SetTranslation(glm::make_vec3(Node.translation.data()));
    }
    if (!Node.rotation.empty())
    {
        TreeNode->SetRotation(glm::make_quat(Node.rotation.data()));
    }
    if (!Node.scale.empty())
    {
        TreeNode->SetScale(glm::make_vec3(Node.scale.data()));
    }

    TreeNode->CalculateNodeMatrix();
}

void GltfModel::ResetNodeData(std::shared_ptr<GltfNode>& InOutTreeNode)
{
    if (InOutTreeNode == nullptr)
    {
        return;
    }

    GetNodeData(InOutTreeNode);

    glm::mat4 TreeNodeMatrix;
    InOutTreeNode->GetNodeMatrix(TreeNodeMatrix);
    std::vector<std::shared_ptr<GltfNode>> ChildrenNodes;
    InOutTreeNode->GetChildren(ChildrenNodes);
    for (std::shared_ptr<GltfNode>& ChildNode : ChildrenNodes)
    {
        ResetNodeData(ChildNode);
    }
}

void GltfModel::GetModelFilename(std::string &OutModelName)
{
    OutModelName = mModelFilename;
}

void GltfModel::GetAnimations()
{
    if (mModel == nullptr || mModel->animations.empty())
    {
        return;
    }

    for (const auto &Anim : mModel->animations)
    {
        Logger::Log(1, "%s: loading animation '%s' with %i channels\n", __FUNCTION__, Anim.name.c_str(), Anim.channels.size());
        std::shared_ptr<GltfAnimationClip> Clip = std::make_shared<GltfAnimationClip>(Anim.name);
        for (const auto& Channel : Anim.channels)
        {
            Clip->AddChannel(mModel, Anim, Channel);
        }
        mAnimClips.push_back(Clip);
    }
}
