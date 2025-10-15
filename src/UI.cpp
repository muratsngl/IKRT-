#include "include/UI.hpp"
#include "include/render_setup.hpp"
#include "include/collision_visualizer.hpp"
#include "include/application_logic.hpp"
#include "include/model_loader.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include "ImGuizmo.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

// UI state variables
static bool show_operations_window = true;
static bool show_light_manager_window = true;
static bool show_gizmo_controls_window = true;


// --- MODIFICATION: New Blender-style theme ---
ImGuiStyle create_gui_style() {
    ImGuiStyle style;
    ImVec4* colors = style.Colors;

    // A theme inspired by Blender's UI
    colors[ImGuiCol_Text]                   = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.94f, 0.56f, 0.20f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(1.00f, 0.50f, 0.00f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.20f, 0.45f, 0.80f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.20f, 0.45f, 0.80f, 1.00f);
    colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.20f, 0.45f, 0.80f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.20f, 0.45f, 0.80f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);

    // Rounding and Spacing
    style.FramePadding = ImVec2(4, 4);
    style.ItemSpacing = ImVec2(8, 4);
    style.WindowPadding = ImVec2(8, 8);
    style.GrabMinSize = 12.0f;
    style.WindowRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;

    return style;
}

bool init_ui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    // Apply custom style
    ImGuiStyle& style = ImGui::GetStyle();
    style = create_gui_style();
    
    ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");
    
    return true;
}

void cleanup_ui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}


void RenderModelLoaderWidget(bool* p_open);
void RenderLightManagerWidget(bool* p_open);
void RenderGizmoUI(const glm::mat4& cameraView, const glm::mat4& cameraProjection, glm::mat4& objectMatrix, bool* p_open);
void RenderMainMenuBar();


void render_ui() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    RenderMainMenuBar();

    if (show_operations_window) {
        RenderModelLoaderWidget(&show_operations_window);
    }
    if (show_light_manager_window) {
        RenderLightManagerWidget(&show_light_manager_window);
    }
    
    if (show_gizmo_controls_window && get_selected_object_id() != -1) {
        glm::mat4 cameraView, cameraProjection;
        get_current_camera_matrices(cameraView, cameraProjection);
        glm::mat4& objectMatrix = get_selected_object_matrix();
        RenderGizmoUI(cameraView, cameraProjection, objectMatrix, &show_gizmo_controls_window);
    }
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// --- MODIFICATION: "File" menu has been removed ---
void RenderMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Operations Panel", NULL, &show_operations_window);
            ImGui::MenuItem("Light Manager", NULL, &show_light_manager_window);
            ImGui::MenuItem("Gizmo Controls", NULL, &show_gizmo_controls_window);
            ImGui::EndMenu();
        }

        // Mode Switcher integrated into the menu bar
        const char* mode_names[] = {"Edit Scene", "Free View", "Animate"};
        static int current_mode_index = get_app_mode();
        float combo_width = 120.0f;
        ImGui::SameLine(ImGui::GetWindowWidth() - combo_width - 15);
        
        ImGui::Text("Mode:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(combo_width);
        if (ImGui::Combo("##mode_combo", &current_mode_index, mode_names, 3)) {
            set_app_mode((MODE)current_mode_index);
        }

        ImGui::EndMainMenuBar();
    }
}

