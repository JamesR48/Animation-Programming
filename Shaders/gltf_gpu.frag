#version 460 core

layout (location = 0) in vec3 Normal;
layout (location = 1) in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D tex;

vec3 LightPos = vec3(4.0, 3.0, 6.0);
vec3 LightColor = vec3(1.0, 1.0, 1.0);

float ToSRGB(float X)
{
    if (X <= 0.0031308)
    {
        return 12.92 * X;
    }
    else
    {
        return 1.055 * pow(X, (1.0/2.4)) - 0.055;
    }
}

vec3 sRGB(vec3 C)
{
    return vec3(ToSRGB(C.x), ToSRGB(C.y), ToSRGB(C.z));
}

void main()
{
    float LightAngle = max(dot(normalize(Normal), normalize(LightPos)), 0.0);

    /*  0.3 is used to create some ambient light, instead of a complete black color
    if the vertex normal points away from the light. The light angle is also scaled down slightly to avoid
    “overshooting” the color to a value larger than 1.0 when adding the values from the ambient light
    and light angle */
    FragColor = texture(tex, TexCoord) * vec4((0.3 + 0.7 * LightAngle) * LightColor, 1.0);
    //FragColor.rgb = sRGB(FragColor.rgb);
}
