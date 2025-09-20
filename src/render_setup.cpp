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

// ImGui includes
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "include/UI.hpp"

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
    
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    // Setup Dear ImGui style with green theme and transparency
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Set alpha/transparency for the entire UI
    style.Alpha = 0.85f;  // Slightly transparent
    
    // Set window background to transparent green
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.2f, 0.1f, 0.8f);  // Dark green with transparency
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.3f, 0.15f, 0.9f);  // Slightly lighter green for title
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.2f, 0.4f, 0.2f, 0.95f);  // Active title
    
    // Header colors (collapsible sections)
    style.Colors[ImGuiCol_Header] = ImVec4(0.2f, 0.4f, 0.2f, 0.8f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.5f, 0.25f, 0.9f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.3f, 0.6f, 0.3f, 1.0f);
    
    // Button colors
    style.Colors[ImGuiCol_Button] = ImVec4(0.2f, 0.4f, 0.2f, 0.8f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.6f, 0.3f, 0.9f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.4f, 0.7f, 0.4f, 1.0f);
    
    // Frame colors (input fields, etc.)
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.25f, 0.15f, 0.7f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.2f, 0.35f, 0.2f, 0.8f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.45f, 0.25f, 0.9f);
    
    // Text colors
    style.Colors[ImGuiCol_Text] = ImVec4(0.9f, 1.0f, 0.9f, 1.0f);  // Light green text
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.5f, 0.7f, 0.5f, 0.8f);
    
    // Border colors
    style.Colors[ImGuiCol_Border] = ImVec4(0.3f, 0.5f, 0.3f, 0.5f);
    
    // Popup/modal colors
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.1f, 0.2f, 0.1f, 0.9f);
    
    // Scrollbar colors
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.1f, 0.2f, 0.1f, 0.5f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3f, 0.5f, 0.3f, 0.8f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.4f, 0.6f, 0.4f, 0.9f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.5f, 0.7f, 0.5f, 1.0f);
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(render_context.window, true);
    ImGui_ImplOpenGL3_Init("#version 460");
    
    return true;
}

void load_shaders() {
    skeletonShader = new Shader("assets/shaders/skeletal.vs", "assets/shaders/skeletal.fs");
    sceneElementShader = new Shader("assets/shaders/pbr.vs", "assets/shaders/pbr.fs");
    
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
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_MULTISAMPLE); // Enable multisampling for anti-aliasing
    
    // Set up matrices
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), 
                                          (float)render_context.screen_width / 
                                          (float)render_context.screen_height, 
                                          0.1f, 100.0f);
    glm::mat4 model = glm::mat4(1.0f);
    
   
    
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
         // Add quaternion rotations for model transformations THIS WAS MADE FOR 
        // glm::quat rotationX = glm::angleAxis(glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
        // glm::quat rotationZ = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, -1.0f));
        
    // Apply the rotations to the model matrix
        sceneElementShader->setMat4("model", model);

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
            glm::vec3(300.0f, 250.0f, 200.0f),  // Warm white light
            glm::vec3(250.0f, 300.0f, 300.0f),  // Cool white light
            glm::vec3(300.0f, 200.0f, 250.0f),  // Slightly magenta light
            glm::vec3(200.0f, 300.0f, 250.0f)   // Slightly green light
        };

        // Set light uniforms
        for (int i = 0; i < 4; ++i) {
            sceneElementShader->setVec3("lightPositions[" + std::to_string(i) + "]", lightPositions[i]);
            sceneElementShader->setVec3("lightColors[" + std::to_string(i) + "]", lightColors[i]);
        }

        // Set camera position for view direction calculation
        sceneElementShader->setVec3("camPos", camera.Position);

        // Note: PBR material texture samplers are now handled dynamically in Mesh::Draw()


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
    if (CollisionVisualizer::getEnabled()) {
        // Get collision boxes from model loading phase
        std::vector<Shape> sceneBoxes = scene_element_boxes;  // Scene element boxes (red)
        std::vector<Shape> interactableBoxes = interactable_element_boxes;  // Interactable boxes (green)
        
        // Create dynamic boxes from current bone positions
        std::vector<Shape> dynamicBoxes;
        
        if (is_interactor_model_available()) {
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
    
        } // End of interactor model availability check

        // Update and render collision boxes
        collisionVisualizer->updateBoundingBoxes(sceneBoxes, interactableBoxes, dynamicBoxes);
        collisionVisualizer->render(view, projection);
    }
    
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // Call your model loader widget here
    RenderModelLoaderWidget();
    
    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
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
    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
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
