#include "include/render_setup.hpp"
#include "include/model_loader.hpp"
#include "include/application_logic.hpp"
#include "include/shared_memory.hpp"
#include "include/Camera.h"
#include "include/Shader.h"
#include "include/model_bones.h" 
#include "include/application_logic.hpp"// Include the Model class definition

#include "include/raycast.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <unordered_map>

#include "include/UI.hpp"

// Global application mode variable definition
MODE app_mode = EDIT_SCENE;  // Default to edit scene mode

static RenderContext render_context;
// Initialize camera with edit mode default position
static Camera camera(glm::vec3(0.0f, -0.5f, 4.0f));

static Shader* skeletonShader = nullptr;
static Shader* sceneElementShader = nullptr;

// Collision visualizer instance
static CollisionVisualizer collisionVisualizer;


// Model matrix storage for UBO (50 matrices max)
static std::vector<glm::mat4> modelMatrices(50, glm::mat4(1.0f));
// Hashmap to map model IDs to their UBO matrix indices (CREATED FOR GIZMO FAST LOOKUP AND CHANGE OF MODEL MATRIX)
static std::unordered_map<int, unsigned int> modelIdToIndexMap;

// Mouse and timing variables
static float lastX = 1240.0f / 2.0f;
static float lastY = 720.0f / 2.0f;
static bool firstMouse = true;

// Add static variables to track the key state
static bool spaceKeyPressed = false;

// Gizmo state variables
static int selected_model_id = -1;

bool init_rendering() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4); 

    // Create window
    render_context.screen_width = 1240;
    render_context.screen_height = 720;
    render_context.window = glfwCreateWindow(render_context.screen_width, 
                                           render_context.screen_height, 
                                           "Inverse Kinematics with Realtime Data", 
                                           nullptr, nullptr);
    
    if (!render_context.window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(render_context.window);
    glfwSetFramebufferSizeCallback(render_context.window, framebuffer_size_callback);
    glfwSetCursorPosCallback(render_context.window, mouse_callback);
    glfwSetMouseButtonCallback(render_context.window, mouse_button_callback);
    glfwSetScrollCallback(render_context.window, scroll_callback);
    glfwSetInputMode(render_context.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return false;
    }
    
    // Initialize buffer management
    render_context.whichBuffertoRead = 0;
    render_context.whichBuffertoWrite = 1;
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    // Initialize UI system
    if (!init_ui(render_context.window)) {
        std::cerr << "Failed to initialize UI" << std::endl;
        return false;
    }
    
    return true;
}

void load_shaders() {
    skeletonShader = new Shader("assets/shaders/skeletal.vs", "assets/shaders/skeletal.fs");
    sceneElementShader = new Shader("assets/shaders/pbr.vs", "assets/shaders/pbr.fs");
    
   

}

void init_buffers() {
    GLbitfield flags = GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_WRITE_BIT;
    
  

    glGenBuffers(1, &render_context.orcunUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, render_context.orcunUBO);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, render_context.orcunUBO);
    glBufferData(GL_UNIFORM_BUFFER, 4000, nullptr, GL_DYNAMIC_READ);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    
    // Create interactable bone UBO
    // Size: 2 * MAX_INTERACTABLE_MODELS * sizeof(glm::mat4) matrices per interactable model
    GLsizeiptr interactable_bone_buffer_size = 2 * MAX_INTERACTABLE_MODELS * sizeof(glm::mat4);
    
    glGenBuffers(1, &render_context.interactable_bone_UBO);
    glBindBuffer(GL_UNIFORM_BUFFER, render_context.interactable_bone_UBO);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, render_context.interactable_bone_UBO);
    glBufferData(GL_UNIFORM_BUFFER, interactable_bone_buffer_size, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    
    // Create scene element model UBO
    GLsizeiptr scene_element_model_buffer_size = 50 * sizeof(glm::mat4);

    glGenBuffers(1, &render_context.scene_element_model_UBO);
    glBindBuffer(GL_UNIFORM_BUFFER, render_context.scene_element_model_UBO);
    glBindBufferBase(GL_UNIFORM_BUFFER, 3, render_context.scene_element_model_UBO);
    glBufferData(GL_UNIFORM_BUFFER, scene_element_model_buffer_size, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0); 
    
    // Initialize collision visualizer
    collisionVisualizer.init(); 
}

