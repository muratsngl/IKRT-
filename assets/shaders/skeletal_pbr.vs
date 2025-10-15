#version 460 core

// PBR vertex attributes with skeletal animation support
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;
layout(location = 5) in ivec4 boneID;
layout(location = 6) in vec4 boneWeights;

out VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    mat3 TBN;
} vs_out;

// Standard transformation matrices
uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

// Bone transformation matrices
layout(std140, binding = 1) uniform bone_transforms {
    mat4 bone_matrices[50];
};

void main()
{
    // Calculate the final bone transformation matrix by blending bone matrices with weights
    mat4 boneTransform = bone_matrices[boneID.x] * boneWeights.x + 
                         bone_matrices[boneID.y] * boneWeights.y + 
                         bone_matrices[boneID.z] * boneWeights.z + 
                         bone_matrices[boneID.w] * boneWeights.w;
    
    // Transform position by bone transformation, then by model matrix
    vec4 skinnedPos = boneTransform * vec4(aPos, 1.0);
    vs_out.FragPos = vec3(model * skinnedPos);
    
    // Pass texture coordinates through
    vs_out.TexCoords = aTexCoords;
    
    // Transform normals, tangents, and bitangents for PBR
    // Apply bone transformation to vertex attributes, then model transformation
    vec3 skinnedNormal = mat3(boneTransform) * aNormal;
    vec3 skinnedTangent = mat3(boneTransform) * aTangent;
    vec3 skinnedBitangent = mat3(boneTransform) * aBitangent;
    
    // Use normal matrix to correctly transform to world space
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(normalMatrix * skinnedTangent);
    vec3 B = normalize(normalMatrix * skinnedBitangent);
    vec3 N = normalize(normalMatrix * skinnedNormal);
    
    // Create the TBN matrix for transforming normals from tangent space to world space
    vs_out.TBN = mat3(T, B, N);
    
    // Calculate final clip-space position
    gl_Position = projection * view * vec4(vs_out.FragPos, 1.0);
}
