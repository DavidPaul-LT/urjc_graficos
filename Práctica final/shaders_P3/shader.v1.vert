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
out vec3 posCam;           // espacio cámara
out vec3 posWorld;        // espacio mundo
out vec3 normCam;        // normal en espacio cámara
out vec3 normWorld;     // normal en espacio mundo
out vec2 texCoord;
flat out int vCubeId;
uniform mat4 model;


void main()
{
    color = inColor;
    texCoord = inTexCoord;
    posCam   = (modelView * vec4(inPos,1)).xyz;
    posWorld  = (model * vec4(inPos,1)).xyz;
    normCam  = normalize((normal * vec4(inNormal,0)).xyz);
    normWorld = normalize((model * vec4(inNormal,0)).xyz);
    vCubeId = objectId;
    
    gl_Position = modelViewProj * vec4(inPos, 1.0);
}