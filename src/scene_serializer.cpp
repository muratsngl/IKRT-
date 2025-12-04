#include "include/scene_serializer.hpp"
#include "include/model_loader.hpp"
#include "include/render_setup.hpp"
#include "include/light_manager.hpp"
#include "include/model_bones.h"
#include "include/animation_manager.hpp"
#include "json.hpp"
#include <fstream>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

using json = nlohmann::json;

// Matrix utility implementations
namespace MatrixUtils {
    glm::vec3 extractPosition(const glm::mat4& matrix) {
        return glm::vec3(matrix[3]);
    }

    glm::quat extractRotation(const glm::mat4& matrix) {
        glm::vec3 scale;
        glm::quat rotation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(matrix, scale, rotation, translation, skew, perspective);
        return rotation;
    }

    glm::vec3 extractScale(const glm::mat4& matrix) {
        glm::vec3 scale;
        glm::quat rotation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(matrix, scale, rotation, translation, skew, perspective);
        return scale;
    }

    glm::mat4 composeMatrix(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale) {
        glm::mat4 matrix = glm::mat4(1.0f);
        matrix = glm::translate(matrix, position);
        matrix = matrix * glm::mat4_cast(rotation);
        matrix = glm::scale(matrix, scale);
        return matrix;
    }
}

// Scene serialization implementations
namespace SceneSerializer {
    
