#include "collision_visualizer.hpp"
#include "model_bones.h"
#include "bone_hierarchy.hpp"
#include "model_loader.hpp"
#include "render_setup.hpp"
#include "application_logic.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
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
            // Skip bone shapes (IDs >= 20000) - they're rendered separately
            if (shape.id >= 20000) continue;
            
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
            // Skip bone shapes (IDs >= 20000) - they're rendered separately
            if (shape.id >= 20000) continue;
            
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

bool CollisionVisualizer::showTargetProxies = false;

void CollisionVisualizer::renderTargetProxies(const glm::mat4& view, const glm::mat4& projection) {
    // Target proxies are independent of collision geometry rendering
    // Only check if initialized and if showTargetProxies is enabled
    if (!isInitialized || !bboxShader || !showTargetProxies) return;
    
    std::vector<Shape>& target_proxies = get_target_proxy_boxes();
    ApplicationState& app_state = get_application_state();
    
    if (target_proxies.empty()) return;
    
    bboxShader->use();
    bboxShader->setMat4("view", view);
    bboxShader->setMat4("projection", projection);
    
    glBindVertexArray(VAO);
    glLineWidth(2.0f);
    
    // Get actual target positions
    glm::vec3 targetPositions[4] = {
        app_state.targetPositionIndex,
        app_state.targetPositionMiddle,
        app_state.targetPositionRing,
        app_state.targetPositionPinky
    };
    
    // Small box size for visual representation at target position
    float visualBoxSize = 0.05f;
    
    // Render small boxes at the ACTUAL target positions
    for (size_t i = 0; i < 4; i++) {
        // Use bright cyan color for target proxies
        bboxShader->setVec3("boxColor", glm::vec3(0.0f, 1.0f, 1.0f));
        
        // Create small box at the exact target position
        glm::mat4 bboxTransform = glm::mat4(1.0f);
        bboxTransform = glm::translate(bboxTransform, targetPositions[i]);
        bboxTransform = glm::scale(bboxTransform, glm::vec3(visualBoxSize));
        
        bboxShader->setMat4("model", bboxTransform);
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    }
    
    glBindVertexArray(0);
    glLineWidth(1.0f);
}

bool CollisionVisualizer::showBoneVisualization = false;

