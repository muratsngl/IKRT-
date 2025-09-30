#include "include/UI.hpp"
#include "include/render_setup.hpp"
#include "include/collision_visualizer.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// --- ImGuizmo Integration Start ---
#include "ImGuizmo.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
// --- ImGuizmo Integration End ---


// --- UI System Functions ---

ImGuiStyle create_gui_style() {
    ImGuiStyle style;
    
    // Set alpha/transparency for the entire UI
    style.Alpha = 0.85f;  // Slightly transparent
    
    // Set window background to transparent green
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.2f, 0.1f, 0.8f);  // Dark green with transparency
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.3f, 0.15f, 0.9f);  // Slightly lighter green for title
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.2f, 0.4f, 0.2f, 0.95f);  // Active title
    
    // Header colors (collapsible sections)
    style.Colors[ImGuiCol_Header] = ImVec4(0.2f, 0.4f, 0.2f, 0.8f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.5f, 0.25f, 0.9f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.3f, 0.6f, 0.3f, 1.0f);
    
    // Button colors
    style.Colors[ImGuiCol_Button] = ImVec4(0.2f, 0.4f, 0.2f, 0.8f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.6f, 0.3f, 0.9f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.4f, 0.7f, 0.4f, 1.0f);
    
    // Frame colors (input fields, etc.)
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.25f, 0.15f, 0.7f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.2f, 0.35f, 0.2f, 0.8f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.45f, 0.25f, 0.9f);
    
    // Text colors
    style.Colors[ImGuiCol_Text] = ImVec4(0.9f, 1.0f, 0.9f, 1.0f);  // Light green text
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.5f, 0.7f, 0.5f, 0.8f);
    
    // Border colors
    style.Colors[ImGuiCol_Border] = ImVec4(0.3f, 0.5f, 0.3f, 0.5f);
    
    // Popup/modal colors
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.1f, 0.2f, 0.1f, 0.9f);
    
    // Scrollbar colors
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.1f, 0.2f, 0.1f, 0.5f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3f, 0.5f, 0.3f, 0.8f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.4f, 0.6f, 0.4f, 0.9f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.5f, 0.7f, 0.5f, 1.0f);
    
    return style;
}

bool init_ui(GLFWwindow* window) {
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    // Apply custom style
    ImGuiStyle& style = ImGui::GetStyle();
    style = create_gui_style();
    
    // Set the ImGui context for ImGuizmo
    ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");
    
    return true;
}

void cleanup_ui() {
    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}


// --- ImGuizmo Integration Change ---
// 1. UPDATE THE FORWARD DECLARATION
void RenderGizmoUI(const glm::mat4& cameraView, const glm::mat4& cameraProjection, glm::mat4& objectMatrix);
// --- End Change ---


