#include "include/render_setup.hpp"
#include "include/model_loader.hpp"
#include "include/application_logic.hpp"
#include "include/shared_memory.hpp"
#include "include/Camera.h"
#include "include/Shader.h"
#include "include/model_bones.h" 
#include "include/bone_hierarchy.hpp"
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
static Shader* shadowShader = nullptr;
static Shader* pointShadowShader = nullptr;
static Shader* skeletalShadowShader = nullptr;
static Shader* skeletalPointShadowShader = nullptr;
static Shader* debug_shader = nullptr;

// Light manager instance
static LightManager lightManager;

// Collision visualizer instance
static CollisionVisualizer collisionVisualizer;


// Model matrix storage for UBO (50 matrices max)
static std::vector<glm::mat4> modelMatrices(50, glm::mat4(1.0f));
// Hashmap to map model IDs to their UBO matrix indices (CREATED FOR GIZMO FAST LOOKUP AND CHANGE OF MODEL MATRIX)
static std::unordered_map<int, unsigned int> modelIdToIndexMap;
// Free list of available UBO indices for reuse
static std::vector<unsigned int> freeModelIndices;

// Mouse and timing variables
static float lastX = 1240.0f / 2.0f;
static float lastY = 720.0f / 2.0f;
static bool firstMouse = true;

// Add static variables to track the key state
static bool spaceKeyPressed = false;
static bool deleteKeyPressed = false;

// Quit confirmation state
static bool quitRequested = false;
static bool showQuitConfirmation = false;

// Window close callback (forward declaration)


static uint debug_rect_vao,debug_rect_vbo;
// Gizmo state variables
static int selected_model_id = -1;
const float debug_rect[12]={
        // Triangle 1
        -1.0f, -1.0f, // Bottom-left
         1.0f, -1.0f, // Bottom-right
        -1.0f,  1.0f, // Top-left
        // Triangle 2
        -1.0f,  1.0f, // Top-left
         1.0f, -1.0f, // Bottom-right
         1.0f,  1.0f  // Top-right
    };
const float cubemap_vertices[108] = {
    // positions          
    // Back face (-Z)
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    // Left face (-X)
    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    // Right face (+X)
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     
    // Front face (+Z)
    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    // Bottom face (-Y)
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f, // <--- Corrected line
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,

    // Top face (+Y)
    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f, // <--- Corrected line
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f
};
// Cycle selection state variables
static std::vector<int> hit_objects_list;
static int current_selection_index = 0;
static double last_mouse_x = -1.0;
static double last_mouse_y = -1.0;
static const double CLICK_THRESHOLD = 25.0; // Pixels tolerance for "same location"

// Wireframe mode state
static bool wireframe_mode = false;

void set_wireframe_mode(bool enabled) {
    wireframe_mode = enabled;
}

bool is_wireframe_mode_enabled() {
    return wireframe_mode;
}

void render_texture_debug_rect(GLuint debug_texture){
    debug_shader->use();
    debug_shader->setInt("debug_texture", 0);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, debug_texture);
    
    glBindVertexArray(debug_rect_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

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
    
    // Set window close callback for quit confirmation
    glfwSetWindowCloseCallback(render_context.window, window_close_callback);
    
    return true;
}

