#include "collision_visualizer.hpp"
#include "model_bones.h"
#include "render_setup.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

// Static member initialization
bool CollisionVisualizer::isEnabled = false;

// Cube vertices (8 corners of a unit cube centered at origin)
const float CollisionVisualizer::cubeVertices[8 * 3] = {
    // Bottom face
    -0.5f, -0.5f, -0.5f,  // 0
     0.5f, -0.5f, -0.5f,  // 1
     0.5f, -0.5f,  0.5f,  // 2
    -0.5f, -0.5f,  0.5f,  // 3
    // Top face
    -0.5f,  0.5f, -0.5f,  // 4
     0.5f,  0.5f, -0.5f,  // 5
     0.5f,  0.5f,  0.5f,  // 6
    -0.5f,  0.5f,  0.5f   // 7
};

// Wireframe cube indices (24 indices for 12 edges)
const unsigned int CollisionVisualizer::cubeIndices[24] = {
    // Bottom face edges
    0, 1,  1, 2,  2, 3,  3, 0,
    // Top face edges
    4, 5,  5, 6,  6, 7,  7, 4,
    // Vertical edges
    0, 4,  1, 5,  2, 6,  3, 7
};

CollisionVisualizer::CollisionVisualizer() 
    : VAO(0), VBO(0), EBO(0), bboxShader(nullptr), isInitialized(false) {
}

CollisionVisualizer::~CollisionVisualizer() {
    cleanup();
}

void CollisionVisualizer::init() {
    if (isInitialized) return;
    
    // Create and load shader
    try {
        bboxShader = new Shader("assets/shaders/bbox_visualize.vs", "assets/shaders/bbox_visualize.fs");
    } catch (const std::exception& e) {
        std::cerr << "Failed to create bbox shader: " << e.what() << std::endl;
        return;
    }
    
    // Generate VAO, VBO, and EBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    // Bind VAO
    glBindVertexArray(VAO);
    
    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    
    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);
    
    // Set vertex attributes (position only)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Unbind VAO
    glBindVertexArray(0);
    
    isInitialized = true;
    std::cout << "Collision visualizer initialized successfully" << std::endl;
}

void CollisionVisualizer::cleanup() {
    if (!isInitialized) return;
    
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (EBO != 0) {
        glDeleteBuffers(1, &EBO);
        EBO = 0;
    }
    
    delete bboxShader;
    bboxShader = nullptr;
    
    isInitialized = false;
}

void CollisionVisualizer::renderCollisionGeometry(const glm::mat4& view, const glm::mat4& projection) {
    if (!isEnabled || !isInitialized || !bboxShader) return;
    
    // Get model matrices for transforming collision boxes
    const std::vector<glm::mat4>& modelMatrices = get_model_matrices();
    
    // Enable wireframe mode and line drawing
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(2.0f);
    
    // Use bbox shader
    bboxShader->use();
    bboxShader->setMat4("view", view);
    bboxShader->setMat4("projection", projection);
    bboxShader->setVec3("color", glm::vec3(0.0f, 1.0f, 0.0f)); // Green wireframe
    
    glBindVertexArray(VAO);
    
    // Render scene element boxes (AABB type)
    for (const Shape& shape : scene_element_boxes) {
        if (shape.type == AABB) {
            // Get the model matrix for this shape
            unsigned int modelIndex = get_model_index_by_id(shape.id);
            if (modelIndex >= modelMatrices.size()) continue;
            
            const glm::mat4& modelMatrix = modelMatrices[modelIndex];
            
            // Calculate size and center from AABB
            glm::vec3 size = shape.aabb.max - shape.aabb.min;
            glm::vec3 center = (shape.aabb.min + shape.aabb.max) * 0.5f;
            
            // Create transformation matrix
            glm::mat4 bboxTransform = modelMatrix;
            bboxTransform = glm::translate(bboxTransform, center);
            bboxTransform = glm::scale(bboxTransform, size);
            
            bboxShader->setMat4("model", bboxTransform);
            glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
        }
    }
    
    // Render interactable element boxes (AABB type)
    bboxShader->setVec3("color", glm::vec3(1.0f, 0.0f, 0.0f)); // Red wireframe for interactable AABBs
    
    for (const Shape& shape : interactable_element_boxes) {
        if (shape.type == AABB) {
            // Get the model matrix for this shape
            unsigned int modelIndex = get_model_index_by_id(shape.id);
            if (modelIndex >= modelMatrices.size()) continue;
            
            const glm::mat4& modelMatrix = modelMatrices[modelIndex];
            
            // Calculate size and center from AABB
            glm::vec3 size = shape.aabb.max - shape.aabb.min;
            glm::vec3 center = (shape.aabb.min + shape.aabb.max) * 0.5f;
            
            // Create transformation matrix
            glm::mat4 bboxTransform = modelMatrix;
            bboxTransform = glm::translate(bboxTransform, center);
            bboxTransform = glm::scale(bboxTransform, size);
            
            bboxShader->setMat4("model", bboxTransform);
            glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
        }
    }
    
    glBindVertexArray(0);
    
    // Restore fill mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(1.0f);
}