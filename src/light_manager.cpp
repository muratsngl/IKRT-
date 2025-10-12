#include "light_manager.hpp"
#include <iostream>
#include <cstring>

LightManager::LightManager() 
    : lightUBO(0), activePointLights(0), activeSpotLights(0), directionalLightActive(false) {
    // Initialize light data to zero
    memset(&lightData, 0, sizeof(LightUBO));
}

LightManager::~LightManager() {
    if (lightUBO != 0) {
        glDeleteBuffers(1, &lightUBO);
    }
}

void LightManager::initialize() {
    // Generate and setup the UBO
    glGenBuffers(1, &lightUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, lightUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(LightUBO), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    
    // Clear all light data
    clearAllLights();
    updateUBO();
}

void LightManager::setDirectionalLight(const DirectionalLight& light) {
    directionalLight = light;
    directionalLightActive = true;
    updateUBO(); // Changed from updateLightDataFromStructs()
}

void LightManager::enableDirectionalLight(bool enable) {
    directionalLightActive = enable;
    updateUBO(); // Changed from updateLightDataFromStructs()
}

int LightManager::addPointLight(const PointLight& light) {
    if (activePointLights >= MAX_POINT_LIGHTS) {
        std::cerr << "Maximum number of point lights reached!" << std::endl;
        return -1;
    }
    
    pointLights[activePointLights] = light;
    int index = activePointLights;
    activePointLights++;
    updateUBO(); // Changed from updateLightDataFromStructs()
    return index;
}

void LightManager::removePointLight(int index) {
    if (index < 0 || index >= activePointLights) {
        std::cerr << "Invalid point light index: " << index << std::endl;
        return;
    }
    
    // Shift lights down to fill the gap
    for (int i = index; i < activePointLights - 1; i++) {
        pointLights[i] = pointLights[i + 1];
    }
    activePointLights--;
    updateUBO(); // Changed from updateLightDataFromStructs()
}

void LightManager::updatePointLight(int index, const PointLight& light) {
    if (index < 0 || index >= activePointLights) {
        std::cerr << "Invalid point light index: " << index << std::endl;
        return;
    }
    
    pointLights[index] = light;
    updateUBO(); // Changed from updateLightDataFromStructs()
}

int LightManager::addSpotLight(const SpotLight& light) {
    if (activeSpotLights >= MAX_SPOT_LIGHTS) {
        std::cerr << "Maximum number of spot lights reached!" << std::endl;
        return -1;
    }
    
    spotLights[activeSpotLights] = light;
    int index = activeSpotLights;
    activeSpotLights++;
    updateUBO(); // Changed from updateLightDataFromStructs()
    return index;
}

void LightManager::removeSpotLight(int index) {
    if (index < 0 || index >= activeSpotLights) {
        std::cerr << "Invalid spot light index: " << index << std::endl;
        return;
    }
    
    // Shift lights down to fill the gap
    for (int i = index; i < activeSpotLights - 1; i++) {
        spotLights[i] = spotLights[i + 1];
    }
    activeSpotLights--;
    updateUBO(); // Changed from updateLightDataFromStructs()
}

void LightManager::updateSpotLight(int index, const SpotLight& light) {
    if (index < 0 || index >= activeSpotLights) {
        std::cerr << "Invalid spot light index: " << index << std::endl;
        return;
    }
    
    spotLights[index] = light;
    updateUBO(); // Changed from updateLightDataFromStructs()
}

void LightManager::updateLightDataFromStructs() {
    // Update directional light
    if (directionalLightActive) {
        lightData.directionalLightDirection = directionalLight.direction;
        lightData.directionalLightColor = directionalLight.color;
        lightData.directionalLightIntensity = directionalLight.intensity;
    } else {
        lightData.directionalLightDirection = glm::vec3(0.0f);
        lightData.directionalLightColor = glm::vec3(0.0f);
        lightData.directionalLightIntensity = 0.0f;
    }
    
    // Copy point lights directly to UBO array
    for (int i = 0; i < MAX_POINT_LIGHTS; i++) {
        if (i < activePointLights) {
            lightData.pointLights[i] = pointLights[i];
        } else {
            // Clear unused slots
            lightData.pointLights[i].position = glm::vec3(0.0f);
            lightData.pointLights[i].color = glm::vec3(0.0f);
            lightData.pointLights[i].intensity = 0.0f;
            lightData.pointLights[i]._padding = 0.0f;
        }
    }
    
    // Copy spot lights directly to UBO array
    for (int i = 0; i < MAX_SPOT_LIGHTS; i++) {
        if (i < activeSpotLights) {
            lightData.spotLights[i] = spotLights[i];
        } else {
            // Clear unused slots
            lightData.spotLights[i].position = glm::vec3(0.0f);
            lightData.spotLights[i].direction = glm::vec3(0.0f);
            lightData.spotLights[i].color = glm::vec3(0.0f);
            lightData.spotLights[i].intensity = 0.0f;
            lightData.spotLights[i].innerCone = 0.0f;
            lightData.spotLights[i].outerCone = 0.0f;
        }
    }
    
    // Update light counts
    lightData.numPointLights = activePointLights;
    lightData.numSpotLights = activeSpotLights;
}

void LightManager::updateUBO() {
    updateLightDataFromStructs();
    
    glBindBuffer(GL_UNIFORM_BUFFER, lightUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(LightUBO), &lightData);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void LightManager::bindUBO(int bindingPoint) {
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, lightUBO);
}

void LightManager::clearAllLights() {
    activePointLights = 0;
    activeSpotLights = 0;
    directionalLightActive = false;
    
    // Clear the structs
    memset(&directionalLight, 0, sizeof(DirectionalLight));
    memset(pointLights, 0, sizeof(pointLights));
    memset(spotLights, 0, sizeof(spotLights));
    
    updateUBO(); // Changed from updateLightDataFromStructs()
}