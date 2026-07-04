#version 460 core

// Input of the shader (the vertex buffers in our vertex array object)
// The two location definitions in the shader must match the vertex buffer definition in the
// Vertex buffers and vertex arrays section to produce correct results
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

layout (location = 0) out vec4 texColor;
layout (location = 1) out vec2 texCoord;

/* The std140 keyword defines the memory layout inside the shader, while the binding keyword is
used with an index number, in case multiple uniform buffers are used */
layout (std140, binding = 0) uniform Matrices
{
    mat4 View;
    mat4 Projection;
};

void main()
{
    gl_Position = Projection * View * vec4(aPos, 1.0);
    texColor = vec4(aColor, 1.0);
    texCoord = aTexCoord;
}