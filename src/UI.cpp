#include "include/UI.hpp"
#include "include/collision_visualizer.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

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

void render_ui() {
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // Render all UI components
    RenderModeSwitcher();
    RenderModelLoaderWidget();
    
    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// --- Individual UI Component Functions ---

// --- Your main UI rendering function ---
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
        static bool renderCollisionGeometry = CollisionVisualizer::getEnabled();
        
        if (ImGui::Checkbox("Render Collision Geometry", &renderCollisionGeometry)) {
            CollisionVisualizer::setEnabled(renderCollisionGeometry);
        }
        
        // Sync checkbox state with collision visualizer state
        renderCollisionGeometry = CollisionVisualizer::getEnabled();
    }

    ImGui::End();


    // 2. DISPLAY AND HANDLE THE FILE DIALOG
    // =======================================
    // This part is crucial. It displays the dialog if it's open and
    // handles the user's selection.
    if (ImGuiFileDialog::Instance()->Display("ChooseModelFileDlgKey")) {
        // Check if the user clicked "OK"
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string file_path = ImGuiFileDialog::Instance()->GetFilePathName();

            // Based on the state we saved earlier, call the correct function
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
                    break; // Should not happen
            }

            // Reset the state for the next time
            current_model_type_to_load = ModelType::None;
        }

        // Close the dialog instance
        ImGuiFileDialog::Instance()->Close();
    }
}

// --- Mode switcher UI function ---
void RenderModeSwitcher() {
    // Render minimalistic mode switcher at the top
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(200, 35), ImGuiCond_Always);
    
    ImGui::Begin("Mode", nullptr, 
        ImGuiWindowFlags_NoTitleBar | 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar);
    
    // Mode names for the combo box
    const char* mode_names[] = {"Edit Scene", "Free View", "Animate"};
    static int current_mode_index = get_app_mode();
    
    ImGui::Text("Mode:");
    ImGui::SameLine();
    
    // Set combo box width to fill remaining space
    ImGui::SetNextItemWidth(-1);
    
    if (ImGui::Combo("##mode_combo", &current_mode_index, mode_names, 3)) {
        set_app_mode((MODE)current_mode_index);
    }
    
    ImGui::End();
}