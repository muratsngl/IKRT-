#ifndef RENDER_SETUP_HPP
#define RENDER_SETUP_HPP

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <UI.hpp>
#include "collision_visualizer.hpp"

// Constants
#define MAX_INTERACTABLE_MODELS 10
#define DIRECTIONAL_LIGHT_INDEX 0   // Directional light uses index 0
#define SPOTLIGHT_START_INDEX 1      // Spotlights use indices 1-10
#define POINTLIGHT_INDEX 11          // Point lights use indices 11-20
#define SHADOW_WIDTH 1024
#define SHADOW_HEIGHT 1024

// Application mode enum
enum MODE {
    EDIT_SCENE,
    FREE_VIEW
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
    
    // Single framebuffer used for all shadow passes (texture swapping)
    GLuint shadow_FBO;
    
    // Array of 21 shadow textures:
    // Index 0: Directional light
    // Indices 1-10: 10 Spotlights
    // Indices 11-20: 10 Point lights
    GLuint shadow_tex[21];
    
    
    // Buffer management
    int whichBuffertoRead;
    int whichBuffertoWrite;
};

// Rendering functions
bool init_rendering();
void create_shadow_maps();
void init_buffers();
void load_shaders();
void render_frame();
void cleanup_rendering();
bool should_close_window();
RenderContext& get_render_context();

// Model matrix management functions
unsigned int get_model_index();
void register_model_id_to_index(int model_id, unsigned int model_index);
void unregister_model_id_from_index(int model_id);
unsigned int get_model_index_by_id(int model_id);
void upload_model_matrices();
const std::vector<glm::mat4>& get_model_matrices();

// Gizmo management functions
// Gizmo and selection management functions
void set_selected_object(int model_id);
int get_selected_object_id();
glm::mat4& get_selected_object_matrix();
void clear_selection();
void get_current_camera_matrices(glm::mat4& view, glm::mat4& projection);

// Cycle selection functions
void cycle_to_next_object();
bool is_same_click_location(double mouse_x, double mouse_y);

// Mode utility functions
const char* get_mode_name(MODE mode);
MODE get_app_mode();
void set_app_mode(MODE mode);

// Collision visualization functions
void init_collision_visualizer();
void cleanup_collision_visualizer();
CollisionVisualizer& get_collision_visualizer();

// Light management functions
class LightManager;
LightManager& get_light_manager();

// Callback functions
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void process_input(GLFWwindow* window);

#endif
