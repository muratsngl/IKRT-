#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

// We conventionallly name our textures with a type and a number
uniform sampler2D texture_diffuse1;

void main()
{
    FragColor = texture(texture_diffuse1, TexCoords);
}