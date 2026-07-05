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

layout (std430, binding = 2) readonly buffer JointDualQuats {
    mat2x4 JointDQs[];
};

mat2x4 GetJointTransform(ivec4 joints, vec4 weights) {
    // read dual quaterions from buffer
    mat2x4 dq0 = JointDQs[joints.x];
    mat2x4 dq1 = JointDQs[joints.y];
    mat2x4 dq2 = JointDQs[joints.z];
    mat2x4 dq3 = JointDQs[joints.w];

    // shortest rotation
    weights.y *= sign(dot(dq0[0], dq1[0]));
    weights.z *= sign(dot(dq0[0], dq2[0]));
    weights.w *= sign(dot(dq0[0], dq3[0]));

    // blending, interpolation between the four dual quaternions
    mat2x4 result =
    weights.x * dq0 +
    weights.y * dq1 +
    weights.z * dq2 +
    weights.w * dq3;

    // normalize the dual quaternion
    float norm = length(result[0]);
    return result / norm;
}

mat4 GetSkinMat() {
    mat2x4 bone = GetJointTransform(ivec4(aJointIndices), aJointWeight);

    vec4 r = bone[0]; // rotation
    vec4 t = bone[1]; // translation

    /*
        quat = a + bi + cj + dk; |quat| = 1

                 |  (1 − 2c^2 − 2d^2)    (2bc − 2ad)       (2bd + 2ac)    |
        RotMat = |     (2bc + 2ad)    (1 − 2b^2 − 2d^2)    (2cd − 2ab)    |
                 |     (2bd − 2ac)       (2cd + 2ab)    (1 − 2b^2 − 2c^2) |

        qTranslation = 1/2 * t * qRotation
        ---> t = 2 * q * p
        where t is translation; q is the dual-quat translation; p is the conjugate of the dual-quat rotation

        The transformation matrix is created in the GLSL column-major format
            |                tx |
        T = |     RotMat     ty |
            |                tz |
            |  0    0    0    1 |
    */
    return mat4(
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

void main()
{
    mat4 SkinMat = GetSkinMat();
    gl_Position = Projection * View * SkinMat * vec4(aPos, 1.0);
    Normal = vec3(transpose(inverse(SkinMat)) * vec4(aNormal, 1.0));
    TexCoord = aTexCoord;
}