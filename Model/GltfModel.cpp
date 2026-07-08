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

    glGenVertexArrays(1, &mVAO);
    glBindVertexArray(mVAO);

    /* extract position, normal, texture coords, and indices */
    CreateVertexBuffers();
    CreateIndexBuffer();

    glBindVertexArray(0);

    RenderData.rdGltfTriangleCount = GetTriangleCount();

    /* extract joints, weights, and invers bind matrices*/
    GetJointData();
    GetWeightData();
    GetInvBindMatrices();

    /* build model tree */
    RenderData.rdModelNodeCount = mModel->nodes.size();
    const int RootNodeIndex = mModel->scenes.at(0).nodes.at(0);
    mRootNode = GltfNode::CreateRoot(RootNodeIndex);
    Logger::Log(1, "%s: model has %i nodes, root node is %i\n", __FUNCTION__, RenderData.rdModelNodeCount, RootNodeIndex);

    mNodeList.resize(RenderData.rdModelNodeCount);
    mNodeList.at(RootNodeIndex) = mRootNode;

    GetNodeData(mRootNode, glm::mat4(1.0f));
    GetNodes(mRootNode);
    mRootNode->PrintTree();

    mSkeletonMesh = std::make_shared<OGLMesh>();

    /* extract animation data */
    GetAnimations();
    RenderData.rdAnimClipSize = mAnimClips.size();

    // initializing with all nodes valid
    mAdditiveAnimationMask.set();

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

