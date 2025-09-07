#include "include/render_setup.hpp"
#include "include/model_loader.hpp"
#include "include/application_logic.hpp"
#include "include/shared_memory.hpp"
#include "include/Camera.h"
#include "include/Shader.h"
#include "include/model_bones.h" 
#include "include/application_logic.hpp"// Include the Model class definition
#include "include/collision_visualizer.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

static RenderContext render_context;
static Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

static Shader* skeletonShader = nullptr;
static Shader* sceneElementShader = nullptr;
static CollisionVisualizer* collisionVisualizer = nullptr;

// Mouse and timing variables
static float lastX = 1240.0f / 2.0f;
static float lastY = 720.0f / 2.0f;
static bool firstMouse = true;

// Add static variables to track the key state
static bool spaceKeyPressed = false;

bool init_rendering() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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
    glfwSetScrollCallback(render_context.window, scroll_callback);
    glfwSetInputMode(render_context.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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
    
    return true;
}

void load_shaders() {
    skeletonShader = new Shader("assets/shaders/skeletal.vs", "assets/shaders/skeletal.fs");
    sceneElementShader = new Shader("assets/shaders/model.vert", "assets/shaders/model.frag");
    
    // Initialize collision visualizer
    collisionVisualizer = new CollisionVisualizer();
    collisionVisualizer->init();

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
    
 
    
   
   
}

