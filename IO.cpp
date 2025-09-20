#include "include/IO.hpp"
#include "include/collision_visualizer.hpp"

// --- Your main UI rendering function ---
void RenderModelLoaderWidget() {
    // Create a small collapsible operations panel in the top-left corner
    static bool show_operations = true;

    // Set window position and size for top-left corner
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
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