// Function to get unique model index for each model (called only once per model)
unsigned int get_model_index() {
    static unsigned int modelIndexCounter = 0;
    return modelIndexCounter++;
}

// Function to register model ID to index mapping
void register_model_id_to_index(int model_id, unsigned int model_index) {
    modelIdToIndexMap[model_id] = model_index;
}

// Function to get model index by model ID
unsigned int get_model_index_by_id(int model_id) {
    auto it = modelIdToIndexMap.find(model_id);
    if (it != modelIdToIndexMap.end()) {
        return it->second;
    }
    // Return 0 as fallback if ID not found
    std::cerr << "Warning: Model ID " << model_id << " not found in index map, using index 0" << std::endl;
    return 0;
}

// Function to upload model matrices to UBO (called every frame)
void upload_model_matrices() {
    glBindBuffer(GL_UNIFORM_BUFFER, render_context.scene_element_model_UBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, modelMatrices.size() * sizeof(glm::mat4), modelMatrices.data());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

// Function to get access to model matrices (for raycasting)
const std::vector<glm::mat4>& get_model_matrices() {
    return modelMatrices;
}

// Gizmo management functions
void set_selected_object(int model_id) {
    selected_model_id = model_id;
}

int get_selected_object_id() {
    return selected_model_id;
}

glm::mat4& get_selected_object_matrix() {
    if (selected_model_id != -1) {
        unsigned int model_index = get_model_index_by_id(selected_model_id);
        if (model_index < modelMatrices.size()) {
            return modelMatrices[model_index];
        }
    }
    // Return a static identity matrix as fallback (should not happen in normal usage)
    static glm::mat4 identity = glm::mat4(1.0f);
    return identity;
}

void clear_selection() {
    selected_model_id = -1;
}

void get_current_camera_matrices(glm::mat4& view, glm::mat4& projection) {
    
        // Free view or animate mode - use camera's natural view matrix
        view = camera.GetViewMatrix();
        projection = glm::perspective(glm::radians(camera.Zoom), 
                                     (float)render_context.screen_width / 
                                     (float)render_context.screen_height, 
                                     0.1f, 100.0f);
    
}

void render_frame() {
    // Upload model matrices to UBO every frame
    upload_model_matrices();
    
    // Clear the screen
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_MULTISAMPLE); // Enable multisampling for anti-aliasing
    //glEnable(GL_FRAMEBUFFER_SRGB); // Enable sRGB for correct color space
    
    // Set up matrices based on application mode using unified camera
    glm::mat4 view, projection;
    glm::mat4 model = glm::mat4(1.0f);
    
   // Both modes should use the same projection calculation
   projection = glm::perspective(glm::radians(camera.Zoom), 
                            (float)render_context.screen_width / 
                            (float)render_context.screen_height, 
                            0.1f, 100.0f);
    view = camera.GetViewMatrix();
   
    
    // Update uniform buffer with bone transforms
    //Room for optimization this could be done in a single memory access;
    if (is_interactor_model_available()) {
        const InteractorModelData& model_data = get_interactor_model_data();
        glBindBuffer(GL_UNIFORM_BUFFER, render_context.orcunUBO);
        for (short i = 0; i < model_data.bind_pose_matrices.size(); i++) {
            if (i < model_data.bind_pose_matrices.size()) {
                glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(glm::mat4), 
                              sizeof(glm::mat4), 
                              glm::value_ptr(model_data.bind_pose_matrices[i]));
            }
        }
    }

    // Render the skeletal model
    if (skeletonShader) {
        skeletonShader->use();
        skeletonShader->setMat4("model", model);
        skeletonShader->setMat4("view", view);
        skeletonShader->setMat4("projection", projection);
        
        if (is_interactor_model_available()) {
            get_interactor_model()->Draw(*skeletonShader);
        }
        
    }

    if(sceneElementShader){
        
        
        sceneElementShader->use();
        


        sceneElementShader->setMat4("view", view);
        sceneElementShader->setMat4("projection", projection);

        // Set up PBR lighting
        // Define 4 point lights positioned around the scene
        glm::vec3 lightPositions[4] = {
            glm::vec3(-10.0f,  10.0f, 10.0f),   // Top-left front
            glm::vec3( 10.0f,  10.0f, 10.0f),   // Top-right front  
            glm::vec3(-10.0f, -10.0f, 10.0f),   // Bottom-left front
            glm::vec3( 10.0f, -10.0f, 10.0f)    // Bottom-right front
        };
        
        glm::vec3 lightColors[4] = {
            glm::vec3(30.0f, 25.0f, 20.0f),  // Warm white light
            glm::vec3(25.0f, 30.0f, 30.0f),  // Cool white light
            glm::vec3(30.0f, 20.0f, 25.f),  // Slightly magenta light
            glm::vec3(20.0f, 30.0f, 25.0f)   // Slightly green light
        };

        // Set light uniforms
        for (int i = 0; i < 4; ++i) {
            sceneElementShader->setVec3("lightPositions[" + std::to_string(i) + "]", lightPositions[i]);
            sceneElementShader->setVec3("lightColors[" + std::to_string(i) + "]", lightColors[i]);
        }

        // Set camera position for view direction calculation (unified camera position)
        sceneElementShader->setVec3("camPos", camera.Position);

        // Note: PBR material texture samplers are now handled dynamically in Mesh::Draw()

        // Render scene element models (each model sets its own index)
        for(size_t i = 0; i < get_scene_element_model_count(); i++){
            const SceneElementModel& model = get_scene_element_model(i);
            model.Draw(*sceneElementShader);
        }
        
        // Render interactable models (each model sets its own index)
        for(size_t i = 0; i < get_interactable_model_count(); i++){
            const InteractableModel& model = get_interactable_model(i);
            model.Draw(*sceneElementShader);
        }
      
        
    }
    
    // Render collision geometry if enabled
    collisionVisualizer.renderCollisionGeometry(view, projection);
    
    // Render all UI components
    render_ui();
    
    // Process input and swap buffers
    process_input(render_context.window);
    glfwSwapBuffers(render_context.window);
    glfwPollEvents();
    
    // Update buffer indices
    render_context.whichBuffertoRead = (render_context.whichBuffertoRead + 1) % 2;
    render_context.whichBuffertoWrite = (render_context.whichBuffertoWrite + 1) % 2;
}

