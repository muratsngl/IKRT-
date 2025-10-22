#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

// UBO for model matrices (buffer base 3, holds 50 mat4s)
layout(std140, binding = 3) uniform ModelMatrices {
    mat4 models[50];
};

uniform mat4 light_view;
uniform mat4 light_projection;
uniform uint modelIndex;  // Changed from int to uint to match what shader.setUInt sends

void main(){
    mat4 model = models[modelIndex];
    gl_Position = light_projection * light_view * model * vec4(aPos, 1.0f);
}
