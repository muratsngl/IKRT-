#version 460 core

layout(location = 0) in vec2 aPos;

out vec2 tex_pos;

void main(){
    // Use the vertex position directly for the screen position
    gl_Position = vec4(aPos, 0.0, 1.0);
    
    // Convert aPos from [-1, 1] range to [0, 1] range for tex_pos
    // This correctly maps the quad to the texture
    tex_pos = (aPos + 1.0) / 2.0; 
}