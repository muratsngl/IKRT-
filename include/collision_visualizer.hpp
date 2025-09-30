#ifndef COLLISION_VISUALIZER_HPP
#define COLLISION_VISUALIZER_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include "collision.hpp"
#include "Shader.h"

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
    
    // Enable/disable collision visualization
    static bool isEnabled;
};

#endif