bool should_close_window() {
    return glfwWindowShouldClose(render_context.window);
}

void cleanup_rendering() {
    // Cleanup UI system
    cleanup_ui();
    
    // Cleanup collision visualizer
    collisionVisualizer.cleanup();
    
    delete skeletonShader;
    delete sceneElementShader;
    
   
    
    // Clean up UBOs
    glDeleteBuffers(1, &render_context.orcunUBO);
    glDeleteBuffers(1, &render_context.interactable_bone_UBO);
    
    if (render_context.window) {
        glfwDestroyWindow(render_context.window);
    }
    glfwTerminate();
}

RenderContext& get_render_context() {
    return render_context;
}

// Mode utility functions
const char* get_mode_name(MODE mode) {
    const char* mode_names[] = {"Edit Scene", "Free View", "Animate"};
    return mode_names[mode];
}

MODE get_app_mode() {
    return app_mode;
}

void set_app_mode(MODE mode) {
    app_mode = mode;
}

// Callback implementations
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    // Let ImGui handle mouse input if it wants to capture it
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        return;
    }
    
    // Only process mouse camera movement if not in EDIT_SCENE mode
    if (app_mode == EDIT_SCENE) {
        return;
    }
    
    // Only process mouse movement when left mouse button is held down
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS) {
        firstMouse = true;  // Reset first mouse flag when button is released
        return;
    }
    
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // Let ImGui handle mouse input if it wants to capture it
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        return;
    }
    
    // Only process mouse scroll for camera zoom if not in EDIT_SCENE mode
    if (app_mode != EDIT_SCENE) {
        camera.ProcessMouseScroll(static_cast<float>(yoffset));
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    // Let ImGui handle mouse input if it wants to capture it
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        return;
    }
    
    // Check if ImGuizmo is being used first
    if (ImGuizmo::IsUsing()) {
        return; // Let ImGuizmo handle the input
    }
    
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && app_mode == EDIT_SCENE) {
        // Get current mouse position
        double mouse_x, mouse_y;
        glfwGetCursorPos(window, &mouse_x, &mouse_y);
        
        // Get current camera matrices - need to recreate the same logic from render_frame
        glm::mat4 view, projection;
        

            // Free view or animate mode - use camera's natural view matrix
            view = camera.GetViewMatrix();
            projection = glm::perspective(glm::radians(camera.Zoom), 
                                        (float)render_context.screen_width / 
                                        (float)render_context.screen_height, 
                                        0.1f, 100.0f);
        
        
        // Generate ray from mouse position
        Ray ray = generate_ray(static_cast<float>(mouse_x), static_cast<float>(mouse_y), 
                              view, projection, camera.Position,
                              render_context.screen_width, render_context.screen_height);
        
        // Collect all shapes for intersection testing

        

        
   
        
        int hit_model_id = intersect_ray(ray,scene_element_boxes);
        if (hit_model_id != -1) {
            // Set the selected object for gizmo manipulation
            set_selected_object(hit_model_id);
            std::cout << "Selected object ID: " << hit_model_id << std::endl;
        } else {
            // Clear selection if nothing was hit
            clear_selection();
            std::cout << "Selection cleared" << std::endl;
        }
    }
}