void RenderModelLoaderWidget(bool* p_open) {
    ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(250, 400), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Operations", p_open, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::End();
        return;
    }

    if (ImGui::CollapsingHeader("Model Loading", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Load 3D Models:");
        ImGui::Separator();
        if (ImGui::Button("Load Scene Element", ImVec2(-1, 0))) {
            current_model_type_to_load = ModelType::SceneElement;
            ImGuiFileDialog::Instance()->OpenDialog("ChooseModelFileDlgKey", "Choose a Model File", ".obj,.gltf,.glb,.dae,.fbx");
        }
        if (ImGui::Button("Load Interactor", ImVec2(-1, 0))) {
            current_model_type_to_load = ModelType::Interactor;
            ImGuiFileDialog::Instance()->OpenDialog("ChooseModelFileDlgKey", "Choose a Model File", ".obj,.gltf,.glb,.dae,.fbx");
        }
        if (ImGui::Button("Load Interactable", ImVec2(-1, 0))) {
            current_model_type_to_load = ModelType::Interactable;
            ImGuiFileDialog::Instance()->OpenDialog("ChooseModelFileDlgKey", "Choose a Model File", ".obj,.gltf,.glb,.dae,.fbx");
        }
    }

    if (ImGui::CollapsingHeader("Settings")) {
        ImGui::Text("Settings will go here...");
    }

    if (ImGui::CollapsingHeader("Animation Controls")) {
        ApplicationState& app_state = get_application_state();
        
        // Check if interactor model is loaded
        if (is_interactor_model_available()) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Interactor Model Loaded");
            
            if (ImGui::Checkbox("Manual Control Mode", &app_state.manualControlMode)) {
                if (app_state.manualControlMode) {
                    std::cout << "Manual control mode enabled - shared memory disabled" << std::endl;
                    CollisionVisualizer::showTargetProxies = true;
                } else {
                    std::cout << "Manual control mode disabled - shared memory enabled" << std::endl;
                    CollisionVisualizer::showTargetProxies = false;
                }
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Enable manual control of target positions\nwithout shared memory");
            }
            
            if (ImGui::Checkbox("Show Target Proxies", &CollisionVisualizer::showTargetProxies)) {
                if (CollisionVisualizer::showTargetProxies) {
                    update_target_proxies();
                }
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Visualize and interact with IK target positions");
            }
            
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.0f, 1.0f), "No Interactor Model");
            ImGui::TextWrapped("Please load an interactor model to use animation controls.");
        }
    }

    if (ImGui::CollapsingHeader("Debug")) {
        static bool renderCollisionGeometry = false;
        if (ImGui::Checkbox("Render Collision Geometry", &renderCollisionGeometry)) {
            CollisionVisualizer::isEnabled = renderCollisionGeometry;
        }
    }

    ImGui::End();

    if (ImGuiFileDialog::Instance()->Display("ChooseModelFileDlgKey")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string file_path = ImGuiFileDialog::Instance()->GetFilePathName();
            switch (current_model_type_to_load) {
                case ModelType::SceneElement: load_scene_element_model(file_path.c_str()); break;
                case ModelType::Interactor: load_interactor_model(file_path.c_str()); break;
                case ModelType::Interactable: load_interactable_model(file_path.c_str()); break;
                case ModelType::None: break;
            }
            current_model_type_to_load = ModelType::None;
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

void RenderGizmoUI(const glm::mat4& cameraView, const glm::mat4& cameraProjection, glm::mat4& objectMatrix, bool* p_open) {
    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    
    static ImGuizmo::OPERATION currentOperation = ImGuizmo::TRANSLATE;
    static ImGuizmo::MODE currentMode = ImGuizmo::WORLD;

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 260, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(250, 0), ImGuiCond_FirstUseEver);
    
    if (!ImGui::Begin("Gizmo Controls", p_open)) {
        ImGui::End();
        return;
    }

    if (ImGui::RadioButton("Translate", currentOperation == ImGuizmo::TRANSLATE)) currentOperation = ImGuizmo::TRANSLATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate", currentOperation == ImGuizmo::ROTATE)) currentOperation = ImGuizmo::ROTATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", currentOperation == ImGuizmo::SCALE)) currentOperation = ImGuizmo::SCALE;

    if (currentOperation != ImGuizmo::SCALE) {
        if (ImGui::RadioButton("Local", currentMode == ImGuizmo::LOCAL)) currentMode = ImGuizmo::LOCAL;
        ImGui::SameLine();
        if (ImGui::RadioButton("World", currentMode == ImGuizmo::WORLD)) currentMode = ImGuizmo::WORLD;
    }

    ImGui::Separator();

    ImGuizmo::Manipulate(
        glm::value_ptr(cameraView),
        glm::value_ptr(cameraProjection),
        currentOperation,
        currentMode,
        glm::value_ptr(objectMatrix)
    );

    if (ImGuizmo::IsUsing()) {
        glm::vec3 scale, translation;
        glm::quat rotation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(objectMatrix, scale, rotation, translation, skew, perspective);

        // Check if we're manipulating a target proxy
        int selected_id = get_selected_object_id();
        if (selected_id >= TARGET_PROXY_INDEX && selected_id <= TARGET_PROXY_PINKY) {
            // Update target position via delta system
            update_target_from_gizmo(selected_id, translation);
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Target Proxy");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Object Transform");
        }
        
        ImGui::Text("Translation: %.3f, %.3f, %.3f", translation.x, translation.y, translation.z);
        ImGui::Text("Rotation: %.3f, %.3f, %.3f, %.3f", rotation.w, rotation.x, rotation.y, rotation.z);
        ImGui::Text("Scale: %.3f, %.3f, %.3f", scale.x, scale.y, scale.z);
    }

    ImGui::End();
}

