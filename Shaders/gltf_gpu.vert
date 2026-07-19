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

uniform int aModelStride;

// for Texture Buffer Objects (TBOs)
layout (binding = 1) uniform samplerBuffer JointMatrices;

mat4 GetMatrix(int Offset)
{
    return mat4(texelFetch(JointMatrices, Offset), texelFetch(JointMatrices, Offset + 1),
            texelFetch(JointMatrices, Offset + 2), texelFetch(JointMatrices, Offset + 3));
}

void main()
{
    // advancing to the correct position in the buffer
    const int StridePerInstance = gl_InstanceID * aModelStride;

    /* multiplying by 4 as the TBO contains float values instead of the mat4 values
     from the SSBO and the goal is reaching the same data as before */
    mat4 SkinMat =
    aJointWeight.x * GetMatrix((int(aJointIndices.x) + StridePerInstance) * 4) +
    aJointWeight.y * GetMatrix((int(aJointIndices.y) + StridePerInstance) * 4) +
    aJointWeight.z * GetMatrix((int(aJointIndices.z) + StridePerInstance) * 4) +
    aJointWeight.w * GetMatrix((int(aJointIndices.w) + StridePerInstance) * 4);

    gl_Position = Projection * View * SkinMat * vec4(aPos, 1.0);
    Normal = vec3(transpose(inverse(SkinMat)) * vec4(aNormal, 1.0));
    TexCoord = aTexCoord;
}