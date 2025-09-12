#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 norm;
layout(location = 2) in ivec4 boneID;
layout(location = 3) in vec4 boneWeights;


uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;



layout(std140,binding = 1) uniform bone_transforms{
mat4 bone_matrices[50];
};


void main(){
	mat4 ident = bone_matrices[boneID.x]*boneWeights.x + bone_matrices[boneID.y]*boneWeights.y + bone_matrices[boneID.z]*boneWeights.z + bone_matrices[boneID.w]*boneWeights.w;
	gl_Position = projection * view * model * ident* vec4(aPos,1.0);
}