#version 330 core

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;

out vec3 Normal;

uniform mat4 WorldMatrix;

void main()
{
	Normal = (WorldMatrix * vec4(VertexNormal, 0.0)).xyz;
	gl_Position = WorldMatrix * vec4(VertexPosition, 1.0);
}