#include "include/render_setup.hpp"
#include "include/model_loader.hpp"
#include "include/application_logic.hpp"
#include "include/shared_memory.hpp"
#include "include/Camera.h"
#include "include/Shader.h"
#include "include/model_bones.h" 
#include "include/application_logic.hpp"// Include the Model class definition
#include "include/light_manager.hpp"

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
static Camera camera(glm::vec3(0.0f, 5.5f, 30.0f));

static Shader* skeletonShader = nullptr;
static Shader* sceneElementShader = nullptr;

// Light manager instance
static LightManager lightManager;

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

// Cycle selection state variables
static std::vector<int> hit_objects_list;
static int current_selection_index = 0;
static double last_mouse_x = -1.0;
static double last_mouse_y = -1.0;
static const double CLICK_THRESHOLD = 25.0; // Pixels tolerance for "same location"

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
                                           "I", 
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
    skeletonShader = new Shader("assets/shaders/skeletal_pbr.vs", "assets/shaders/skeletal_pbr.fs");
    sceneElementShader = new Shader("assets/shaders/pbr.vs", "assets/shaders/pbr.fs");
    
    // Initialize light manager
    lightManager.initialize();
    lightManager.bindUBO(4); // Bind to binding point 4 as specified in shader
    
    // Set up a default directional light (sun)
    DirectionalLight sun;
    sun.direction = glm::normalize(glm::vec3(-0.2f, -1.0f, -0.3f));
    sun.color = glm::vec3(1.0f, 0.95f, 0.8f);
    sun.intensity = 0.5f; // Reduced from 2.0f to make point lights more visible
    lightManager.setDirectionalLight(sun);
    
    // Update the UBO with default lighting
    lightManager.updateUBO();
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

// Static matrices for target proxy manipulation
static glm::mat4 targetProxyMatrix_Index = glm::mat4(1.0f);
static glm::mat4 targetProxyMatrix_Middle = glm::mat4(1.0f);
static glm::mat4 targetProxyMatrix_Ring = glm::mat4(1.0f);
static glm::mat4 targetProxyMatrix_Pinky = glm::mat4(1.0f);

glm::mat4& get_selected_object_matrix() {
    // Check if selected object is a target proxy
    if (selected_model_id >= TARGET_PROXY_INDEX && selected_model_id <= TARGET_PROXY_PINKY) {
        // Get current target position and create matrix
        ApplicationState& app_state = get_application_state();
        glm::vec3* targetPos = nullptr;
        glm::mat4* proxyMatrix = nullptr;
        
        switch(selected_model_id) {
            case TARGET_PROXY_INDEX:
                targetPos = &app_state.targetPositionIndex;
                proxyMatrix = &targetProxyMatrix_Index;
                break;
            case TARGET_PROXY_MIDDLE:
                targetPos = &app_state.targetPositionMiddle;
                proxyMatrix = &targetProxyMatrix_Middle;
                break;
            case TARGET_PROXY_RING:
                targetPos = &app_state.targetPositionRing;
                proxyMatrix = &targetProxyMatrix_Ring;
                break;
            case TARGET_PROXY_PINKY:
                targetPos = &app_state.targetPositionPinky;
                proxyMatrix = &targetProxyMatrix_Pinky;
                break;
        }
        
        if (targetPos && proxyMatrix) {
            // Update matrix with current position
            *proxyMatrix = glm::translate(glm::mat4(1.0f), *targetPos);
            return *proxyMatrix;
        }
    }
    
    // Regular model selection
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
    hit_objects_list.clear();
    current_selection_index = 0;
}

bool is_same_click_location(double mouse_x, double mouse_y) {
    if (last_mouse_x < 0 || last_mouse_y < 0) {
        return false; // No previous click recorded
    }
    
    double dx = mouse_x - last_mouse_x;
    double dy = mouse_y - last_mouse_y;
    double distance = dx * dx + dy * dy;
    
    return distance <= CLICK_THRESHOLD;
}

void cycle_to_next_object() {
    if (!hit_objects_list.empty()) {
        // Move to next object in the list
        current_selection_index = (current_selection_index + 1) % hit_objects_list.size();
        
        // Update selection
        selected_model_id = hit_objects_list[current_selection_index];
        std::cout << "Cycled to object ID: " << selected_model_id 
                  << " (index " << current_selection_index + 1 
                  << " of " << hit_objects_list.size() << ")" << std::endl;
    }
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

        // Update and bind light data via UBO
        // The light setup should be done elsewhere, but we'll ensure UBO is updated
        lightManager.updateUBO();
        lightManager.bindUBO(4);

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
    
    // Render target position proxies if enabled The problem here is the proxy rendering and the render collision geometry is coupled for the first click.
    collisionVisualizer.renderTargetProxies(view, projection);
    
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
    const char* mode_names[] = {"Edit Scene", "Free View"};
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
        
        // Check if this is a repeat click at the same location
        bool is_repeat_click = is_same_click_location(mouse_x, mouse_y);
        
        if (!is_repeat_click || hit_objects_list.empty()) {
            // This is a new query - clear previous selection and perform new raycast
            clear_selection();
            
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
            std::vector<Shape> allShapes;
            allShapes.insert(allShapes.end(), scene_element_boxes.begin(), scene_element_boxes.end());
            allShapes.insert(allShapes.end(), get_interactable_element_boxes().begin(), get_interactable_element_boxes().end());
            
            // Include target proxies if they're visible
            if (CollisionVisualizer::showTargetProxies) {
                std::vector<Shape>& target_proxies = get_target_proxy_boxes();
                allShapes.insert(allShapes.end(), target_proxies.begin(), target_proxies.end());
            }
            
            // Find all objects the ray hits and sort them by distance (nearest first)
            hit_objects_list = intersect_ray_all(ray, allShapes);
            
            if (!hit_objects_list.empty()) {
                // Select the first item
                current_selection_index = 0;
                selected_model_id = hit_objects_list[current_selection_index];
                std::cout << "Selected object ID: " << selected_model_id 
                          << " (1 of " << hit_objects_list.size() << " objects at this location)" << std::endl;
            } else {
                // Clicked on empty space
                std::cout << "Selection cleared - clicked on empty space" << std::endl;
            }
            
            // Remember where this click happened
            last_mouse_x = mouse_x;
            last_mouse_y = mouse_y;
        } else {
            // This is a repeat click at the same location - cycle to next object
            cycle_to_next_object();
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

LightManager& get_light_manager() {
    return lightManager;
}