void load_shaders() {
    skeletonShader = new Shader("assets/shaders/skeletal_pbr.vs", "assets/shaders/skeletal_pbr.fs");
    sceneElementShader = new Shader("assets/shaders/pbr.vs", "assets/shaders/pbr.fs");
    shadowShader = new Shader("assets/shaders/shadow_shader.vs","assets/shaders/shadow_shader.fs");
    pointShadowShader = new Shader("assets/shaders/point_shadow.vs","assets/shaders/point_shadow.fs");
    skeletalShadowShader = new Shader("assets/shaders/skeletal_shadow.vs","assets/shaders/skeletal_shadow.fs");
    skeletalPointShadowShader = new Shader("assets/shaders/skeletal_point_shadow.vs","assets/shaders/skeletal_point_shadow.fs");
    debug_shader = new Shader("assets/shaders/debug_shader.vs","assets/shaders/debug_shader.fs");
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

//Initialize depth textures for directional light, spotlights, and point lights
void create_shadow_maps(){ 
    // Create single framebuffer for all shadow passes
    glGenFramebuffers(1, &render_context.shadow_FBO);
    
    // Create shadow texture for directional light (index 0)
    glGenTextures(1, &render_context.shadow_tex[DIRECTIONAL_LIGHT_INDEX]);
    glBindTexture(GL_TEXTURE_2D, render_context.shadow_tex[DIRECTIONAL_LIGHT_INDEX]);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    
    // Create shadow textures for 10 spotlights (indices 1-10)
    for(int i = 0; i < 10; i++) {
        int index = SPOTLIGHT_START_INDEX + i;
        glGenTextures(1, &render_context.shadow_tex[index]);
        glBindTexture(GL_TEXTURE_2D, render_context.shadow_tex[index]);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 
                     0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    }

    // Create shadow textures for 10 point lights (indices 11-20)
    for(int i = 0; i < 10; i++) {
        int index = POINTLIGHT_INDEX + i; // 11, 12, 13, ... 20
        
        glGenTextures(1, &render_context.shadow_tex[index]);
        glBindTexture(GL_TEXTURE_CUBE_MAP, render_context.shadow_tex[index]);
        
        // Create all 6 faces of the cubemap
        for(int face = 0; face < 6; face++) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_DEPTH_COMPONENT, 
                        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        }
        
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
    
    // Test the framebuffer with a dummy attachment to verify it works
    glBindFramebuffer(GL_FRAMEBUFFER, render_context.shadow_FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 
                           render_context.shadow_tex[DIRECTIONAL_LIGHT_INDEX], 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR: Shadow framebuffer is not complete!" << std::endl;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
};


void init_buffers() {
    GLbitfield flags = GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_WRITE_BIT;
    
    //Shadow arrays
    glGenVertexArrays(1,&debug_rect_vao);
    glGenBuffers(1,&debug_rect_vbo);
    glBindVertexArray(debug_rect_vao);
    glBindBuffer(GL_ARRAY_BUFFER,debug_rect_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),nullptr);
    glBufferData(GL_ARRAY_BUFFER,12*sizeof(float),debug_rect,GL_STATIC_DRAW);


    glGenBuffers(1, &render_context.orcunUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, render_context.orcunUBO);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, render_context.orcunUBO);
    glBufferData(GL_UNIFORM_BUFFER, 7680, nullptr, GL_DYNAMIC_READ);  // 120 bone matrices * 64 bytes
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
    
    // Reuse a freed index if available
    if (!freeModelIndices.empty()) {
        unsigned int index = freeModelIndices.back();
        freeModelIndices.pop_back();
        std::cout << "Reusing freed UBO slot: " << index << std::endl;
        return index;
    }
    
    // Otherwise allocate a new index
    if (modelIndexCounter >= 50) {
        std::cerr << "Error: Exceeded maximum number of models (50)!" << std::endl;
        return 0;
    }
    
    return modelIndexCounter++;
}

// Function to register model ID to index mapping
void register_model_id_to_index(int model_id, unsigned int model_index) {
    modelIdToIndexMap[model_id] = model_index;
}

