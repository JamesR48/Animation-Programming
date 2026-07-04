#version 460 core

layout (location = 0) in vec4 texColor;
layout (location = 1) in vec2 texCoord;

out vec4 FragColor;

uniform sampler2D Texture;

void main()
{
    FragColor = texture(Texture, texCoord) * (vec4(1.0) - texColor);
}