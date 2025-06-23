#version 330 core

layout (location=0) in vec3 inPos;
layout (location=1) in vec3 inColor;
layout (location=2) in vec3 inNormal;
layout (location=3) in vec2 inTexCoord;

uniform mat4 modelViewProj;
uniform mat4 modelView;
uniform mat4 normal;
uniform int objectId;

out vec3 color;
out vec3 posCam;
out vec3 posWorld;
out vec3 normCam;
out vec2 texCoord;
flat out int vCubeId;

void main()
{
    color = inColor;
    texCoord = inTexCoord;
    normCam = (normal * vec4(inNormal, 0.0)).xyz;
    posCam = (modelView * vec4(inPos, 1.0)).xyz;
    posWorld = inPos;
    vCubeId = objectId;
    
    gl_Position = modelViewProj * vec4(inPos, 1.0);
}