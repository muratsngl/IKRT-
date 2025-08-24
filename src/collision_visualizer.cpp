#include "collision_visualizer.hpp"
#include <iostream>

CollisionVisualizer::CollisionVisualizer() 
    : VAO_AABB(0), VBO_AABB(0), EBO_AABB(0),
      VAO_OBB(0), VBO_OBB(0), EBO_OBB(0),
      VAO_Sphere(0), VBO_Sphere(0), EBO_Sphere(0),
      wireframeShader(nullptr) {
}

CollisionVisualizer::~CollisionVisualizer() {
    cleanup();
}

void CollisionVisualizer::init() {
    createWireframeShader();
    setupAABBGeometry();
    setupOBBGeometry();
    setupSphereGeometry();
}

void CollisionVisualizer::createWireframeShader() {
    // Use the existing Shader class to load from files
    wireframeShader = new Shader("assets/shaders/bbox_visualize.vs", "assets/shaders/bbox_visualize.fs");
}

void CollisionVisualizer::setupAABBGeometry() {
    // Unit cube vertices (will be transformed to actual AABB)
    std::vector<glm::vec3> vertices = {
        // Bottom face
        glm::vec3(-0.5f, -0.5f, -0.5f),
        glm::vec3( 0.5f, -0.5f, -0.5f),
        glm::vec3( 0.5f, -0.5f,  0.5f),
        glm::vec3(-0.5f, -0.5f,  0.5f),
        // Top face
        glm::vec3(-0.5f,  0.5f, -0.5f),
        glm::vec3( 0.5f,  0.5f, -0.5f),
        glm::vec3( 0.5f,  0.5f,  0.5f),
        glm::vec3(-0.5f,  0.5f,  0.5f)
    };
    
    // Wireframe indices
    std::vector<unsigned int> indices = {
        // Bottom face
        0, 1, 1, 2, 2, 3, 3, 0,
        // Top face
        4, 5, 5, 6, 6, 7, 7, 4,
        // Vertical edges
        0, 4, 1, 5, 2, 6, 3, 7
    };
    
    glGenVertexArrays(1, &VAO_AABB);
    glGenBuffers(1, &VBO_AABB);
    glGenBuffers(1, &EBO_AABB);
    
    glBindVertexArray(VAO_AABB);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO_AABB);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_AABB);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

void CollisionVisualizer::setupOBBGeometry() {
    // Same as AABB - unit cube that will be transformed
    setupAABBGeometry(); // Reuse the same geometry
    VAO_OBB = VAO_AABB;
    VBO_OBB = VBO_AABB;
    EBO_OBB = EBO_AABB;
}

void CollisionVisualizer::setupSphereGeometry() {
    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> indices;
    
    // Create a wireframe sphere with latitude/longitude lines
    const int latitudeLines = 8;
    const int longitudeLines = 16;
    const float radius = 1.0f;
    
    // Generate vertices
    for (int i = 0; i <= latitudeLines; ++i) {
        float theta = i * glm::pi<float>() / latitudeLines;
        for (int j = 0; j <= longitudeLines; ++j) {
            float phi = j * 2.0f * glm::pi<float>() / longitudeLines;
            
            float x = radius * sin(theta) * cos(phi);
            float y = radius * cos(theta);
            float z = radius * sin(theta) * sin(phi);
            
            vertices.push_back(glm::vec3(x, y, z));
        }
    }
    
    // Generate indices for wireframe
    for (int i = 0; i < latitudeLines; ++i) {
        for (int j = 0; j < longitudeLines; ++j) {
            int current = i * (longitudeLines + 1) + j;
            int next = current + longitudeLines + 1;
            
            // Horizontal lines
            indices.push_back(current);
            indices.push_back(current + 1);
            
            // Vertical lines
            if (i < latitudeLines) {
                indices.push_back(current);
                indices.push_back(next);
            }
        }
    }
    
    glGenVertexArrays(1, &VAO_Sphere);
    glGenBuffers(1, &VBO_Sphere);
    glGenBuffers(1, &EBO_Sphere);
    
    glBindVertexArray(VAO_Sphere);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Sphere);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_Sphere);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

