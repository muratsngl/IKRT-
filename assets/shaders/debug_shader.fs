#version 460 core

in vec2 tex_pos;
uniform sampler2D debug_texture;

out vec4 FragColor;

void main(){
    float depth = texture(debug_texture, tex_pos).r;
    // Visualize depth: closer = darker, farther = brighter
    FragColor = vec4(vec3(depth), 1.0);
}