#version 460 core
layout (location = 0) in vec3 aPos;

// UBO for model matrices (buffer base 3, holds 50 mat4s)
layout(std140, binding = 3) uniform ModelMatrices {
    mat4 models[50];
};

uniform mat4 lightSpaceMatrix;
uniform uint modelIndex;

out vec4 FragPos;

void main()
{
    mat4 model = models[modelIndex];
    FragPos = model * vec4(aPos, 1.0);
    gl_Position = lightSpaceMatrix * FragPos;
}