// Function to unregister model ID from index mapping
void unregister_model_id_from_index(int model_id) {
    // Get the index before removing from map
    auto it = modelIdToIndexMap.find(model_id);
    if (it != modelIdToIndexMap.end()) {
        unsigned int freed_index = it->second;
        modelIdToIndexMap.erase(it);
        
        // Add the freed index to the free list for reuse
        freeModelIndices.push_back(freed_index);
        std::cout << "Freed UBO slot " << freed_index << " for reuse" << std::endl;
    }
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

// Function to get model matrix by ID
glm::mat4 get_model_matrix_by_id(int model_id) {
    unsigned int index = get_model_index_by_id(model_id);
    if (index < modelMatrices.size()) {
        return modelMatrices[index];
    }
    std::cerr << "Warning: Invalid model index " << index << " for model ID " << model_id << std::endl;
    return glm::mat4(1.0f);
}

// Function to set model matrix by ID
void set_model_matrix_by_id(int model_id, const glm::mat4& matrix) {
    unsigned int index = get_model_index_by_id(model_id);
    if (index < modelMatrices.size()) {
        modelMatrices[index] = matrix;
    } else {
        std::cerr << "Warning: Invalid model index " << index << " for model ID " << model_id << std::endl;
    }
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

// TODO:: EITHER GET RID OF THIS CODE OR MOVE TO ANOTHER UNDERSTANDABLE FILE 0 DEBUGGABILITY
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
    
    // Check if this is a bone selection (ID >= 20000)
    if (selected_model_id >= 20000) {
        int bone_id = selected_model_id - 20000;
        InteractorModelData& model_data = get_interactor_model_data_mutable();
        InteractorModel* model = get_interactor_model();
        
        if (bone_id < model_data.bind_pose_matrices.size() && model) {
            // Check if this bone has no children (leaf bone)
            BoneHierarchy* hierarchy = model->getBoneHierarchy();
            if (hierarchy && hierarchy->isValid()) {
                BoneNode* bone = hierarchy->findBoneById(bone_id);
            }
            
            // Return direct reference to the bone matrix so gizmo can modify it
            model_data.imm_transformation_matrices[bone_id] = glm::mat4(1.0f);
            return model_data.imm_transformation_matrices[bone_id];
        }
        
        // Bone ID out of bounds, return identity
        static glm::mat4 identity = glm::mat4(1.0f);
        return identity;
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
    glm::mat4 view, projection,lightView,lightProjection;
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

    // ============================================================================
    // DIRECTIONAL LIGHT SHADOW PASS
    // ============================================================================
    glm::mat4 directionalLightSpaceMatrix = glm::mat4(1.0f);
    bool directionalLightCastsShadow = lightManager.isDirectionalLightActive();
    
    if(directionalLightCastsShadow && shadowShader) {
        const DirectionalLight& dirLight = lightManager.getDirectionalLight();
        
        // For directional light, we need to set up an orthographic projection
        // that covers the scene. This is a simplified approach - a better one
        // would use cascaded shadow maps.
        float shadowDistance = 50.0f;  // How far the shadow extends
        lightProjection = glm::ortho(-shadowDistance, shadowDistance, 
                                     -shadowDistance, shadowDistance, 
                                     0.1f, 100.0f);
        
        // Position the light "far away" in the opposite direction of the light direction
        glm::vec3 lightPos = -glm::normalize(dirLight.direction) * 50.0f;
        glm::vec3 lightTarget = glm::vec3(0.0f);  // Look at scene center
        lightView = glm::lookAt(lightPos, lightTarget, glm::vec3(0.0f, 1.0f, 0.0f));
        
        directionalLightSpaceMatrix = lightProjection * lightView;
        
        // Bind single framebuffer and attach directional light shadow texture
        glBindFramebuffer(GL_FRAMEBUFFER, render_context.shadow_FBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 
                               render_context.shadow_tex[DIRECTIONAL_LIGHT_INDEX], 0);
        
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if(status != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Directional light shadow framebuffer not complete: " << status << std::endl;
        }
        
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glClear(GL_DEPTH_BUFFER_BIT);
        
        // Bind shadow shader
        shadowShader->use();
        shadowShader->setMat4("light_view", lightView);
        shadowShader->setMat4("light_projection", lightProjection);
        
        // Draw scene
        size_t scene_count = get_scene_element_model_count();
        for(size_t j = 0; j < scene_count; j++) {
            const SceneElementModel& model = get_scene_element_model(j);
            model.Draw(*shadowShader);
        }
        
        // Render interactable models
        size_t interactable_count = get_interactable_model_count();
        for(size_t j = 0; j < interactable_count; j++) {
            const InteractableModel& model = get_interactable_model(j);
            model.Draw(*shadowShader);
        }
        
        // Render skeletal model (interactor)
        if(is_interactor_model_available() && skeletalShadowShader) {
            skeletalShadowShader->use();
            skeletalShadowShader->setMat4("light_view", lightView);
            skeletalShadowShader->setMat4("light_projection", lightProjection);
            skeletalShadowShader->setMat4("model", model);
            InteractorModel* interactorModel = get_interactor_model();
            if(interactorModel) {
                interactorModel->Draw(*skeletalShadowShader);
            }
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, render_context.screen_width, render_context.screen_height);
    }

    // ============================================================================
    // SPOTLIGHT SHADOW PASSES (for all active spotlights)
    // ============================================================================
    int numShadowCastingSpotLights = std::min(lightManager.getActiveSpotLights(), 10);
    static std::vector<glm::mat4> spotLightSpaceMatrices(10, glm::mat4(1.0f));
    
    for(int i = 0; i < numShadowCastingSpotLights; i++) {
        const SpotLight& sl = lightManager.getSpotLight(i);
        
        // Create light matrices
        // Here the up vector and the center vector are given in a non rigorous fashion will change in the actual system
        lightView = glm::lookAt(sl.position, sl.position + sl.direction, glm::vec3(0.0f, 0.99f, 0.1f));
        lightProjection = glm::perspective(glm::radians(90.0f), 
                            (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, 
                            0.1f, 100.0f);
        
        // Store the light space matrix for later use in main render pass
        spotLightSpaceMatrices[i] = lightProjection * lightView;
        
        // Bind single framebuffer and attach this spotlight's shadow texture
        int shadowMapIndex = SPOTLIGHT_START_INDEX + i;
        glBindFramebuffer(GL_FRAMEBUFFER, render_context.shadow_FBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 
                               render_context.shadow_tex[shadowMapIndex], 0);
        
        // Check framebuffer status
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if(status != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Spotlight " << i << " shadow framebuffer not complete: " << status << std::endl;
        }
        
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glClear(GL_DEPTH_BUFFER_BIT);
       
        // Bind shadow shader
        shadowShader->use();
        // Upload uniforms to the shader
        shadowShader->setMat4("light_view", lightView);
        shadowShader->setMat4("light_projection", lightProjection);
        
        // Draw scene
        size_t scene_count = get_scene_element_model_count();
        for(size_t j = 0; j < scene_count; j++) {
            const SceneElementModel& model = get_scene_element_model(j);
            model.Draw(*shadowShader);
        }
        
        // Render interactable models (each model sets its own index)
        size_t interactable_count = get_interactable_model_count();
        for(size_t j = 0; j < interactable_count; j++) {
            const InteractableModel& model = get_interactable_model(j);
            model.Draw(*shadowShader);
        }
        
        // Render skeletal model (interactor)
        if(is_interactor_model_available() && skeletalShadowShader) {
            skeletalShadowShader->use();
            skeletalShadowShader->setMat4("light_view", lightView);
            skeletalShadowShader->setMat4("light_projection", lightProjection);
            skeletalShadowShader->setMat4("model", model);
            InteractorModel* interactorModel = get_interactor_model();
            if(interactorModel) {
                interactorModel->Draw(*skeletalShadowShader);
            }
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, render_context.screen_width, render_context.screen_height);
    }

    // ============================================================================
    // POINT LIGHT SHADOW PASSES (6 faces for each cubemap)
    // ============================================================================
    int numShadowCastingPointLights = std::min(lightManager.getActivePointLights(), 10);
    
    for(int lightIdx = 0; lightIdx < numShadowCastingPointLights; lightIdx++) {
        const PointLight& pointLight = lightManager.getPointLight(lightIdx);
        
        float near_plane = 0.1f;
        float far_plane = 25.0f;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, near_plane, far_plane);
        
        // Define 6 view matrices for each cubemap face
        static std::vector<glm::mat4> shadowTransforms(6, glm::mat4(1.0f));
        shadowTransforms[0] = shadowProj * glm::lookAt(pointLight.position, pointLight.position + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)); // +X
        shadowTransforms[1] = shadowProj * glm::lookAt(pointLight.position, pointLight.position + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)); // -X
        shadowTransforms[2] = shadowProj * glm::lookAt(pointLight.position, pointLight.position + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)); // +Y
        shadowTransforms[3] = shadowProj * glm::lookAt(pointLight.position, pointLight.position + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)); // -Y
        shadowTransforms[4] = shadowProj * glm::lookAt(pointLight.position, pointLight.position + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)); // +Z
        shadowTransforms[5] = shadowProj * glm::lookAt(pointLight.position, pointLight.position + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)); // -Z
        
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, render_context.shadow_FBO);
        
        pointShadowShader->use();
        pointShadowShader->setVec3("lightPos", pointLight.position);
        pointShadowShader->setFloat("far_plane", far_plane);
        
        // Render to each face of the cubemap
        for(int face = 0; face < 6; face++) {
            // Attach the current face to the framebuffer (texture swapping)
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 
                                   render_context.shadow_tex[POINTLIGHT_INDEX + lightIdx], 0);
            
            glClear(GL_DEPTH_BUFFER_BIT);
            
            pointShadowShader->setMat4("lightSpaceMatrix", shadowTransforms[face]);
            
            // Render scene elements
            for(size_t i = 0; i < get_scene_element_model_count(); i++) {
                const SceneElementModel& model = get_scene_element_model(i);
                model.Draw(*pointShadowShader);
            }
            
            // Render interactable models
            for(size_t i = 0; i < get_interactable_model_count(); i++) {
                const InteractableModel& model = get_interactable_model(i);
                model.Draw(*pointShadowShader);
            }
            
            // Render skeletal model (interactor)
            if(is_interactor_model_available() && skeletalPointShadowShader) {
                skeletalPointShadowShader->use();
                skeletalPointShadowShader->setMat4("lightSpaceMatrix", shadowTransforms[face]);
                skeletalPointShadowShader->setVec3("lightPos", pointLight.position);
                skeletalPointShadowShader->setFloat("far_plane", far_plane);
                skeletalPointShadowShader->setMat4("model", model);
                InteractorModel* interactorModel = get_interactor_model();
                if(interactorModel) {
                    interactorModel->Draw(*skeletalPointShadowShader);
                }
            }
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, render_context.screen_width, render_context.screen_height);
    }

    if(sceneElementShader){
        
        sceneElementShader->use();

        // Set polygon mode based on wireframe setting
        // Shadows must be rendered solid, but main scene can be wireframe
        if (wireframe_mode) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        sceneElementShader->setMat4("view", view);
        sceneElementShader->setMat4("projection", projection);
        
        // Upload directional light shadow settings
        sceneElementShader->setMat4("directionalLightSpaceMatrix", directionalLightSpaceMatrix);
        sceneElementShader->setBool("directionalLightCastsShadow", directionalLightCastsShadow);
        
        // Bind directional light shadow map (texture unit 19)
        glActiveTexture(GL_TEXTURE19);
        glBindTexture(GL_TEXTURE_2D, render_context.shadow_tex[DIRECTIONAL_LIGHT_INDEX]);
        sceneElementShader->setInt("directionalShadowMap", 19);
        
        // Upload light space matrices for all spotlights
        for(int i = 0; i < numShadowCastingSpotLights; i++) {
            std::string uniformName = "lightSpaceMatrices[" + std::to_string(i) + "]";
            sceneElementShader->setMat4(uniformName, spotLightSpaceMatrices[i]);
        }
        sceneElementShader->setInt("numActiveShadowCastingSpotLights", numShadowCastingSpotLights);
        sceneElementShader->setInt("numActiveShadowCastingPointLights", numShadowCastingPointLights);
        
        // Bind all spotlight shadow maps (texture units 20-29)
        // Offset to avoid collision with PBR material textures (0-5)
        for(int i = 0; i < 10; i++) {
            glActiveTexture(GL_TEXTURE20 + i);
            glBindTexture(GL_TEXTURE_2D, render_context.shadow_tex[SPOTLIGHT_START_INDEX + i]);
            std::string uniformName = "shadowMaps[" + std::to_string(i) + "]";
            sceneElementShader->setInt(uniformName, 20 + i);
        }
        
        // Bind all point light shadow cubemaps (texture units 30-39)
        for(int i = 0; i < 10; i++) {
            glActiveTexture(GL_TEXTURE30 + i);
            glBindTexture(GL_TEXTURE_CUBE_MAP, render_context.shadow_tex[POINTLIGHT_INDEX + i]);
            std::string uniformName = "shadowCubemaps[" + std::to_string(i) + "]";
            sceneElementShader->setInt(uniformName, 30 + i);
        }
        
        sceneElementShader->setFloat("far_plane", 25.0f);
        
        // Update and bind light data via UBO
        // The light setup should be done elsewhere, but we'll ensure UBO is updated
        lightManager.updateUBO();
        lightManager.bindUBO(4);

        // Set camera position for view direction calculation (unified camera position)
        sceneElementShader->setVec3("camPos", camera.Position);
        
        // Note: PBR material texture samplers are now handled dynamically in Mesh::Draw()
        // They will use texture units starting from 20 onwards

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
    
    // Render the skeletal model with full lighting
    if (skeletonShader && is_interactor_model_available()) {
        skeletonShader->use();
        skeletonShader->setMat4("model", model);
        skeletonShader->setMat4("view", view);
        skeletonShader->setMat4("projection", projection);
        
        // Upload directional light shadow settings
        skeletonShader->setMat4("directionalLightSpaceMatrix", directionalLightSpaceMatrix);
        skeletonShader->setBool("directionalLightCastsShadow", directionalLightCastsShadow);
        
        // Bind directional light shadow map (texture unit 19)
        glActiveTexture(GL_TEXTURE19);
        glBindTexture(GL_TEXTURE_2D, render_context.shadow_tex[DIRECTIONAL_LIGHT_INDEX]);
        skeletonShader->setInt("directionalShadowMap", 19);
        
        // Upload light space matrices for all spotlights
        for(int i = 0; i < numShadowCastingSpotLights; i++) {
            std::string uniformName = "lightSpaceMatrices[" + std::to_string(i) + "]";
            skeletonShader->setMat4(uniformName, spotLightSpaceMatrices[i]);
        }
        skeletonShader->setInt("numActiveShadowCastingSpotLights", numShadowCastingSpotLights);
        skeletonShader->setInt("numActiveShadowCastingPointLights", numShadowCastingPointLights);
        
        // Bind all spotlight shadow maps (texture units 20-29)
        for(int i = 0; i < 10; i++) {
            glActiveTexture(GL_TEXTURE20 + i);
            glBindTexture(GL_TEXTURE_2D, render_context.shadow_tex[SPOTLIGHT_START_INDEX + i]);
            std::string uniformName = "shadowMaps[" + std::to_string(i) + "]";
            skeletonShader->setInt(uniformName, 20 + i);
        }
        
        // Bind all point light shadow cubemaps (texture units 30-39)
        for(int i = 0; i < 10; i++) {
            glActiveTexture(GL_TEXTURE30 + i);
            glBindTexture(GL_TEXTURE_CUBE_MAP, render_context.shadow_tex[POINTLIGHT_INDEX + i]);
            std::string uniformName = "shadowCubemaps[" + std::to_string(i) + "]";
            skeletonShader->setInt(uniformName, 30 + i);
        }
        
        skeletonShader->setFloat("far_plane", 25.0f);
        
        // Update and bind light data via UBO (already updated, just bind)
        lightManager.bindUBO(4);

        // Set camera position for view direction calculation
        skeletonShader->setVec3("camPos", camera.Position);
        
        get_interactor_model()->Draw(*skeletonShader);
    }
    
    // Reset polygon mode to FILL for UI and debug rendering
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    // if(debug_shader){
       
    //    glDisable(GL_DEPTH_TEST);  // Disable depth test for 2D overlay
    //    render_texture_debug_rect(render_context.shadow_tex[0]);
    //    glEnable(GL_DEPTH_TEST);   // Re-enable depth test
       
    // }
    
    // Render collision geometry if enabled
    collisionVisualizer.renderCollisionGeometry(view, projection);
    
    // Render target position proxies if enabled The problem here is the proxy rendering and the render collision geometry is coupled for the first click.
    collisionVisualizer.renderTargetProxies(view, projection);
    
    // Render bone visualization if enabled
    collisionVisualizer.renderBoneVisualization(view, projection);
    
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
    return quitRequested;
}

