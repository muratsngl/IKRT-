#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in ivec4 boneID;
layout (location = 6) in vec4 boneWeights;

// Bone transformation matrices
layout(std140, binding = 1) uniform bone_transforms {
    mat4 bone_matrices[120];
};

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

out vec4 FragPos;

void main()
{
    // Calculate the final bone transformation matrix by blending bone matrices with weights
    mat4 boneTransform = bone_matrices[boneID.x] * boneWeights.x + 
                         bone_matrices[boneID.y] * boneWeights.y + 
                         bone_matrices[boneID.z] * boneWeights.z + 
                         bone_matrices[boneID.w] * boneWeights.w;
    
    // Transform position: model * bone * position (matching skeletal PBR shader order)
    FragPos = model * boneTransform * vec4(aPos, 1.0);
    gl_Position = lightSpaceMatrix * FragPos;
}
