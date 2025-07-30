#version 460 core

layout (location = 0) in vec3 in_pos;
layout (location = 2) in uvec2 boneIds;
layout (location = 1) in vec2 boneWeights;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

layout(std140,binding = 0) uniform bone_transforms{
mat4 bone_matrices[4];
};

void main(){
//currently invalidating the second bone by using negative index not a good practice I suppose
mat4 ident = bone_matrices[boneIds[0]-1]*boneWeights[0] + bone_matrices[boneIds[1]-1]*boneWeights[1];
gl_Position = projection*view*model*ident*vec4(in_pos,1.0);
}