void confirm_quit() {
    quitRequested = true;
}

void cancel_quit() {
    showQuitConfirmation = false;
}

bool is_quit_confirmation_shown() {
    return showQuitConfirmation;
}

void cleanup_rendering() {
    // Cleanup UI system
    cleanup_ui();
    
    // Cleanup collision visualizer
    collisionVisualizer.cleanup();
    
    delete skeletonShader;
    delete sceneElementShader;
    delete shadowShader;
    delete pointShadowShader;
    delete skeletalShadowShader;
    delete skeletalPointShadowShader;
    delete debug_shader;
    
    // Clean up shadow system
    glDeleteFramebuffers(1, &render_context.shadow_FBO);
    glDeleteTextures(21, render_context.shadow_tex);
    
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
            
            //TODO:: MOVE THIS CODE PART THAT HANDLES MOUSE CLICK RAY GENERATION IN ANOTHER FILE OR FUNCTION 
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
            
            // Include bone boxes for selection (always available when interactor model loaded)
            if (is_interactor_model_available()) {
                std::vector<Shape>& bone_boxes = get_bone_boxes();
                allShapes.insert(allShapes.end(), bone_boxes.begin(), bone_boxes.end());
            }
            
            // Find all objects the ray hits and sort them by distance (nearest first)
            hit_objects_list = intersect_ray_all(ray, allShapes);
            
            if (!hit_objects_list.empty()) {
                // Select the first item
                current_selection_index = 0;
                selected_model_id = hit_objects_list[current_selection_index];
                std::cout << "Selected object ID: " << selected_model_id 
                          << " (1 of " << hit_objects_list.size() << " objects at this location)" << std::endl;
                
                // If building IK chain and selected object is a bone, add it to the chain
                if (is_building_chain() && selected_model_id >= 20000) {
                    int bone_id = selected_model_id - 20000;
                    add_bone_to_current_chain(bone_id);
                }
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
            
            // If building IK chain and cycled object is a bone, add it to the chain
            if (is_building_chain() && selected_model_id >= 20000) {
                int bone_id = selected_model_id - 20000;
                add_bone_to_current_chain(bone_id);
            }
        }
    }
}

void process_input(GLFWwindow* window) {
    // ESC key disabled - use window close button instead
    
    const ApplicationState& app_state = get_application_state();
    
    // Only process camera movement if not in EDIT_SCENE mode
    if (app_mode != EDIT_SCENE) {
        // Check if Shift is held for 10x speed boost
        float speedMultiplier = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || 
                                  glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) ? 10.0f : 1.0f;
        float adjustedDeltaTime = app_state.deltaTime * speedMultiplier;
        
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, adjustedDeltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, adjustedDeltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, adjustedDeltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, adjustedDeltaTime);
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
    
    // Handle Delete key for removing last bone from the IK chain
    if (glfwGetKey(window, GLFW_KEY_DELETE) == GLFW_PRESS) {
        if (!deleteKeyPressed && is_building_chain()) {
            remove_last_bone_from_current_chain();
            deleteKeyPressed = true;
        }
    } else {
        deleteKeyPressed = false;
    }
}



static void window_close_callback(GLFWwindow* window) {
    // Don't close immediately, show confirmation dialog instead
    glfwSetWindowShouldClose(window, GLFW_FALSE);
    showQuitConfirmation = true;
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
