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
    vec4 FragPosDirectionalLightSpace;  // For directional light
    vec4 FragPosLightSpace[10];  // Array for up to 10 spotlights
} vs_out;

// Standard transformation matrices
uniform mat4 view;
uniform mat4 projection;
//redundant holding for backward compatibility
uniform mat4 model;

// Shadow mapping uniforms
uniform mat4 directionalLightSpaceMatrix;  // For directional light
uniform mat4 lightSpaceMatrices[10];  // Array of light space matrices for 10 spotlights
uniform int numActiveShadowCastingSpotLights;  // Number of spotlights that cast shadows
uniform bool directionalLightCastsShadow;  // Whether directional light casts shadows

// Bone transformation matrices
layout(std140, binding = 1) uniform bone_transforms {
    mat4 bone_matrices[120];
};

void main()
{
    // Calculate the final bone transformation matrix by blending bone matrices with weights
    mat4 boneTransform = bone_matrices[boneID.x] * boneWeights.x + 
                         bone_matrices[boneID.y] * boneWeights.y + 
                         bone_matrices[boneID.z] * boneWeights.z + 
                         bone_matrices[boneID.w] * boneWeights.w;
    
    // Transform position: model * bone * position (matching working shader order)
    vec4 worldPos = model * boneTransform * vec4(aPos, 1.0);
    vs_out.FragPos = vec3(worldPos);
    
    // Pass texture coordinates through
    vs_out.TexCoords = aTexCoords;
    
    // Transform fragment position to directional light space
    if(directionalLightCastsShadow) {
        vs_out.FragPosDirectionalLightSpace = directionalLightSpaceMatrix * vec4(vs_out.FragPos, 1.0);
    }
    
    // Transform fragment position to light space for all active spotlights
    for(int i = 0; i < numActiveShadowCastingSpotLights && i < 10; i++) {
        vs_out.FragPosLightSpace[i] = lightSpaceMatrices[i] * vec4(vs_out.FragPos, 1.0);
    }
    
    // Transform normals, tangents, and bitangents for PBR
    // First apply bone transformation, then model transformation (same order as position)
    mat3 boneRotation = mat3(boneTransform);
    vec3 skinnedNormal = boneRotation * aNormal;
    vec3 skinnedTangent = boneRotation * aTangent;
    vec3 skinnedBitangent = boneRotation * aBitangent;
    
    // Use normal matrix to correctly transform to world space
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(normalMatrix * skinnedTangent);
    vec3 B = normalize(normalMatrix * skinnedBitangent);
    vec3 N = normalize(normalMatrix * skinnedNormal);
    
    // Create the TBN matrix for transforming normals from tangent space to world space
    vs_out.TBN = mat3(T, B, N);
    
    // Calculate final clip-space position
    gl_Position = projection * view * worldPos;
}
