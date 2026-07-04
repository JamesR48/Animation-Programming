#version 460 core

/*
    The internal name for the incoming data element must match the name given to the output
    element of the previous shader stage. Here, the output name (texCoord) from the vertex
    shader must match the input name in the fragment shader. If the names do not match, the
    shader compiling will fail!
*/
layout (location = 0) in vec4 texColor;
layout (location = 1) in vec2 texCoord;

out vec4 FragColor;

/* uniform marks the parameter as non-changing for all parallel invocations of the shader
 during a draw call. */
uniform sampler2D Tex;

float toSRGB(float x) {
    if (x <= 0.0031308)
    return 12.92 * x;
    else
    return 1.055 * pow(x, (1.0/2.4)) - 0.055;
}
vec3 sRGB(vec3 c) {
    return vec3(toSRGB(c.x), toSRGB(c.y), toSRGB(c.z));
}

void main()
{
    FragColor = texture(Tex, texCoord) * texColor;
    FragColor.rgb = sRGB(FragColor.rgb);
}