#include "include/UI.hpp"
#include "include/render_setup.hpp"
#include "include/collision_visualizer.hpp"
#include "include/application_logic.hpp"
#include "include/model_loader.hpp"
#include "include/scene_serializer.hpp"
#include "include/bone_hierarchy.hpp"
#include "include/model_bones.h"
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
static bool show_bone_inspector_window = false;

// IK Chain builder state
static IKChainManager chainManager;
static IKChainDefinition currentChain;
static bool buildingChain = false;


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
void RenderBoneInspectorWidget(bool* p_open);
void RenderMainMenuBar();


void render_ui() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    // Quit confirmation popup
    if (is_quit_confirmation_shown()) {
        ImGui::OpenPopup("Quit Confirmation");
    }
    
    // Center the popup
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    
    if (ImGui::BeginPopupModal("Quit Confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Are you sure you want to quit?");
        ImGui::Text("Any unsaved changes will be lost.");
        ImGui::Separator();
        
        ImGui::Spacing();
        if (ImGui::Button("Yes, Quit", ImVec2(120, 0))) {
            confirm_quit();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            cancel_quit();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    RenderMainMenuBar();

    if (show_operations_window) {
        RenderModelLoaderWidget(&show_operations_window);
    }
    if (show_light_manager_window) {
        RenderLightManagerWidget(&show_light_manager_window);
    }
    if (show_bone_inspector_window) {
        RenderBoneInspectorWidget(&show_bone_inspector_window);
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

// --- IK Chain Building State Access Implementation ---
bool is_building_chain() {
    return buildingChain;
}

bool is_bone_in_current_chain(int bone_id) {
    if (!buildingChain) return false;
    
    for (int id : currentChain.boneIndices) {
        if (id == bone_id) return true;
    }
    return false;
}

void add_bone_to_current_chain(int bone_id) {
    if (!buildingChain) return;
    
    // Check if bone is already in chain to avoid duplicates
    for (int id : currentChain.boneIndices) {
        if (id == bone_id) return;
    }
    
    // Get bone name
    std::string boneName = "Unknown";
    if (is_interactor_model_available()) {
        InteractorModel* model = get_interactor_model();
        if (model) {
            BoneHierarchy* hierarchy = model->getBoneHierarchy();
            if (hierarchy) {
                BoneNode* node = hierarchy->findBoneById(bone_id);
                if (node) {
                    boneName = node->name;
                }
            }
        }
    }
    
    currentChain.boneIndices.push_back(bone_id);
    currentChain.boneNames.push_back(boneName);
    std::cout << "Added bone '" << boneName << "' (ID: " << bone_id << ") to chain via selection" << std::endl;
}

void remove_last_bone_from_current_chain() {
    if (!buildingChain || currentChain.boneIndices.empty()) return;
    
    int removedId = currentChain.boneIndices.back();
    std::string removedName = currentChain.boneNames.back();
    
    currentChain.boneIndices.pop_back();
    currentChain.boneNames.pop_back();
    
    std::cout << "Removed bone '" << removedName << "' (ID: " << removedId << ") from chain" << std::endl;
}

// --- MODIFICATION: "File" menu has been removed ---
void RenderMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Operations Panel", NULL, &show_operations_window);
            ImGui::MenuItem("Light Manager", NULL, &show_light_manager_window);
            ImGui::MenuItem("Gizmo Controls", NULL, &show_gizmo_controls_window);
            ImGui::MenuItem("Bone Inspector & IK Builder", NULL, &show_bone_inspector_window);
            ImGui::EndMenu();
        }

        // Mode Switcher integrated into the menu bar
        const char* mode_names[] = {"Edit Scene", "Free View"};
        static int current_mode_index = get_app_mode();
        float combo_width = 120.0f;
        ImGui::SameLine(ImGui::GetWindowWidth() - combo_width - 15);
        
        ImGui::Text("Mode:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(combo_width);
        if (ImGui::Combo("##mode_combo", &current_mode_index, mode_names, 2)) {
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

    if (ImGui::CollapsingHeader("Scene Management", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Scene Operations:");
        ImGui::Separator();
        
        if (ImGui::Button("Save Scene", ImVec2(-1, 0))) {
            ImGuiFileDialog::Instance()->OpenDialog("SaveSceneFileDlgKey", "Save Scene File", ".scene,.json");
        }
        
        if (ImGui::Button("Load Scene", ImVec2(-1, 0))) {
            ImGuiFileDialog::Instance()->OpenDialog("LoadSceneFileDlgKey", "Load Scene File", ".scene,.json");
        }
        
        if (ImGui::Button("Clear Scene", ImVec2(-1, 0))) {
            ImGui::OpenPopup("ClearSceneConfirm");
        }
        
        // Confirmation popup for clear scene
        if (ImGui::BeginPopupModal("ClearSceneConfirm", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Are you sure you want to clear the entire scene?");
            ImGui::Text("This will remove all models and lights.");
            ImGui::Separator();
            
            if (ImGui::Button("Yes, Clear Scene", ImVec2(120, 0))) {
                SceneSerializer::clearScene();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
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
                    update_target_proxies();
                } else {
                    std::cout << "Manual control mode disabled - shared memory enabled" << std::endl;
                    CollisionVisualizer::showTargetProxies = false;
                }
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Enable manual control of target positions\nShows target proxies and disables shared memory");
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
        
        static bool renderBoneVisualization = false;
        if (ImGui::Checkbox("Render Bone Visualization", &renderBoneVisualization)) {
            CollisionVisualizer::showBoneVisualization = renderBoneVisualization;
        }
        
        static bool wireframeMode = false;
        if (ImGui::Checkbox("Wireframe Mode", &wireframeMode)) {
            set_wireframe_mode(wireframeMode);
        }
        
        ImGui::Separator();
        
        // Remove selected model button
        int selected_id = get_selected_object_id();
        if (selected_id != -1) {
            ImGui::Text("Selected Object ID: %d", selected_id);
            
            // Check if it's a bone
            int bone_id = get_bone_id_from_shape_id(selected_id);
            if (bone_id != -1) {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Selected: Bone #%d", bone_id);
                ImGui::TextWrapped("Click on bones to build IK chains in the Bone Inspector.");
            }
            // Determine if it's a target proxy (cannot be deleted)
            else if (selected_id >= TARGET_PROXY_INDEX && selected_id <= TARGET_PROXY_PINKY) {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Target Proxy (Cannot Remove)");
            } else {
                if (ImGui::Button("Remove Selected Model", ImVec2(-1, 0))) {
                    if (remove_scene_element_model_by_id(selected_id)) {
                        ImGui::OpenPopup("RemoveSuccess");
                    } else {
                        ImGui::OpenPopup("RemoveFailed");
                    }
                }
                
                if (ImGui::BeginPopup("RemoveSuccess")) {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Model removed successfully!");
                    ImGui::EndPopup();
                }
                
                if (ImGui::BeginPopup("RemoveFailed")) {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to remove model.");
                    ImGui::TextWrapped("Model may not exist or could be an interactable.");
                    ImGui::EndPopup();
                }
            }
        } else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No object selected");
        }
    }

    ImGui::End();

    // Model file dialog
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
    
    // Save scene dialog
    if (ImGuiFileDialog::Instance()->Display("SaveSceneFileDlgKey")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string file_path = ImGuiFileDialog::Instance()->GetFilePathName();
            // Ensure it has .scene extension
            if (file_path.find(".scene") == std::string::npos && file_path.find(".json") == std::string::npos) {
                file_path += ".scene";
            }
            if (SceneSerializer::saveScene(file_path)) {
                std::cout << "Scene saved successfully!" << std::endl;
            } else {
                std::cerr << "Failed to save scene!" << std::endl;
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
    
    // Load scene dialog
    if (ImGuiFileDialog::Instance()->Display("LoadSceneFileDlgKey")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string file_path = ImGuiFileDialog::Instance()->GetFilePathName();
            if (SceneSerializer::loadScene(file_path)) {
                std::cout << "Scene loaded successfully!" << std::endl;
            } else {
                std::cerr << "Failed to load scene!" << std::endl;
            }
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
        } else if (selected_id >= 20000) {
            // Bone manipulation - propagate to children in FK mode
            int bone_id = selected_id - 20000;
            recompute_bone_hierarchy_from(bone_id);
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Bone Transform (FK)");
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

// Helper function to render bone tree recursively
static void RenderBoneNodeTree(BoneNode* node, IKChainDefinition& currentChain, bool buildingChain) {
    if (!node) return;
    
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen;
    if (node->children.empty()) {
        nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    
    // Highlight if bone is in current chain
    bool inChain = false;
    int chainPosition = -1;
    for (size_t i = 0; i < currentChain.boneIndices.size(); i++) {
        if (currentChain.boneIndices[i] == node->boneId) {
            inChain = true;
            chainPosition = (int)i;
            break;
        }
    }
    
    if (inChain) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
    }
    
    // Show bone with ID and chain position if applicable
    std::string label = node->name + " (ID: " + std::to_string(node->boneId) + ")";
    if (inChain) {
        label += " [Chain #" + std::to_string(chainPosition + 1) + "]";
    }
    
    bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)node->boneId, nodeFlags, "%s", label.c_str());
    
    if (inChain) {
        ImGui::PopStyleColor();
    }
    
    // Tooltip showing full path on hover
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("Path: %s", node->getPath().c_str());
        ImGui::Text("Depth: %d", node->getDepth());
        ImGui::Text("Children: %d", (int)node->children.size());
        ImGui::EndTooltip();
    }
    
    // Context menu for bone
    if (ImGui::BeginPopupContextItem()) {
        ImGui::Text("Bone: %s", node->name.c_str());
        ImGui::Separator();
        
        if (buildingChain) {
            if (ImGui::MenuItem("Add to Chain")) {
                currentChain.boneIndices.push_back(node->boneId);
                currentChain.boneNames.push_back(node->name);
                std::cout << "Added bone '" << node->name << "' (ID: " << node->boneId << ") to chain" << std::endl;
            }
            if (!currentChain.boneIndices.empty() && 
                currentChain.boneIndices.back() == node->boneId) {
                if (ImGui::MenuItem("Remove from Chain")) {
                    currentChain.boneIndices.pop_back();
                    currentChain.boneNames.pop_back();
                }
            }
        }
        
        ImGui::EndPopup();
    }
    
    // Render children
    if (nodeOpen && !node->children.empty()) {
        for (auto& child : node->children) {
            RenderBoneNodeTree(child.get(), currentChain, buildingChain);
        }
        ImGui::TreePop();
    }
}

void RenderBoneInspectorWidget(bool* p_open) {
    ImGui::SetNextWindowPos(ImVec2(10, 440), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(450, 700), ImGuiCond_FirstUseEver);
    
    if (!ImGui::Begin("Bone Inspector & IK Chain Builder", p_open, ImGuiWindowFlags_None)) {
        ImGui::End();
        return;
    }
    
    if (!is_interactor_model_available()) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "No Interactor Model Loaded");
        ImGui::TextWrapped("Load an interactor model with bones to use this tool.");
        ImGui::End();
        return;
    }
    
    InteractorModel* modelPtr = get_interactor_model();
    if (!modelPtr) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.0f, 1.0f), "Error accessing model");
        ImGui::End();
        return;
    }
    
    BoneHierarchy* hierarchy = modelPtr->getBoneHierarchy();
    
    if (!hierarchy || !hierarchy->isValid()) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.0f, 1.0f), "No Bone Hierarchy Available");
        ImGui::TextWrapped("The loaded model doesn't have a valid bone structure.");
        ImGui::End();
        return;
    }
    
    // Bone Inspector Section
    if (ImGui::CollapsingHeader("Bone Hierarchy", ImGuiTreeNodeFlags_DefaultOpen)) {
        std::vector<BoneNode*> allBones;
        hierarchy->getAllBones(allBones);
        ImGui::Text("Total Bones: %d", (int)allBones.size());
        
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Right-click bones to add to chain");
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Tip: Resize window if hierarchy is cut off");
        ImGui::Separator();
        
        // Calculate flexible height - take remaining window space minus chain builder section
        float availableHeight = ImGui::GetContentRegionAvail().y;
        float treeHeight = availableHeight * 0.5f; // Use 50% for tree, rest for chain builder
        
        if (ImGui::BeginChild("BoneTree", ImVec2(0, treeHeight), true, ImGuiWindowFlags_HorizontalScrollbar)) {
            BoneNode* root = hierarchy->getRoot();
            if (root) {
                // Set all tree nodes to start open by default
                ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 15.0f);
                RenderBoneNodeTree(root, currentChain, buildingChain);
                ImGui::PopStyleVar();
            }
        }
        ImGui::EndChild();
    }
    
    // IK Chain Builder Section
    if (ImGui::CollapsingHeader("IK Chain Builder", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (!buildingChain) {
            if (ImGui::Button("Start New Chain", ImVec2(-1, 0))) {
                buildingChain = true;
                currentChain = IKChainDefinition();
                currentChain.name = "New Chain";
                std::cout << "Started building new IK chain" << std::endl;
            }
        } else {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Building Chain...");
            
            // Chain name input
            char nameBuffer[128];
            strncpy(nameBuffer, currentChain.name.c_str(), sizeof(nameBuffer));
            if (ImGui::InputText("Chain Name", nameBuffer, sizeof(nameBuffer))) {
                currentChain.name = nameBuffer;
            }
            
            // Display current chain
            ImGui::Text("Bones in chain: %d", (int)currentChain.boneIndices.size());
            if (ImGui::BeginChild("CurrentChain", ImVec2(0, 100), true)) {
                for (size_t i = 0; i < currentChain.boneIndices.size(); i++) {
                    ImGui::Text("%d. %s (ID: %d)", 
                               (int)i+1, 
                               currentChain.boneNames[i].c_str(),
                               currentChain.boneIndices[i]);
                }
            }
            ImGui::EndChild();
            
            // Chain actions
            if (ImGui::Button("Save Chain", ImVec2(-1, 0))) {
                if (!currentChain.boneIndices.empty()) {
                    chainManager.addChain(currentChain);
                    std::cout << "Saved chain: " << currentChain.name << std::endl;
                    buildingChain = false;
                    currentChain = IKChainDefinition();
                } else {
                    std::cerr << "Cannot save empty chain!" << std::endl;
                }
            }
            
            if (ImGui::Button("Cancel", ImVec2(-1, 0))) {
                buildingChain = false;
                currentChain = IKChainDefinition();
                std::cout << "Cancelled chain building" << std::endl;
            }
        }
    }
    
    // Saved Chains Section
    if (ImGui::CollapsingHeader("Saved IK Chains")) {
        size_t chainCount = chainManager.getChainCount();
        ImGui::Text("Saved Chains: %d", (int)chainCount);
        
        if (chainCount > 0) {
            for (size_t i = 0; i < chainCount; i++) {
                IKChainDefinition* chain = chainManager.getChain(i);
                if (ImGui::TreeNode((void*)(intptr_t)i, "%s (%d bones)", 
                                   chain->name.c_str(), 
                                   (int)chain->boneIndices.size())) {
                    for (size_t j = 0; j < chain->boneIndices.size(); j++) {
                        ImGui::Text("  %d. %s (ID: %d)", 
                                   (int)j+1,
                                   chain->boneNames[j].c_str(),
                                   chain->boneIndices[j]);
                    }
                    
                    if (ImGui::Button("Delete Chain")) {
                        chainManager.removeChain(i);
                        ImGui::TreePop();
                        break;
                    }
                    
                    ImGui::TreePop();
                }
            }
        }
        
        ImGui::Separator();
        
        if (ImGui::Button("Save Chains to File", ImVec2(-1, 0))) {
            ImGuiFileDialog::Instance()->OpenDialog("SaveChainsFileDlgKey", "Save IK Chains", ".ikchains,.json");
        }
        
        if (ImGui::Button("Load Chains from File", ImVec2(-1, 0))) {
            ImGuiFileDialog::Instance()->OpenDialog("LoadChainsFileDlgKey", "Load IK Chains", ".ikchains,.json");
        }
    }
    
    ImGui::End();
    
    // File dialogs for chain save/load
    if (ImGuiFileDialog::Instance()->Display("SaveChainsFileDlgKey")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filepath = ImGuiFileDialog::Instance()->GetFilePathName();
            if (filepath.find(".ikchains") == std::string::npos && filepath.find(".json") == std::string::npos) {
                filepath += ".ikchains";
            }
            chainManager.saveToFile(filepath);
        }
        ImGuiFileDialog::Instance()->Close();
    }
    
    if (ImGuiFileDialog::Instance()->Display("LoadChainsFileDlgKey")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filepath = ImGuiFileDialog::Instance()->GetFilePathName();
            chainManager.loadFromFile(filepath);
        }
        ImGuiFileDialog::Instance()->Close();
    }
}