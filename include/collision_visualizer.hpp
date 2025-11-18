#ifndef COLLISION_VISUALIZER_HPP
#define COLLISION_VISUALIZER_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include "collision.hpp"
#include "Shader.h"

/**
 * @brief A collision visualizer class for rendering bounding boxes as wireframes
 * 
 * This class provides functionality to visualize collision geometry for debugging purposes.
 * It renders:
 * - Scene element AABBs as green wireframes
 * - Interactable element AABBs as red wireframes
 * 
 * The visualizer uses a single VAO/VBO setup for efficiency and transforms each
 * bounding box using the appropriate model matrix.
 */
class CollisionVisualizer {
private:
    // OpenGL resources
    GLuint VAO, VBO, EBO;
    Shader* bboxShader;
    
    // Cube vertices for wireframe bounding box
    static const float cubeVertices[8 * 3];
    static const unsigned int cubeIndices[24];
    
    bool isInitialized;
    
public:
    CollisionVisualizer();
    ~CollisionVisualizer();
    
    // Initialize OpenGL resources
    void init();
    
    // Cleanup OpenGL resources
    void cleanup();
    
    // Render all collision boxes with their model matrices
    void renderCollisionGeometry(const glm::mat4& view, const glm::mat4& projection);
    
    // Render target position proxies (points and boxes)
    void renderTargetProxies(const glm::mat4& view, const glm::mat4& projection);
    
    // Render bone visualization (red boxes between bones)
    void renderBoneVisualization(const glm::mat4& view, const glm::mat4& projection);
    
    // Enable/disable collision visualization
    static bool isEnabled;
    
    // Enable/disable target proxy visualization
    static bool showTargetProxies;
    
    // Enable/disable bone visualization
    static bool showBoneVisualization;
};

#endif