void RenderLightManagerWidget(bool* p_open) {
    ImGui::SetNextWindowPos(ImVec2(270, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(280, 500), ImGuiCond_FirstUseEver);
    
    if (!ImGui::Begin("Light Manager", p_open, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::End();
        return;
    }
    
    LightManager& lightManager = get_light_manager();
    
    // Point Lights Section
    if (ImGui::CollapsingHeader("Point Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Active: %d / %d", lightManager.getActivePointLights(), MAX_POINT_LIGHTS);
        
        if (lightManager.getActivePointLights() > 0) {
            ImGui::Text("Debug - First light position: %.2f, %.2f, %.2f", 
                lightManager.getPointLight(0).position.x,
                lightManager.getPointLight(0).position.y,
                lightManager.getPointLight(0).position.z);
            ImGui::Text("Debug - First light intensity: %.2f", lightManager.getPointLight(0).intensity);
        }
        
        if (ImGui::Button("Add Point Light", ImVec2(-1, 0)) && lightManager.getActivePointLights() < MAX_POINT_LIGHTS) {
            PointLight newLight;
            newLight.position = glm::vec3(0.0f, 10.0f, 10.0f);
            newLight.color = glm::vec3(1.0f, 0.5f, 0.2f);
            newLight.intensity = 500.0f;
            lightManager.addPointLight(newLight);
        }
        
        if (ImGui::Button("Add Camera Light", ImVec2(-1, 0)) && lightManager.getActivePointLights() < MAX_POINT_LIGHTS) {
            PointLight newLight;
            newLight.position = glm::vec3(0.0f, 5.5f, 30.0f);
            newLight.color = glm::vec3(1.0f, 0.0f, 0.0f);
            newLight.intensity = 1000.0f;
            lightManager.addPointLight(newLight);
        }
        
        for (int i = 0; i < lightManager.getActivePointLights(); i++) {
            ImGui::PushID(i);
            if (ImGui::TreeNode(("Point Light " + std::to_string(i)).c_str())) {
                PointLight& light = lightManager.getPointLight(i);
                ImGui::DragFloat3("Position", &light.position.x, 0.1f);
                ImGui::ColorEdit3("Color", &light.color.r);
                ImGui::DragFloat("Intensity", &light.intensity, 0.5f, 0.0f, 1000.0f);
                if (ImGui::Button("Remove", ImVec2(-1, 0))) {
                    lightManager.removePointLight(i);
                    ImGui::TreePop();
                    ImGui::PopID();
                    break;
                }
                lightManager.updatePointLight(i, light);
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }
    
    // --- RESTORED SPOT LIGHTS SECTION ---
    if (ImGui::CollapsingHeader("Spot Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Active: %d / %d", lightManager.getActiveSpotLights(), MAX_SPOT_LIGHTS);
        
        if (ImGui::Button("Add Spot Light", ImVec2(-1, 0)) && lightManager.getActiveSpotLights() < MAX_SPOT_LIGHTS) {
            SpotLight newLight;
            newLight.position = glm::vec3(0.0f, 5.0f, 0.0f);
            newLight.direction = glm::vec3(0.0f, -1.0f, 0.0f);
            newLight.color = glm::vec3(1.0f, 1.0f, 1.0f);
            newLight.intensity = 25.0f;
            newLight.innerCone = cos(glm::radians(12.5f));
            newLight.outerCone = cos(glm::radians(17.5f));
            lightManager.addSpotLight(newLight);
        }
        
        for (int i = 0; i < lightManager.getActiveSpotLights(); i++) {
            ImGui::PushID(i + 1000); // Offset ID
            if (ImGui::TreeNode(("Spot Light " + std::to_string(i)).c_str())) {
                SpotLight& light = lightManager.getSpotLight(i);
                
                ImGui::DragFloat3("Position##spot", &light.position.x, 0.1f);
                ImGui::DragFloat3("Direction##spot", &light.direction.x, 0.1f, -1.0f, 1.0f);
                ImGui::ColorEdit3("Color##spot", &light.color.r);
                ImGui::DragFloat("Intensity##spot", &light.intensity, 0.5f, 0.0f, 1000.0f);
                
                float innerAngleDegrees = glm::degrees(acos(light.innerCone));
                float outerAngleDegrees = glm::degrees(acos(light.outerCone));
                
                if (ImGui::DragFloat("Inner Cone Angle", &innerAngleDegrees, 0.5f, 0.0f, 90.0f)) {
                    light.innerCone = cos(glm::radians(innerAngleDegrees));
                }
                if (ImGui::DragFloat("Outer Cone Angle", &outerAngleDegrees, 0.5f, 0.0f, 90.0f)) {
                    light.outerCone = cos(glm::radians(outerAngleDegrees));
                }
                if (light.innerCone < light.outerCone) {
                    light.outerCone = light.innerCone;
                }
                
                if (ImGui::Button("Remove##spot", ImVec2(-1, 0))) {
                    lightManager.removeSpotLight(i);
                    ImGui::TreePop();
                    ImGui::PopID();
                    break;
                }
                lightManager.updateSpotLight(i, light);
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }
    
    // --- RESTORED DIRECTIONAL LIGHT SECTION ---
    if (ImGui::CollapsingHeader("Directional Light (Sun)", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool hasDirectionalLight = lightManager.isDirectionalLightActive();
        
        if (!hasDirectionalLight) {
            ImGui::Text("No directional light active (0/1)");
            if (ImGui::Button("Add Directional Light", ImVec2(-1, 0))) {
                DirectionalLight newDirLight;
                newDirLight.direction = glm::normalize(glm::vec3(-0.2f, -1.0f, -0.3f));
                newDirLight.color = glm::vec3(1.0f, 0.95f, 0.8f);
                newDirLight.intensity = 1.0f;
                lightManager.setDirectionalLight(newDirLight);
            }
        } else {
            ImGui::Text("Active: 1/1");
            DirectionalLight& dirLight = lightManager.getDirectionalLight();
            
            static float azimuth = atan2(dirLight.direction.x, dirLight.direction.z) * 180.0f / M_PI;
            static float elevation = asin(-dirLight.direction.y) * 180.0f / M_PI;
            
            bool changed = false;
            if (ImGui::SliderFloat("Azimuth (degrees)", &azimuth, -180.0f, 180.0f)) changed = true;
            if (ImGui::SliderFloat("Elevation (degrees)", &elevation, -90.0f, 90.0f)) changed = true;
            
            if (changed) {
                float azimuthRad = glm::radians(azimuth);
                float elevationRad = glm::radians(elevation);
                dirLight.direction.x = sin(azimuthRad) * cos(elevationRad);
                dirLight.direction.y = -sin(elevationRad);
                dirLight.direction.z = cos(azimuthRad) * cos(elevationRad);
                dirLight.direction = glm::normalize(dirLight.direction);
                lightManager.setDirectionalLight(dirLight);
            }
            
            if (ImGui::ColorEdit3("Color##dir", &dirLight.color.r)) lightManager.setDirectionalLight(dirLight);
            if (ImGui::DragFloat("Intensity##dir", &dirLight.intensity, 0.1f, 0.0f, 10.0f)) lightManager.setDirectionalLight(dirLight);
            
            static bool enabled = true;
            if (ImGui::Checkbox("Enable Directional Light", &enabled)) {
                lightManager.enableDirectionalLight(enabled);
            }
            
            ImGui::Text("Direction: %.3f, %.3f, %.3f", dirLight.direction.x, dirLight.direction.y, dirLight.direction.z);
            
            ImGui::Separator();
            ImGui::Text("Presets:");
            if (ImGui::Button("Noon")) {
                azimuth = 0.0f; elevation = 70.0f; changed = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Morning")) {
                azimuth = 90.0f; elevation = 30.0f; changed = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Evening")) {
                azimuth = -90.0f; elevation = 30.0f; changed = true;
            }
            
            ImGui::Separator();
            if (ImGui::Button("Remove Directional Light", ImVec2(-1, 0))) {
                lightManager.enableDirectionalLight(false);
            }
        }
    }
    
    ImGui::Separator();
    if (ImGui::Button("Clear All Lights", ImVec2(-1, 0))) {
        lightManager.clearAllLights();
    }
    
    ImGui::End();
}