    bool saveScene(const std::string& filepath) {
        try {
            json scene;
            scene["version"] = "1.0";
            scene["scene_name"] = "Untitled Scene";
            
            // Serialize scene element models
            scene["models"]["scene_elements"] = json::array();
            size_t scene_count = get_scene_element_model_count();
            for (size_t i = 0; i < scene_count; i++) {
                const SceneElementModel& model = get_scene_element_model(i);
                json model_data;
                
                model_data["file_path"] = model.original_file_path;
                model_data["id"] = model.id;
                
                // Get model matrix and decompose
                glm::mat4 matrix = get_model_matrix_by_id(model.id);
                glm::vec3 pos = MatrixUtils::extractPosition(matrix);
                glm::quat rot = MatrixUtils::extractRotation(matrix);
                glm::vec3 scl = MatrixUtils::extractScale(matrix);
                
                model_data["transform"]["position"] = {pos.x, pos.y, pos.z};
                model_data["transform"]["rotation"] = {rot.w, rot.x, rot.y, rot.z};
                model_data["transform"]["scale"] = {scl.x, scl.y, scl.z};
                
                scene["models"]["scene_elements"].push_back(model_data);
            }
            
            // Serialize interactable models
            scene["models"]["interactables"] = json::array();
            size_t interactable_count = get_interactable_model_count();
            for (size_t i = 0; i < interactable_count; i++) {
                const InteractableModel& model = get_interactable_model(i);
                json model_data;
                
                model_data["file_path"] = model.original_file_path;
                model_data["id"] = model.id;
                
                // Get model matrix and decompose
                glm::mat4 matrix = get_model_matrix_by_id(model.id);
                glm::vec3 pos = MatrixUtils::extractPosition(matrix);
                glm::quat rot = MatrixUtils::extractRotation(matrix);
                glm::vec3 scl = MatrixUtils::extractScale(matrix);
                
                model_data["transform"]["position"] = {pos.x, pos.y, pos.z};
                model_data["transform"]["rotation"] = {rot.w, rot.x, rot.y, rot.z};
                model_data["transform"]["scale"] = {scl.x, scl.y, scl.z};
                
                scene["models"]["interactables"].push_back(model_data);
            }

            // Serialize interactor model (skeletal)
            if (is_interactor_model_available()) {
                InteractorModel* interactor = get_interactor_model();
                json interactor_data;
                interactor_data["file_path"] = interactor->original_file_path;
                
                // Save bone matrices
                const InteractorModelData& data = get_interactor_model_data();
                interactor_data["bone_matrices"] = json::array();
                
                for (const auto& matrix : data.bind_pose_matrices) {
                    json matrix_json = json::array();
                    const float* pSource = (const float*)glm::value_ptr(matrix);
                    for (int i = 0; i < 16; ++i) {
                        matrix_json.push_back(pSource[i]);
                    }
                    interactor_data["bone_matrices"].push_back(matrix_json);
                }
                
                // Also save bone positions if needed (optional but good for completeness)
                interactor_data["bone_positions"] = json::array();
                for (const auto& pos : data.bind_pose_positions) {
                    interactor_data["bone_positions"].push_back({pos.x, pos.y, pos.z});
                }

                scene["models"]["interactor"] = interactor_data;
            }
            
            // Serialize lighting
            LightManager& lightManager = get_light_manager();
            
            // Directional light
            if (lightManager.isDirectionalLightActive()) {
                DirectionalLight& dirLight = lightManager.getDirectionalLight();
                scene["lighting"]["directional"]["active"] = true;
                scene["lighting"]["directional"]["direction"] = {dirLight.direction.x, dirLight.direction.y, dirLight.direction.z};
                scene["lighting"]["directional"]["color"] = {dirLight.color.r, dirLight.color.g, dirLight.color.b};
                scene["lighting"]["directional"]["intensity"] = dirLight.intensity;
            } else {
                scene["lighting"]["directional"]["active"] = false;
            }
            
            // Point lights
            scene["lighting"]["point_lights"] = json::array();
            int numPointLights = lightManager.getActivePointLights();
            for (int i = 0; i < numPointLights; i++) {
                PointLight& light = lightManager.getPointLight(i);
                json light_data;
                light_data["position"] = {light.position.x, light.position.y, light.position.z};
                light_data["color"] = {light.color.r, light.color.g, light.color.b};
                light_data["intensity"] = light.intensity;
                scene["lighting"]["point_lights"].push_back(light_data);
            }
            
            // Spot lights
            scene["lighting"]["spot_lights"] = json::array();
            int numSpotLights = lightManager.getActiveSpotLights();
            for (int i = 0; i < numSpotLights; i++) {
                SpotLight& light = lightManager.getSpotLight(i);
                json light_data;
                light_data["position"] = {light.position.x, light.position.y, light.position.z};
                light_data["direction"] = {light.direction.x, light.direction.y, light.direction.z};
                light_data["color"] = {light.color.r, light.color.g, light.color.b};
                light_data["intensity"] = light.intensity;
                light_data["inner_cone"] = glm::degrees(acos(light.innerCone));
                light_data["outer_cone"] = glm::degrees(acos(light.outerCone));
                scene["lighting"]["spot_lights"].push_back(light_data);
            }
            
            // Write to file
            std::ofstream file(filepath);
            if (!file.is_open()) {
                std::cerr << "Failed to open file for writing: " << filepath << std::endl;
                return false;
            }
            
            file << scene.dump(4);  // Pretty print with 4-space indent
            file.close();
            
            std::cout << "Scene saved successfully to: " << filepath << std::endl;

            // Auto-save animations
            std::string animFilepath = filepath + ".anim";
            get_animation_manager().saveToFile(animFilepath);

            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Error saving scene: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool loadScene(const std::string& filepath) {
        try {
            // Read file
            std::ifstream file(filepath);
            if (!file.is_open()) {
                std::cerr << "Failed to open file for reading: " << filepath << std::endl;
                return false;
            }
            
            json scene;
            file >> scene;
            file.close();
            
            // Check version
            std::string version = scene.value("version", "unknown");
            if (version != "1.0") {
                std::cerr << "Warning: Scene file version mismatch. Expected 1.0, got " << version << std::endl;
            }
            
            // Clear current scene
            clearScene();
            
            // Load scene element models
            if (scene.contains("models") && scene["models"].contains("scene_elements")) {
                for (const auto& model_data : scene["models"]["scene_elements"]) {
                    std::string file_path = model_data["file_path"];
                    
                    // Load the model
                    if (load_scene_element_model(file_path.c_str())) {
                        // Get the newly loaded model (last in array)
                        const SceneElementModel& model = get_scene_element_model(get_scene_element_model_count() - 1);
                        
                        // Restore transform
                        auto pos_arr = model_data["transform"]["position"];
                        auto rot_arr = model_data["transform"]["rotation"];
                        auto scl_arr = model_data["transform"]["scale"];
                        
                        glm::vec3 position(pos_arr[0], pos_arr[1], pos_arr[2]);
                        glm::quat rotation(rot_arr[0], rot_arr[1], rot_arr[2], rot_arr[3]);
                        glm::vec3 scale(scl_arr[0], scl_arr[1], scl_arr[2]);
                        
                        glm::mat4 matrix = MatrixUtils::composeMatrix(position, rotation, scale);
                        set_model_matrix_by_id(model.id, matrix);
                        
                        std::cout << "Loaded scene element: " << file_path << std::endl;
                    } else {
                        std::cerr << "Failed to load scene element: " << file_path << std::endl;
                    }
                }
            }
            
            // Load interactable models
            if (scene.contains("models") && scene["models"].contains("interactables")) {
                for (const auto& model_data : scene["models"]["interactables"]) {
                    std::string file_path = model_data["file_path"];
                    
                    // Load the model
                    if (load_interactable_model(file_path.c_str())) {
                        // Get the newly loaded model (last in array)
                        const InteractableModel& model = get_interactable_model(get_interactable_model_count() - 1);
                        
                        // Restore transform
                        auto pos_arr = model_data["transform"]["position"];
                        auto rot_arr = model_data["transform"]["rotation"];
                        auto scl_arr = model_data["transform"]["scale"];
                        
                        glm::vec3 position(pos_arr[0], pos_arr[1], pos_arr[2]);
                        glm::quat rotation(rot_arr[0], rot_arr[1], rot_arr[2], rot_arr[3]);
                        glm::vec3 scale(scl_arr[0], scl_arr[1], scl_arr[2]);
                        
                        glm::mat4 matrix = MatrixUtils::composeMatrix(position, rotation, scale);
                        set_model_matrix_by_id(model.id, matrix);
                        
                        std::cout << "Loaded interactable: " << file_path << std::endl;
                    } else {
                        std::cerr << "Failed to load interactable: " << file_path << std::endl;
                    }
                }
            }

            // Load interactor model (skeletal)
            if (scene.contains("models") && scene["models"].contains("interactor")) {
                auto& interactor_data = scene["models"]["interactor"];
                std::string file_path = interactor_data["file_path"];
                
                if (load_interactor_model(file_path.c_str())) {
                    std::cout << "Loaded interactor model: " << file_path << std::endl;
                    
                    InteractorModelData& data = get_interactor_model_data_mutable();
                    
                    // Restore bone matrices
                    if (interactor_data.contains("bone_matrices")) {
                        auto& matrices_json = interactor_data["bone_matrices"];
                        if (matrices_json.size() == data.bind_pose_matrices.size()) {
                            for (size_t i = 0; i < matrices_json.size(); ++i) {
                                auto& mat_arr = matrices_json[i];
                                float mat_values[16];
                                for (int j = 0; j < 16; ++j) {
                                    mat_values[j] = mat_arr[j];
                                }
                                data.bind_pose_matrices[i] = glm::make_mat4(mat_values);
                            }
                            std::cout << "Restored " << matrices_json.size() << " bone matrices." << std::endl;
                        } else {
                            std::cerr << "Warning: Saved bone matrices count (" << matrices_json.size() 
                                      << ") does not match loaded model bone count (" << data.bind_pose_matrices.size() << ")" << std::endl;
                        }
                    }
                    
                    // Restore bone positions
                    if (interactor_data.contains("bone_positions")) {
                        auto& positions_json = interactor_data["bone_positions"];
                        if (positions_json.size() == data.bind_pose_positions.size()) {
                            for (size_t i = 0; i < positions_json.size(); ++i) {
                                auto& pos_arr = positions_json[i];
                                data.bind_pose_positions[i] = glm::vec3(pos_arr[0], pos_arr[1], pos_arr[2]);
                            }
                            std::cout << "Restored " << positions_json.size() << " bone positions." << std::endl;
                        }
                    }
                } else {
                    std::cerr << "Failed to load interactor model: " << file_path << std::endl;
                }
            }
            
            // Restore lighting
            LightManager& lightManager = get_light_manager();
            
            // Directional light
            if (scene.contains("lighting") && scene["lighting"].contains("directional")) {
                auto& dir_data = scene["lighting"]["directional"];
                if (dir_data["active"].get<bool>()) {
                    DirectionalLight dirLight;
                    auto dir_arr = dir_data["direction"];
                    auto col_arr = dir_data["color"];
                    dirLight.direction = glm::vec3(dir_arr[0], dir_arr[1], dir_arr[2]);
                    dirLight.color = glm::vec3(col_arr[0], col_arr[1], col_arr[2]);
                    dirLight.intensity = dir_data["intensity"];
                    lightManager.setDirectionalLight(dirLight);
                    lightManager.enableDirectionalLight(true);
                }
            }
            
            // Point lights
            if (scene.contains("lighting") && scene["lighting"].contains("point_lights")) {
                for (const auto& light_data : scene["lighting"]["point_lights"]) {
                    PointLight light;
                    auto pos_arr = light_data["position"];
                    auto col_arr = light_data["color"];
                    light.position = glm::vec3(pos_arr[0], pos_arr[1], pos_arr[2]);
                    light.color = glm::vec3(col_arr[0], col_arr[1], col_arr[2]);
                    light.intensity = light_data["intensity"];
                    lightManager.addPointLight(light);
                }
            }
            
            // Spot lights
            if (scene.contains("lighting") && scene["lighting"].contains("spot_lights")) {
                for (const auto& light_data : scene["lighting"]["spot_lights"]) {
                    SpotLight light;
                    auto pos_arr = light_data["position"];
                    auto dir_arr = light_data["direction"];
                    auto col_arr = light_data["color"];
                    light.position = glm::vec3(pos_arr[0], pos_arr[1], pos_arr[2]);
                    light.direction = glm::vec3(dir_arr[0], dir_arr[1], dir_arr[2]);
                    light.color = glm::vec3(col_arr[0], col_arr[1], col_arr[2]);
                    light.intensity = light_data["intensity"];
                    light.innerCone = cos(glm::radians(light_data["inner_cone"].get<float>()));
                    light.outerCone = cos(glm::radians(light_data["outer_cone"].get<float>()));
                    lightManager.addSpotLight(light);
                }
            }
            
            std::cout << "Scene loaded successfully from: " << filepath << std::endl;
            
            // Auto-load animations
            std::string animFilepath = filepath + ".anim";
            std::ifstream animFile(animFilepath);
            if (animFile.good()) {
                animFile.close();
                get_animation_manager().loadFromFile(animFilepath);
                std::cout << "Auto-loaded animation sequences from: " << animFilepath << std::endl;
            } else {
                std::cout << "No animation file found at: " << animFilepath << std::endl;
            }
            
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Error loading scene: " << e.what() << std::endl;
            return false;
        }
    }
    
    void clearScene() {
        std::cout << "Clearing scene..." << std::endl;
        
        // Remove all scene element models
        std::vector<int> scene_ids;
        size_t scene_count = get_scene_element_model_count();
        for (size_t i = 0; i < scene_count; i++) {
            scene_ids.push_back(get_scene_element_model(i).id);
        }
        for (int id : scene_ids) {
            remove_scene_element_model_by_id(id);
        }
        
        // Remove all interactable models (if removal function exists)
        // TODO: Implement remove_interactable_model_by_id() if needed
        
        // Clear all lights
        LightManager& lightManager = get_light_manager();
        lightManager.clearAllLights();
        
        // Clear selection
        set_selected_object(-1);
        
        std::cout << "Scene cleared" << std::endl;
    }
}
