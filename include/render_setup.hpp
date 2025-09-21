#ifndef RENDER_SETUP_HPP
#define RENDER_SETUP_HPP

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <UI.hpp>

// Constants
#define MAX_INTERACTABLE_MODELS 10

// Application mode enum
enum MODE {
    EDIT_SCENE,
    FREE_VIEW,
    ANIMATE
};

// Global application mode variable
extern MODE app_mode;

// Rendering context structure
struct RenderContext {
    GLFWwindow* window;
    unsigned int screen_width;
    unsigned int screen_height;
    
    // OpenGL Uniform Buffer Objects
    GLuint orcunUBO;
    GLuint interactable_bone_UBO;
    GLuint scene_element_model_UBO;
    
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

// Model matrix management functions
unsigned int get_model_index();
void upload_model_matrices();

// Mode utility functions
const char* get_mode_name(MODE mode);
MODE get_app_mode();
void set_app_mode(MODE mode);

// Callback functions
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void process_input(GLFWwindow* window);

#endif
