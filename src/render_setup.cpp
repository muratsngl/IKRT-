#include "include/render_setup.hpp"
#include "include/model_loader.hpp"
#include "include/application_logic.hpp"
#include "include/Camera.h"
#include "include/Shader.h"
#include "include/model_bones.h" // Include the Model class definition
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

static RenderContext render_context;
static Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
static Shader* generalPurposeShader = nullptr;
static Shader* snakeMeshShader = nullptr;
static Shader* skeletonShader = nullptr;

// Mouse and timing variables
static float lastX = 1240.0f / 2.0f;
static float lastY = 720.0f / 2.0f;
static bool firstMouse = true;

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
    generalPurposeShader = new Shader("assets/shaders/triangle.vs", "assets/shaders/triangle.fs");
    snakeMeshShader = new Shader("assets/shaders/snake_mesh.vs", "assets/shaders/snake_mesh.fs");
    skeletonShader = new Shader("assets/shaders/skeletal.vs", "assets/shaders/skeletal.fs");
}

void init_buffers() {
    GLbitfield flags = GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_WRITE_BIT;
    
    // Generate uniform buffer objects
    glGenBuffers(1, &render_context.snakeBoneUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, render_context.snakeBoneUBO);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, render_context.snakeBoneUBO);
    glBufferData(GL_UNIFORM_BUFFER, 320, nullptr, GL_DYNAMIC_READ);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glGenBuffers(1, &render_context.orcunUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, render_context.orcunUBO);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, render_context.orcunUBO);
    glBufferData(GL_UNIFORM_BUFFER, 1216, nullptr, GL_DYNAMIC_READ);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    
    // Initialize other buffers (guide points, snake, etc.)
    std::vector<float> guidePoints{0.0f, 0.0f, 0.0f};
    
    glGenVertexArrays(1, &render_context.guidePointsVAO);
    glBindVertexArray(render_context.guidePointsVAO);
    
    glGenBuffers(1, &render_context.guidePointsVBO);
    glBindBuffer(GL_ARRAY_BUFFER, render_context.guidePointsVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * guidePoints.size(), 
                 guidePoints.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

void render_frame() {
    // Clear the screen
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Set up matrices
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), 
                                          (float)render_context.screen_width / 
                                          (float)render_context.screen_height, 
                                          0.1f, 100.0f);
    glm::mat4 model = glm::mat4(1.0f);
    
    // Update uniform buffer with bone transforms
    const InteractorModelData& model_data = get_model_data();
    glBindBuffer(GL_UNIFORM_BUFFER, render_context.orcunUBO);
    for (short i = 0; i < 19; i++) {
        if (i < model_data.bind_pose_matrices.size()) {
            glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(glm::mat4), 
                          sizeof(glm::mat4), 
                          glm::value_ptr(model_data.bind_pose_matrices[i]));
        }
    }

    // Render the skeletal model
    if (skeletonShader) {
        skeletonShader->setMat4("model", model);
        skeletonShader->setMat4("view", view);
        skeletonShader->setMat4("projection", projection);
        get_model()->Draw(*skeletonShader);
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
    delete generalPurposeShader;
    delete snakeMeshShader;
    delete skeletonShader;
    
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
}
