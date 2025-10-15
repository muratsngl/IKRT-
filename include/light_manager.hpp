#pragma once
#include <glm/glm.hpp>
#include <GL/glew.h>
#include "light_types.hpp"

// Maximum number of lights supported
constexpr int MAX_POINT_LIGHTS = 10;
constexpr int MAX_SPOT_LIGHTS = 10;

// UBO structure that matches the shader layout exactly
//std140 alignment rules
struct alignas(16) LightUBO {
    // Directional Light (32 bytes)
    glm::vec3 directionalLightDirection;    // offset 0
    float     directionalLightIntensity;    // offset 12
    glm::vec3 directionalLightColor;        // offset 16
    float     _padding1;                    // offset 28
    
    // Light Arrays
    PointLight pointLights[MAX_POINT_LIGHTS];     // offset 32, size 320 (10 * 32 bytes)
    SpotLight  spotLights[MAX_SPOT_LIGHTS];       // offset 352, size 480 (10 * 48 bytes)
    
    // Light Counts
    int numPointLights;                     // offset 832
    int numSpotLights;                      // offset 836
};

// Light Manager class to handle all lighting operations
class LightManager {
private:
    GLuint lightUBO;
    LightUBO lightData;
    
    // Individual light storage
    DirectionalLight directionalLight;
    PointLight pointLights[MAX_POINT_LIGHTS];
    SpotLight spotLights[MAX_SPOT_LIGHTS];
    
    int activePointLights;
    int activeSpotLights;
    bool directionalLightActive;

public:
    LightManager();
    ~LightManager();
    
    // Initialize the UBO
    void initialize();
    
    // Directional light management
    void setDirectionalLight(const DirectionalLight& light);
    void enableDirectionalLight(bool enable);
    bool isDirectionalLightActive() const { return directionalLightActive; }
    DirectionalLight& getDirectionalLight() { return directionalLight; }
    
    // Point light management
    int addPointLight(const PointLight& light);
    void removePointLight(int index);
    void updatePointLight(int index, const PointLight& light);
    PointLight& getPointLight(int index) { return pointLights[index]; }
    int getActivePointLights() const { return activePointLights; }
    
    // Spot light management
    int addSpotLight(const SpotLight& light);
    void removeSpotLight(int index);
    void updateSpotLight(int index, const SpotLight& light);
    SpotLight& getSpotLight(int index) { return spotLights[index]; }
    int getActiveSpotLights() const { return activeSpotLights; }
    
    // Update and bind UBO
    void updateUBO();
    void bindUBO(int bindingPoint = 4);
    
    // Utility functions
    void clearAllLights();
    GLuint getUBOHandle() const { return lightUBO; }
    
private:
    void updateLightDataFromStructs();
};