void CollisionVisualizer::updateBoundingBoxes(const std::vector<Shape>& staticBoxes, 
                                             const std::vector<Shape>& dynamicBoxes) {
    aabbTransforms.clear();
    obbTransforms.clear();
    sphereTransforms.clear();
    sphereRadii.clear();
    
    // Process static boxes
    for (const auto& shape : staticBoxes) {
        switch (shape.type) {
            case AABB:
                aabbTransforms.push_back(createAABBTransform(shape.aabb));
                break;
            case OBB:
                obbTransforms.push_back(createOBBTransform(shape.obb));
                break;
            case SPHERE:
                sphereTransforms.push_back(createSphereTransform(shape.sphere));
                sphereRadii.push_back(shape.sphere.radius);
                break;
        }
    }
    
    // Process dynamic boxes
    for (const auto& shape : dynamicBoxes) {
        switch (shape.type) {
            case AABB:
                aabbTransforms.push_back(createAABBTransform(shape.aabb));
                break;
            case OBB:
                obbTransforms.push_back(createOBBTransform(shape.obb));
                break;
            case SPHERE:
                sphereTransforms.push_back(createSphereTransform(shape.sphere));
                sphereRadii.push_back(shape.sphere.radius);
                break;
        }
    }
}

void CollisionVisualizer::render(const glm::mat4& view, const glm::mat4& projection) {
    if (!wireframeShader) return;
    
    glDisable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    wireframeShader->use();
    wireframeShader->setMat4("view", view);
    wireframeShader->setMat4("projection", projection);
    
    renderAABBs(view, projection);
    renderOBBs(view, projection);
    renderSpheres(view, projection);
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_DEPTH_TEST);
}

void CollisionVisualizer::renderAABBs(const glm::mat4& view, const glm::mat4& projection) {
    wireframeShader->setVec3("color", glm::vec3(0.0f, 1.0f, 0.0f)); // Green for AABBs
    
    glBindVertexArray(VAO_AABB);
    for (const auto& transform : aabbTransforms) {
        wireframeShader->setMat4("model", transform);
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}

void CollisionVisualizer::renderOBBs(const glm::mat4& view, const glm::mat4& projection) {
    wireframeShader->setVec3("color", glm::vec3(1.0f, 0.0f, 0.0f)); // Red for OBBs
    
    glBindVertexArray(VAO_OBB);
    for (const auto& transform : obbTransforms) {
        wireframeShader->setMat4("model", transform);
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}

void CollisionVisualizer::renderSpheres(const glm::mat4& view, const glm::mat4& projection) {
    wireframeShader->setVec3("color", glm::vec3(0.0f, 0.0f, 1.0f)); // Blue for Spheres
    
    glBindVertexArray(VAO_Sphere);
    for (size_t i = 0; i < sphereTransforms.size(); ++i) {
        glm::mat4 scaleTransform = glm::scale(sphereTransforms[i], glm::vec3(sphereRadii[i]));
        wireframeShader->setMat4("model", scaleTransform);
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0); // Adjust count based on sphere geometry
    }
    glBindVertexArray(0);
}

glm::mat4 CollisionVisualizer::createAABBTransform(const Aabb& aabb) {
    glm::vec3 center = (aabb.min + aabb.max) * 0.5f;
    glm::vec3 size = aabb.max - aabb.min;
    
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), center);
    transform = glm::scale(transform, size);
    
    return transform;
}

glm::mat4 CollisionVisualizer::createOBBTransform(const Obb& obb) {
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), obb.center);
    transform = transform * glm::mat4_cast(obb.rotation);
    transform = glm::scale(transform, obb.halfExtents * 2.0f);
    
    return transform;
}

glm::mat4 CollisionVisualizer::createSphereTransform(const Sphere& sphere) {
    return glm::translate(glm::mat4(1.0f), sphere.center);
}

void CollisionVisualizer::cleanup() {
    if (VAO_AABB != 0) {
        glDeleteVertexArrays(1, &VAO_AABB);
        glDeleteBuffers(1, &VBO_AABB);
        glDeleteBuffers(1, &EBO_AABB);
    }
    
    if (VAO_Sphere != 0) {
        glDeleteVertexArrays(1, &VAO_Sphere);
        glDeleteBuffers(1, &VBO_Sphere);
        glDeleteBuffers(1, &EBO_Sphere);
    }
    
    if (wireframeShader) {
        delete wireframeShader;
        wireframeShader = nullptr;
    }
}