void render_ui() {
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Begin the ImGuizmo frame
    ImGuizmo::BeginFrame();

    // Render all UI components
    RenderModeSwitcher();
    RenderModelLoaderWidget();

    // --- ImGuizmo Integration Change ---
    // Only render gizmo if an object is selected
    if (get_selected_object_id() != -1) {
        // Get the actual camera matrices from the render system
        glm::mat4 cameraView, cameraProjection;
        get_current_camera_matrices(cameraView, cameraProjection);
        
        // Get direct reference to the selected object's matrix
        glm::mat4& objectMatrix = get_selected_object_matrix();
        
        // Render the gizmo with real matrices - it will directly modify the reference
        RenderGizmoUI(cameraView, cameraProjection, objectMatrix);
    }
    // --- End Change ---
    
    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// --- Individual UI Component Functions ---

void RenderModelLoaderWidget() {
    // Create a small collapsible operations panel in the top-left corner
    static bool show_operations = true;

    // Set window position and size for top-left corner, accounting for minimalistic mode switcher
    ImGui::SetNextWindowPos(ImVec2(10, 45), ImGuiCond_FirstUseEver);  // Moved down to y=45
    ImGui::SetNextWindowSize(ImVec2(250, 400), ImGuiCond_FirstUseEver);

    // Create the operations window
    ImGui::Begin("Operations", &show_operations, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

    // Model Loading Section
    if (ImGui::CollapsingHeader("Model Loading", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Load 3D Models:");
        ImGui::Separator();

        // Scene Element Model Button
        if (ImGui::Button("Load Scene Element", ImVec2(-1, 0))) {
            current_model_type_to_load = ModelType::SceneElement;
            ImGuiFileDialog::Instance()->OpenDialog("ChooseModelFileDlgKey", "Choose a Model File", ".obj,.gltf,.glb,.dae,.fbx");
        }

        // Interactor Model Button
        if (ImGui::Button("Load Interactor", ImVec2(-1, 0))) {
            current_model_type_to_load = ModelType::Interactor;
            ImGuiFileDialog::Instance()->OpenDialog("ChooseModelFileDlgKey", "Choose a Model File", ".obj,.gltf,.glb,.dae,.fbx");
        }

        // Interactable Model Button
        if (ImGui::Button("Load Interactable", ImVec2(-1, 0))) {
            current_model_type_to_load = ModelType::Interactable;
            ImGuiFileDialog::Instance()->OpenDialog("ChooseModelFileDlgKey", "Choose a Model File", ".obj,.gltf,.glb,.dae,.fbx");
        }
    }

    // Future sections can be added here
    if (ImGui::CollapsingHeader("Settings")) {
        ImGui::Text("Settings will go here...");
    }

    if (ImGui::CollapsingHeader("Debug")) {
        // Collision geometry toggle using static methods
        static bool renderCollisionGeometry = false;
        if (ImGui::Checkbox("Render Collision Geometry", &renderCollisionGeometry)) {
            CollisionVisualizer::isEnabled = renderCollisionGeometry;
        }
        
        
    }

    ImGui::End();


    // 2. DISPLAY AND HANDLE THE FILE DIALOG
    if (ImGuiFileDialog::Instance()->Display("ChooseModelFileDlgKey")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string file_path = ImGuiFileDialog::Instance()->GetFilePathName();
            switch (current_model_type_to_load) {
                case ModelType::SceneElement:
                    load_scene_element_model(file_path.c_str());
                    break;
                case ModelType::Interactor:
                    load_interactor_model(file_path.c_str());
                    break;
                case ModelType::Interactable:
                    load_interactable_model(file_path.c_str());
                    break;
                case ModelType::None:
                    break;
            }
            current_model_type_to_load = ModelType::None;
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

void RenderModeSwitcher() {
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(200, 35), ImGuiCond_Always);
    
    ImGui::Begin("Mode", nullptr, 
        ImGuiWindowFlags_NoTitleBar | 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar);
    
    const char* mode_names[] = {"Edit Scene", "Free View", "Animate"};
    static int current_mode_index = get_app_mode();
    
    ImGui::Text("Mode:");
    ImGui::SameLine();
    
    ImGui::SetNextItemWidth(-1);
    
    if (ImGui::Combo("##mode_combo", &current_mode_index, mode_names, 3)) {
        set_app_mode((MODE)current_mode_index);
    }
    
    ImGui::End();
}


// --- ImGuizmo Integration Change ---
// 3. UPDATE FUNCTION TO ACCEPT MATRICES AS PARAMETERS
void RenderGizmoUI(const glm::mat4& cameraView, const glm::mat4& cameraProjection, glm::mat4& objectMatrix) {
    // Set the gizmo to draw on the full viewport
    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    
    // --- Gizmo Controls Window ---
    static ImGuizmo::OPERATION currentOperation = ImGuizmo::TRANSLATE;
    static ImGuizmo::MODE currentMode = ImGuizmo::WORLD;

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 260, 45), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(250, 0), ImGuiCond_FirstUseEver); // Auto-resize height
    ImGui::Begin("Gizmo Controls");

    // Radio buttons for operation type
    if (ImGui::RadioButton("Translate", currentOperation == ImGuizmo::TRANSLATE))
        currentOperation = ImGuizmo::TRANSLATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate", currentOperation == ImGuizmo::ROTATE))
        currentOperation = ImGuizmo::ROTATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", currentOperation == ImGuizmo::SCALE))
        currentOperation = ImGuizmo::SCALE;

    // Radio buttons for coordinate system mode
    if (currentOperation != ImGuizmo::SCALE) {
        if (ImGui::RadioButton("Local", currentMode == ImGuizmo::LOCAL))
            currentMode = ImGuizmo::LOCAL;
        ImGui::SameLine();
        if (ImGui::RadioButton("World", currentMode == ImGuizmo::WORLD))
            currentMode = ImGuizmo::WORLD;
    }

    ImGui::Separator();

    // The core gizmo function call
    ImGuizmo::Manipulate(
        glm::value_ptr(cameraView),
        glm::value_ptr(cameraProjection),
        currentOperation,
        currentMode,
        glm::value_ptr(objectMatrix) // Note: objectMatrix is now a parameter
    );

    // Decompose matrix and display values for feedback
    if (ImGuizmo::IsUsing()) {
        glm::vec3 scale, translation;
        glm::quat rotation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(objectMatrix, scale, rotation, translation, skew, perspective);

        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Object Transform");
        ImGui::Text("Translation: %.3f, %.3f, %.3f", translation.x, translation.y, translation.z);
        ImGui::Text("Rotation: %.3f, %.3f, %.3f, %.3f", rotation.w, rotation.x, rotation.y, rotation.z);
        ImGui::Text("Scale: %.3f, %.3f, %.3f", scale.x, scale.y, scale.z);
    }

    ImGui::End();
}
// --- End Change ---