void process_input(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    const ApplicationState& app_state = get_application_state();
    
    // Only process camera movement if not in EDIT_SCENE mode
    if (app_mode != EDIT_SCENE) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, app_state.deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, app_state.deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, app_state.deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, app_state.deltaTime);
    }
    //DEBUG: CURRENTLY TOGGLES ROOT LOCK RANDOMLY BECAUSE OF BUTTON DEBOUNCING//SOLVED WITH SCHMIDT TRIGGER
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        if (!spaceKeyPressed && is_shared_memory_available()) { // Key was not previously pressed and shared memory is available
            get_finger_data().rootLock = !get_finger_data().rootLock; // Toggle root lock
            std::cout << "rootLock toggled to: " << get_finger_data().rootLock << std::endl;
            spaceKeyPressed = true; // Mark the key as pressed
        } else if (!spaceKeyPressed && !is_shared_memory_available()) {
            static bool notified = false;
            if (!notified) {
                std::cout << "Note: Root lock toggle disabled - shared memory not available" << std::endl;
                notified = true;
            }
            spaceKeyPressed = true;
        }
    } else {
        spaceKeyPressed = false; // Reset the key state when released
        // Reset notification when shared memory becomes available again
        if (is_shared_memory_available()) {
            static bool reset_notification = true;
            if (reset_notification) {
                std::cout << "Root lock toggle is now active - shared memory available" << std::endl;
                reset_notification = false;
            }
        }
    }
}

// Collision visualization functions
void init_collision_visualizer() {
    collisionVisualizer.init();
}

void cleanup_collision_visualizer() {
    collisionVisualizer.cleanup();
}

CollisionVisualizer& get_collision_visualizer() {
    return collisionVisualizer;
}