void render_frame() {
    // Clear the screen
    glClearColor(0.8f, 0.4f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Set up matrices
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), 
                                          (float)render_context.screen_width / 
                                          (float)render_context.screen_height, 
                                          0.1f, 100.0f);
    glm::mat4 model = glm::mat4(1.0f);
    
    
    
    
    // Update uniform buffer with bone transforms
    //Room for optimization this could be done in a single memory access;
    const InteractorModelData& model_data = get_interactor_model_data();
    glBindBuffer(GL_UNIFORM_BUFFER, render_context.orcunUBO);
    for (short i = 0; i < model_data.bind_pose_matrices.size(); i++) {
        if (i < model_data.bind_pose_matrices.size()) {
            glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(glm::mat4), 
                          sizeof(glm::mat4), 
                          glm::value_ptr(model_data.bind_pose_matrices[i]));
        }
    }

    // Render the skeletal model
    if (skeletonShader) {
        skeletonShader->use();
        skeletonShader->setMat4("model", model);
        skeletonShader->setMat4("view", view);
        skeletonShader->setMat4("projection", projection);
        get_interactor_model()->Draw(*skeletonShader);
        
    }

    if(sceneElementShader){
        
        
        sceneElementShader->use();
         // Add quaternion rotations for model transformations THIS WAS MADE FOR 
        // glm::quat rotationX = glm::angleAxis(glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
        // glm::quat rotationZ = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, -1.0f));
        
    // Apply the rotations to the model matrix
        sceneElementShader->setMat4("model", model);

        sceneElementShader->setMat4("view", view);
        sceneElementShader->setMat4("projection", projection);


        for(size_t i = 0;i<get_scene_element_model_count();i++){
            const SceneElementModel& model = get_scene_element_model(i);
            model.Draw(*sceneElementShader);
            
        }
        
        for(size_t i = 0;i<get_interactable_model_count();i++){
            const InteractableModel& model = get_interactable_model(i);
            model.Draw(*sceneElementShader);
            
        }
      
        
    }
    
    // Render collision boxes at the end of draw calls
    if (collisionVisualizer) {
        extern std::vector<Shape> scene_element_boxes;
        
        // Create dynamic boxes from current bone positions
        std::vector<Shape> dynamicBoxes;
        const InteractorModelData& model_data = get_interactor_model_data();
        
        // Create bone OBBs for visualization
        auto createBoneOBB = [&](int boneIndex1, int boneIndex2, float halfExtentXZ) -> Shape {
            Shape boneShape;
            boneShape.type = OBB;

            glm::vec3 pos1 = model_data.bind_pose_matrices[boneIndex1] * glm::vec4(model_data.bind_pose_positions_original[boneIndex1], 1.0f);
            glm::vec3 pos2 = model_data.bind_pose_matrices[boneIndex2] * glm::vec4(model_data.bind_pose_positions_original[boneIndex2], 1.0f);

            glm::vec3 center = (pos1 + pos2) * 0.5f;
            glm::vec3 direction = pos2 - pos1;
            float length = glm::length(direction);
            
            if (length > 0.0001f) {
                direction = glm::normalize(direction);
            } else {
                direction = glm::vec3(1, 0, 0);
            }
            
            // Create a proper rotation matrix where the bone direction is the Y-axis (up)
            glm::vec3 up = direction;  // Bone direction becomes the up vector
            glm::vec3 forward = glm::vec3(0, 0, 1);  // Default forward
            
            // If bone direction is too close to forward, use a different forward
            if (abs(glm::dot(up, forward)) > 0.99f) {
                forward = glm::vec3(1, 0, 0);
            }
            
            glm::vec3 right = glm::normalize(glm::cross(up, forward));
            forward = glm::normalize(glm::cross(right, up));
            
            // Build rotation matrix: right=X, up=Y, forward=Z
            glm::mat3 rotMatrix(right, up, forward);
            glm::quat rotation = glm::quat_cast(rotMatrix);
            
            boneShape.obb.center = center;
            boneShape.obb.rotation = rotation;
            // Half extents: parameterized width/depth (X,Z), length along bone direction (Y)
            boneShape.obb.halfExtents = glm::vec3(halfExtentXZ, length * 0.5f, halfExtentXZ);
            
            return boneShape;
        };
        
        // Add bone OBBs for all limbs
    for (size_t i = 1; i < model_data.right_arm_indices.size(); i++) {
        dynamicBoxes.push_back(createBoneOBB(model_data.right_arm_indices[i - 1], model_data.right_arm_indices[i], 0.085f));
    }
    for (size_t i = 1; i < model_data.left_arm_indices.size(); i++) {
        dynamicBoxes.push_back(createBoneOBB(model_data.left_arm_indices[i - 1], model_data.left_arm_indices[i], 0.085f));
    }
    for (size_t i = 1; i < model_data.right_leg_indices.size(); i++) {
        dynamicBoxes.push_back(createBoneOBB(model_data.right_leg_indices[i - 1], model_data.right_leg_indices[i], 0.1f));
    }
    for (size_t i = 1; i < model_data.left_leg_indices.size(); i++) {
        dynamicBoxes.push_back(createBoneOBB(model_data.left_leg_indices[i - 1], model_data.left_leg_indices[i], 0.1f));
    }

    for (size_t i = 1; i < model_data.right_thumb_indices.size(); i++) {
        dynamicBoxes.push_back(createBoneOBB(model_data.right_thumb_indices[i - 1], model_data.right_thumb_indices[i], 0.015f));
    }
    for (size_t i = 1; i < model_data.right_index_indices.size(); i++) {
        dynamicBoxes.push_back(createBoneOBB(model_data.right_index_indices[i - 1], model_data.right_index_indices[i], 0.015f));
    }
    for (size_t i = 1; i < model_data.right_middle_indices.size(); i++) {
        dynamicBoxes.push_back(createBoneOBB(model_data.right_middle_indices[i - 1], model_data.right_middle_indices[i], 0.015f));
    }
    for (size_t i = 1; i < model_data.right_ring_indices.size(); i++) {
        dynamicBoxes.push_back(createBoneOBB(model_data.right_ring_indices[i - 1], model_data.right_ring_indices[i], 0.015f));
    }
    for (size_t i = 1; i < model_data.right_pinky_indices.size(); i++) {
        dynamicBoxes.push_back(createBoneOBB(model_data.right_pinky_indices[i - 1], model_data.right_pinky_indices[i], 0.015f));
    }

    for (size_t i = 1; i < model_data.left_thumb_indices.size(); i++) {
        dynamicBoxes.push_back(createBoneOBB(model_data.left_thumb_indices[i - 1], model_data.left_thumb_indices[i], 0.015f));
    }
    for (size_t i = 1; i < model_data.left_index_indices.size(); i++) {
        dynamicBoxes.push_back(createBoneOBB(model_data.left_index_indices[i - 1], model_data.left_index_indices[i], 0.015f));
    }
    for (size_t i = 1; i < model_data.left_middle_indices.size(); i++) {
        dynamicBoxes.push_back(createBoneOBB(model_data.left_middle_indices[i - 1], model_data.left_middle_indices[i], 0.02f));
    }
    for (size_t i = 1; i < model_data.left_ring_indices.size(); i++) {
        dynamicBoxes.push_back(createBoneOBB(model_data.left_ring_indices[i - 1], model_data.left_ring_indices[i], 0.02f));
    }
    for (size_t i = 1; i < model_data.left_pinky_indices.size(); i++) {
        dynamicBoxes.push_back(createBoneOBB(model_data.left_pinky_indices[i - 1], model_data.left_pinky_indices[i], 0.02f));
    }

        // Update and render collision boxes
        collisionVisualizer->updateBoundingBoxes(scene_element_boxes, dynamicBoxes);
        collisionVisualizer->render(view, projection);
    }
    
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
    delete skeletonShader;
    delete sceneElementShader;
    
    if (collisionVisualizer) {
        delete collisionVisualizer;
        collisionVisualizer = nullptr;
    }
    
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

// Callback implementations
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
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
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void process_input(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    const ApplicationState& app_state = get_application_state();
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, app_state.deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, app_state.deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, app_state.deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, app_state.deltaTime);
    //DEBUG: CURRENTLY TOGGLES ROOT LOCK RANDOMLY BECAUSE OF BUTTON DEBOUNCING//SOLVED WITH SCHMIDT TRIGGER
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        if (!spaceKeyPressed) { // Key was not previously pressed
            get_finger_data().rootLock = !get_finger_data().rootLock; // Toggle root lock
            std::cout << "rootLock toggled to: " << get_finger_data().rootLock << std::endl;
            spaceKeyPressed = true; // Mark the key as pressed
        }
        } else {
            spaceKeyPressed = false; // Reset the key state when released
        }


  
}
