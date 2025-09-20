#ifndef COLLISION_VISUALIZER_HPP
#define COLLISION_VISUALIZER_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "collision.hpp"
#include "Shader.h"

class CollisionVisualizer {
public:
    CollisionVisualizer();
    ~CollisionVisualizer();
    
    // Initialize the visualizer with shaders
    void init();
    
    // Update bounding boxes to visualize
    void updateBoundingBoxes(const std::vector<Shape>& staticBoxes, 
                           const std::vector<Shape>& dynamicBoxes);
    
    // Update bounding boxes with separate interactable boxes
    void updateBoundingBoxes(const std::vector<Shape>& staticBoxes, 
                           const std::vector<Shape>& interactableBoxes,
                           const std::vector<Shape>& dynamicBoxes);
    
    // Render all bounding boxes
    void render(const glm::mat4& view, const glm::mat4& projection);
    
    // Cleanup resources
    void cleanup();
    
    // Enable/disable collision visualization
    static void setEnabled(bool enabled) { isEnabled = enabled; }
    static bool getEnabled() { return isEnabled; }
    static void toggleEnabled() { isEnabled = !isEnabled; }
    
private:
    // OpenGL objects
    GLuint VAO_AABB, VBO_AABB, EBO_AABB;
    GLuint VAO_OBB, VBO_OBB, EBO_OBB;
    GLuint VAO_Sphere, VBO_Sphere, EBO_Sphere;
    
    // Shader for wireframe rendering
    Shader* wireframeShader;
    
    // Enable/disable flag (static so accessible globally)
    static bool isEnabled;
    
    // Vertex data for different primitive types
    std::vector<glm::vec3> aabbVertices;
    std::vector<unsigned int> aabbIndices;
    std::vector<glm::mat4> aabbTransforms;
    
    std::vector<glm::vec3> obbVertices;
    std::vector<unsigned int> obbIndices;
    std::vector<glm::mat4> obbTransforms;
    std::vector<glm::mat4> obbInteractableTransforms;  // Separate storage for interactable OBBs
    
    std::vector<glm::vec3> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    std::vector<glm::mat4> sphereTransforms;
    std::vector<float> sphereRadii;
    
    // Helper methods
    void setupAABBGeometry();
    void setupOBBGeometry();
    void setupSphereGeometry();
    void createWireframeShader();
    
    void renderAABBs(const glm::mat4& view, const glm::mat4& projection);
    void renderOBBs(const glm::mat4& view, const glm::mat4& projection);
    void renderInteractableOBBs(const glm::mat4& view, const glm::mat4& projection);
    void renderSpheres(const glm::mat4& view, const glm::mat4& projection);
    
    glm::mat4 createAABBTransform(const Aabb& aabb);
    glm::mat4 createOBBTransform(const Obb& obb);
    glm::mat4 createSphereTransform(const Sphere& sphere);
};

#endif