void GltfModel::ApplyCPUVertexSkinning(bool bEnableDualQuats)
{
    if (mModel == nullptr)
    {
        Logger::Log(1, "%s: Invalid Model pointer\n", __FUNCTION__);
        return;
    }

    const tinygltf::Accessor& Accessor = mModel->accessors.at(mAttribAccessors.at(0));
    const tinygltf::BufferView& BufferView = mModel->bufferViews.at(Accessor.bufferView);
    const tinygltf::Buffer& Buffer = mModel->buffers.at(BufferView.buffer);

    std::memcpy(mAlteredPositions.data(),&Buffer.data.at(0) + BufferView.byteOffset, BufferView.byteLength);

    const int NumJoints = mJointVec.size();
    for (int i = 0; i < NumJoints; ++i)
    {
        glm::ivec4 JointIndex = glm::make_vec4(mJointVec.at(i));
        glm::vec4 WeightIndex = glm::make_vec4(mWeightVec.at(i));

        glm::mat4 SkinMat;

        if (bEnableDualQuats)
        {
            // extract dual quaterions
            glm::dualquat dq0 = mJointCPUDualQuats.at(JointIndex.x);
            glm::dualquat dq1 = mJointCPUDualQuats.at(JointIndex.y);
            glm::dualquat dq2 = mJointCPUDualQuats.at(JointIndex.z);
            glm::dualquat dq3 = mJointCPUDualQuats.at(JointIndex.w);

            // shortest rotation
            WeightIndex.y *= glm::sign(glm::dot(dq0.real, dq1.real));
            WeightIndex.z *= glm::sign(glm::dot(dq0.real, dq2.real));
            WeightIndex.w *= glm::sign(glm::dot(dq0.real, dq3.real));

            // blending, interpolation between the four dual quaternions
            glm::dualquat BlendedDQ =
                WeightIndex.x * dq0 +
                WeightIndex.y * dq1 +
                WeightIndex.z * dq2 +
                WeightIndex.w * dq3;

            BlendedDQ = glm::normalize(BlendedDQ);
            glm::quat r = BlendedDQ.real;
            glm::quat t = BlendedDQ.dual;

            SkinMat = glm::mat4(
                1.0 - (2.0 * r.y * r.y) - (2.0 * r.z * r.z),
                (2.0 * r.x * r.y) + (2.0 * r.w * r.z),
                (2.0 * r.x * r.z) - (2.0 * r.w * r.y),
                0.0,

                (2.0 * r.x * r.y) - (2.0 * r.w * r.z),
                1.0 - (2.0 * r.x * r.x) - (2.0 * r.z * r.z),
                (2.0 * r.y * r.z) + (2.0 * r.w * r.x),
                0.0,

                (2.0 * r.x * r.z) + (2.0 * r.w * r.y),
                (2.0 * r.y * r.z) - (2.0 * r.w * r.x),
                1.0 - (2.0 * r.x * r.x) - (2.0 * r.y * r.y),
                0.0,

                2.0 * (-t.w * r.x + t.x * r.w - t.y * r.z + t.z * r.y),
                2.0 * (-t.w * r.y + t.x * r.z + t.y * r.w - t.z * r.x),
                2.0 * (-t.w * r.z - t.x * r.y + t.y * r.x + t.z * r.w),
                1);
        }
        else
        {
            // vertex skinning matrix
            SkinMat =
            WeightIndex.x * mJointMatrices.at(JointIndex.x) +
            WeightIndex.y * mJointMatrices.at(JointIndex.y) +
            WeightIndex.z * mJointMatrices.at(JointIndex.z) +
            WeightIndex.w * mJointMatrices.at(JointIndex.w);
        }

        mAlteredPositions.at(i) = SkinMat * glm::vec4(mAlteredPositions.at(i), 1.0f);
    }

    glBindBuffer(GL_ARRAY_BUFFER, mVertexVBO.at(0));
    glBufferData(GL_ARRAY_BUFFER, BufferView.byteLength, mAlteredPositions.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

std::shared_ptr<OGLMesh> GltfModel::GetSkeleton()
{
    if (mSkeletonMesh == nullptr)
    {
        return nullptr;
    }

    mSkeletonMesh->Vertices.resize(mModel->nodes.size() * 2);
    mSkeletonMesh->Vertices.clear();

    /* start from Armature child */
    std::vector<std::shared_ptr<GltfNode>> ChildrenNodes;
    mRootNode->GetChildren(ChildrenNodes);
    GetSkeletonPerNode(ChildrenNodes.at(0));
    return mSkeletonMesh;
}

int GltfModel::GetJointMatrixSize() const
{
    return mJointMatrices.size();
}

void GltfModel::GetJointMatrices(std::vector<glm::mat4> &OutJointMatrices)
{
    OutJointMatrices = mJointMatrices;
}

int GltfModel::GetJointDualQuatsSize() const
{
    return mJointDualQuats.size();
}

void GltfModel::GetJointDualQuats(std::vector<glm::mat2x4> &OutJointDualQuats)
{
    OutJointDualQuats = mJointDualQuats;
}

void GltfModel::PlayAnimation(const int SourceAnimIndex, const float BlendFactor, const int DestAnimIndex, const float PlaybackSpeed, const bool bPlayBackwards)
{
    if (mAnimClips.empty() || (SourceAnimIndex < 0) || (SourceAnimIndex >= mAnimClips.size()))
    {
        Logger::Log(1, "%s: no valid animations to play\n", __FUNCTION__);
        return;
    }

    const float ClipEndTime = mAnimClips.at(SourceAnimIndex)->GetClipEndTime();
    double CurrentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    double AnimTime = std::fmod((CurrentTime / 1000.0) * PlaybackSpeed, ClipEndTime);
    if (bPlayBackwards)
    {
        AnimTime = ClipEndTime - AnimTime;
    }
    BlendAnimationFrame(SourceAnimIndex, AnimTime, BlendFactor, DestAnimIndex);
}

void GltfModel::BlendAnimationFrame(const int SourceAnimIndex, const float Time, const float BlendFactor, const int DestAnimIndex)
{
    if (mAnimClips.empty() || (SourceAnimIndex < 0) || (SourceAnimIndex >= mAnimClips.size()))
    {
        Logger::Log(1, "%s: no valid animations\n", __FUNCTION__);
        return;
    }

    if (DestAnimIndex > -1)
    {
        const float SourceAnimDuration = mAnimClips.at(SourceAnimIndex)->GetClipEndTime();
        const float DestAnimDuration = mAnimClips.at(DestAnimIndex)->GetClipEndTime();

        /* TODO: This kinda sync-blending method works good for (some) locomotion anims...but not so well
            when transitioning from locomotion to unrelated states (sitting, picking up...)...maybe tracking
            two separate times (one for source, other for dest) and swapping them in this cases could work...
         */

        /* equalizing the clip lengths so the shorter animation clip won't end suddenly,
         resulting in a possible gap in the model movement */
        const float ScaledTime = Time * (DestAnimDuration / SourceAnimDuration);

        // setting the nodes baseline pose entirely based on the source anim at the current time
        mAnimClips.at(SourceAnimIndex)->SetAnimationFrame(mNodeList, mAdditiveAnimationMask, Time);
        // blending between the destination anim and the established pose from the source anim
        mAnimClips.at(DestAnimIndex)->BlendAnimationFrame(mNodeList, mAdditiveAnimationMask, ScaledTime, BlendFactor);

        const std::bitset<MAX_GLTF_NODES> InvertedAdditiveMask = ~mAdditiveAnimationMask;
        mAnimClips.at(DestAnimIndex)->SetAnimationFrame(mNodeList, InvertedAdditiveMask, ScaledTime);
        mAnimClips.at(SourceAnimIndex)->BlendAnimationFrame(mNodeList, InvertedAdditiveMask, Time, BlendFactor);
    }
    else
    {
        mAnimClips.at(SourceAnimIndex)->BlendAnimationFrame(mNodeList, mAdditiveAnimationMask, Time, BlendFactor);
    }

    UpdateNodeMatrices(mRootNode, glm::mat4(1.0f));
}

float GltfModel::GetAnimationEndTime(const int AnimIndex) const
{
    if (mAnimClips.empty() || (AnimIndex < 0) || (AnimIndex >= mAnimClips.size()))
    {
        Logger::Log(1, "%s: no valid animations\n", __FUNCTION__);
        return 0.0f;
    }

    return mAnimClips.at(AnimIndex)->GetClipEndTime();
}

void GltfModel::GetClipName(const int AnimIndex, std::string &Name)
{
    if (mAnimClips.empty() || (AnimIndex < 0) || (AnimIndex >= mAnimClips.size()))
    {
        Logger::Log(1, "%s: no valid animations\n", __FUNCTION__);
        return;
    }

    Name = mAnimClips.at(AnimIndex)->GetClipName();
}

void GltfModel::ResetNodeData()
{
    GetNodeData(mRootNode, glm::mat4(1.0f));
    ResetNodeData(mRootNode, glm::mat4(1.0f));
}

void GltfModel::SetSkeletonSplitNode(const int NodeIndex)
{
    if (NodeIndex >= MAX_GLTF_NODES)
    {
        return;
    }

    mAdditiveAnimationMask.set();
    UpdateAdditiveMask(mRootNode, NodeIndex);
}

void GltfModel::GetNodeName(const int NodeIndex, std::string &OutNodeName)
{
    if ((NodeIndex < 0) || (NodeIndex >= mNodeList.size()) || (mNodeList.at(NodeIndex) == nullptr))
    {
        OutNodeName = "INVALID";
        return;
    }

    mNodeList.at(NodeIndex)->GetNodeName(OutNodeName);
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

void GltfModel::GetSkeletonPerNode(std::shared_ptr<GltfNode> TreeNode)
{
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
    mJointMatrices.resize(Skin.joints.size());
    mJointDualQuats.resize(Skin.joints.size());

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

    for (auto &ChildNode : ChildrenNodes)
    {
        mNodeList.at(ChildNode->GetNodeIndex()) = ChildNode;
        GetNodeData(ChildNode, TreeNodeMatrix);
        GetNodes(ChildNode);
    }
}

void GltfModel::GetNodeData(std::shared_ptr<GltfNode>& TreeNode, const glm::mat4 &ParentNodeMatrix)
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

    TreeNode->CalculateLocalTRSMatrix();
    TreeNode->CalculateNodeMatrix(ParentNodeMatrix);

    UpdateJointMatricesAndQuats(TreeNode);
}

void GltfModel::ResetNodeData(const std::shared_ptr<GltfNode>& TreeNode, const glm::mat4& ParentNodeMatrix)
{
    if (TreeNode == nullptr)
    {
        return;
    }

    glm::mat4 TreeNodeMatrix;
    TreeNode->GetNodeMatrix(TreeNodeMatrix);
    std::vector<std::shared_ptr<GltfNode>> ChildrenNodes;
    TreeNode->GetChildren(ChildrenNodes);
    for (std::shared_ptr<GltfNode>& ChildNode : ChildrenNodes)
    {
        GetNodeData(ChildNode, TreeNodeMatrix);
        ResetNodeData(ChildNode, TreeNodeMatrix);
    }
}

void GltfModel::UpdateNodeMatrices(std::shared_ptr<GltfNode>& TreeNode, const glm::mat4& ParentNodeMatrix)
{
    if (TreeNode == nullptr)
    {
        return;
    }

    TreeNode->CalculateNodeMatrix(ParentNodeMatrix);
    UpdateJointMatricesAndQuats(TreeNode);

    glm::mat4 TreeNodeMatrix;
    TreeNode->GetNodeMatrix(TreeNodeMatrix);

    std::vector<std::shared_ptr<GltfNode>> ChildrenNodes;
    TreeNode->GetChildren(ChildrenNodes);
    for (auto& ChildNode : ChildrenNodes)
    {
        UpdateNodeMatrices(ChildNode, TreeNodeMatrix);
    }

}

void GltfModel::UpdateJointMatricesAndQuats(std::shared_ptr<GltfNode>& TreeNode)
{
    if (TreeNode == nullptr || mInverseBindMatrices.empty() || mNodeToJoint.empty() || mJointMatrices.empty())
    {
        return;
    }

    glm::mat4 TreeNodeMatrix;
    TreeNode->GetNodeMatrix(TreeNodeMatrix);

    /* multiply the node matrix and the inverse bind matrix to create the final transformation matrix
    for the positional change of every vertex of a node appearing in the joints array */
    const int NodeIndex = TreeNode->GetNodeIndex();
    mJointMatrices.at(mNodeToJoint.at(NodeIndex)) = TreeNodeMatrix * mInverseBindMatrices.at(mNodeToJoint.at(NodeIndex));

    /* extract components from node matrix */
    glm::quat Orientation;
    glm::vec3 Scale;
    glm::vec3 Translation;
    glm::vec3 Skew;
    glm::vec4 Perspective;
    glm::dualquat DualQuat;

    /* create dual quaternion */
    if (glm::decompose(mJointMatrices.at(mNodeToJoint.at(NodeIndex)), Scale, Orientation,Translation, Skew, Perspective))
    {
        // qTranslation = 1/2 * t * qRotation
        DualQuat[0] = Orientation;
        DualQuat[1] = glm::quat(0.0, Translation.x, Translation.y, Translation.z) * Orientation * 0.5f;
        mJointDualQuats.at(mNodeToJoint.at(NodeIndex)) = glm::mat2x4_cast(DualQuat);

        mJointCPUDualQuats.at(mNodeToJoint.at(NodeIndex)) = DualQuat;
    }
    else
    {
        Logger::Log(1, "%s error: could not decompose matrix for node %i\n", __FUNCTION__, NodeIndex);
    }
}

void GltfModel::UpdateAdditiveMask(const std::shared_ptr<GltfNode> &TreeNode, const int SplitNodeIndex)
{
    if (TreeNode == nullptr)
    {
        return;
    }

    const int NodeIndex = TreeNode->GetNodeIndex();
    if((NodeIndex >= MAX_GLTF_NODES) || (NodeIndex == SplitNodeIndex))
    {
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
