#version 330 core

layout (location=0) in vec3 inPos;
layout (location=1) in vec3 inColor;
layout (location=2) in vec3 inNormal;
layout (location=3) in vec2 inTexCoord;

uniform mat4 modelViewProj;
uniform mat4 modelView;
uniform mat4 normal;
uniform int cubeId;

out vec3 color;
out vec3 pos;       // espacio cámara
out vec3 pos2;      // espacio mundo
out vec3 norm;      // normal en espacio cámara
out vec3 norm_world;// normal en espacio mundo
out vec2 texCoord;
flat out int vCubeId;
uniform mat4 model;


void main()
{
    color = inColor;
    texCoord = inTexCoord;
    pos   = (modelView * vec4(inPos,1)).xyz;
    pos2  = (model * vec4(inPos,1)).xyz;
    norm  = normalize((normal * vec4(inNormal,0)).xyz);
    norm_world = normalize((model * vec4(inNormal,0)).xyz);
    vCubeId = cubeId;
    
    gl_Position = modelViewProj * vec4(inPos, 1.0);
}