#version 330 core

layout (location = 0) in vec2 aPos;      // Position attribute
layout (location = 1) in vec2 aTexCoord; // Texture coordinate attribute

out vec2 TexCoord;

void main()
{
    // Set the vertex position in clip space
    gl_Position = vec4(aPos.xy, 0.0, 1.0);

    // Pass the texture coordinates to the fragment shader
    TexCoord = aTexCoord;
}