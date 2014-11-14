#version 330

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(location = 2) in ivec4 BoneIDs;
layout(location = 3) in vec4 Weights;

uniform mat4 projMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

uniform mat4 gBones[100];
 
out vec4 vertexPos;
out vec3 Normal;
 
void main()
{
    mat4 BoneTransform = gBones[BoneIDs[0]] * Weights[0];
    BoneTransform += gBones[BoneIDs[1]] * Weights[1];
    BoneTransform += gBones[BoneIDs[2]] * Weights[2];
    BoneTransform += gBones[BoneIDs[3]] * Weights[3];


    Normal = normalize(vec3(viewMatrix * modelMatrix * BoneTransform * vec4(normal,0.0)));
	//TexCoord = vec2(texCoord);
    gl_Position = projMatrix * viewMatrix * modelMatrix * BoneTransform * vec4(position,1.0);
}