void CollisionVisualizer::renderBoneVisualization(const glm::mat4& view, const glm::mat4& projection) {
    if (!showBoneVisualization || !isInitialized || !bboxShader) return;
    
    // Only render if interactor model is available
    if (!is_interactor_model_available()) return;
    
    InteractorModel* model = get_interactor_model();
    if (!model) return;
    
    const InteractorModelData& model_data = get_interactor_model_data();
    
    // Check if we have bone data
    if (model_data.bind_pose_positions.empty() || model_data.bind_pose_matrices.empty()) return;
    
    bboxShader->use();
    bboxShader->setMat4("view", view);
    bboxShader->setMat4("projection", projection);
    bboxShader->setVec3("color", glm::vec3(1.0f, 0.0f, 0.0f)); // Red color for bones
    
    glBindVertexArray(VAO);
    glLineWidth(2.5f);
    
    // Get bone hierarchy to determine parent-child relationships
    BoneHierarchy* hierarchy = model->getBoneHierarchy();
    if (!hierarchy || !hierarchy->isValid()) return;
    
    // Get all bones
    std::vector<BoneNode*> allBones;
    hierarchy->getAllBones(allBones);
    
    // Render a box between each bone and its parent
    for (const BoneNode* bone : allBones) {
        if (!bone->parent) continue; // Skip root bone (no parent)
        
        int childBoneId = bone->boneId;
        int parentBoneId = bone->parent->boneId;
        
        // Check valid indices
        if (childBoneId >= model_data.bind_pose_positions.size() || 
            parentBoneId >= model_data.bind_pose_positions.size()) continue;
        
        // Get bone positions in world space
        glm::vec3 childPos = model_data.bind_pose_matrices[childBoneId] * 
                            glm::vec4(model_data.bind_pose_positions[childBoneId], 1.0f);
        glm::vec3 parentPos = model_data.bind_pose_matrices[parentBoneId] * 
                             glm::vec4(model_data.bind_pose_positions[parentBoneId], 1.0f);
        
        // Calculate center and direction
        glm::vec3 center = (childPos + parentPos) * 0.5f;
        glm::vec3 direction = childPos - parentPos;
        float length = glm::length(direction);
        
        if (length < 0.001f) continue; // Skip if bones are too close
        
        direction = glm::normalize(direction);
        
        // Create rotation to align box with bone direction
        glm::vec3 up = direction;
        glm::vec3 forward = glm::vec3(0, 0, 1);
        
        if (fabs(glm::dot(up, forward)) > 0.99f) {
            forward = glm::vec3(1, 0, 0);
        }
        
        glm::vec3 right = glm::normalize(glm::cross(up, forward));
        forward = glm::normalize(glm::cross(right, up));
        
        glm::mat3 rotMatrix(right, up, forward);
        glm::quat rotation = glm::quat_cast(rotMatrix);
        
        // Create transformation matrix for the bone box
        glm::mat4 boneTransform = glm::mat4(1.0f);
        boneTransform = glm::translate(boneTransform, center);
        boneTransform *= glm::mat4_cast(rotation);
        
        // Scale: thickness proportional to bone length (adaptive sizing)
        float boneThickness = length * 0.15f; // Scale thickness with bone length
        boneThickness = glm::clamp(boneThickness, 0.01f, 0.3f); // Clamp to reasonable range
        boneTransform = glm::scale(boneTransform, glm::vec3(boneThickness, length, boneThickness));
        
        bboxShader->setMat4("model", boneTransform);
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    }
    
    // Render extended segments for leaf bones (so fingertips can be selected)
    for (const BoneNode* bone : allBones) {
        if (bone->children.empty() && bone->parent) {
            int boneId = bone->boneId;
            int parentBoneId = bone->parent->boneId;
            
            if (boneId >= model_data.bind_pose_positions.size() || 
                parentBoneId >= model_data.bind_pose_positions.size()) continue;
            
            // Get bone positions in world space
            glm::vec3 bonePos = model_data.bind_pose_matrices[boneId] * 
                               glm::vec4(model_data.bind_pose_positions[boneId], 1.0f);
            glm::vec3 parentPos = model_data.bind_pose_matrices[parentBoneId] * 
                                 glm::vec4(model_data.bind_pose_positions[parentBoneId], 1.0f);
            
            // Calculate direction and create virtual tip
            glm::vec3 direction = bonePos - parentPos;
            float segmentLength = glm::length(direction);
            
            if (segmentLength < 0.001f) continue;
            
            direction = glm::normalize(direction);
            float tipLength = segmentLength * 0.5f; // Extend 50% of parent-to-bone distance
            glm::vec3 virtualTip = bonePos + direction * tipLength;
            
            // Create box from bone to virtual tip
            glm::vec3 center = (bonePos + virtualTip) * 0.5f;
            float length = tipLength;
            
            // Create rotation
            glm::vec3 up = direction;
            glm::vec3 forward = glm::vec3(0, 0, 1);
            
            if (fabs(glm::dot(up, forward)) > 0.99f) {
                forward = glm::vec3(1, 0, 0);
            }
            
            glm::vec3 right = glm::normalize(glm::cross(up, forward));
            forward = glm::normalize(glm::cross(right, up));
            
            glm::mat3 rotMatrix(right, up, forward);
            glm::quat rotation = glm::quat_cast(rotMatrix);
            
            glm::mat4 tipTransform = glm::mat4(1.0f);
            tipTransform = glm::translate(tipTransform, center);
            tipTransform *= glm::mat4_cast(rotation);
            
            float tipThickness = segmentLength * 0.15f;
            tipThickness = glm::clamp(tipThickness, 0.05f, 0.3f);
            tipTransform = glm::scale(tipTransform, glm::vec3(tipThickness, length, tipThickness));
            
            bboxShader->setMat4("model", tipTransform);
            glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
        }
    }
    
    glBindVertexArray(0);
    glLineWidth(1.0f);
}