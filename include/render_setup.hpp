#ifndef RENDER_SETUP_HPP
#define RENDER_SETUP_HPP

#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Rendering context structure
struct RenderContext {
    GLFWwindow* window;
    unsigned int screen_width;
    unsigned int screen_height;
    
    // OpenGL objects
    GLuint orcunUBO;
   
    
    // Various VAOs and VBOs for different objects
    
    
    // Buffer handles for persistent mapping
    
    
    // Buffer management
    int whichBuffertoRead;
    int whichBuffertoWrite;
};

// Rendering functions
bool init_rendering();
void init_buffers();
void load_shaders();
void render_frame();
void cleanup_rendering();
bool should_close_window();
RenderContext& get_render_context();

// Callback functions
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void process_input(GLFWwindow* window);

#endif
