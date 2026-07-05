#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aJointIndices; // joints that alter the current vertex. Must be casted to int!
layout (location = 4) in vec4 aJointWeight; // weights for every joint

layout (location = 0) out vec3 Normal;
layout (location = 1) out vec2 TexCoord;

layout (std140, binding = 0) uniform Matrices
{
    mat4 View;
    mat4 Projection;
};

// for Shader Storage Buffer Objects (SSBOs)
layout (std430, binding = 1) readonly buffer JointMatrices
{
    mat4 JointMat[];
};

void main()
{
    mat4 SkinMat =
    aJointWeight.x * JointMat[int(aJointIndices.x)] +
    aJointWeight.y * JointMat[int(aJointIndices.y)] +
    aJointWeight.z * JointMat[int(aJointIndices.z)] +
    aJointWeight.w * JointMat[int(aJointIndices.w)];

    gl_Position = Projection * View * SkinMat * vec4(aPos, 1.0);
    Normal = vec3(transpose(inverse(SkinMat)) * vec4(aNormal, 1.0));
    TexCoord = aTexCoord;
}