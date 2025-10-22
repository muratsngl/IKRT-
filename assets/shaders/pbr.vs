#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    mat3 TBN;
    vec4 FragPosLightSpace;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform uint modelIndex;
uniform mat4 lightSpaceMatrix;  // Combined light projection * light view matrix

// UBO for model matrices (buffer base 3, holds 50 mat4s)
layout(std140, binding = 3) uniform ModelMatrices {
    mat4 models[50];
};

void main()
{
    // Get the model matrix from the UBO using the index
    mat4 model = models[modelIndex];
    
    // Calculate world position of the vertex
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.TexCoords = aTexCoords;
    
    // Transform fragment position to light space for shadow mapping
    vs_out.FragPosLightSpace = lightSpaceMatrix * vec4(vs_out.FragPos, 1.0);
    
    // To correctly transform normals, tangents, and bitangents to world space,
    // we use the normal matrix (the transpose of the inverse of the model matrix).
    // This prevents scaling issues from affecting the vectors' orientation.
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);
    
    // Create the TBN matrix for transforming normals from tangent space to world space
    vs_out.TBN = mat3(T, B, N);

    // Calculate final clip-space position
    gl_Position = projection * view * vec4(vs_out.FragPos, 1.0);
}