#ifndef IO_HPP
#define IO_HPP
#include <imgui.h>
#include "ImGuiFileDialog.h"
#include "model_loader.hpp"

// An enum to track which model type we want to load.
// This is needed because the file dialog is non-blocking.
enum class ModelType {
    None,
    SceneElement,
    Interactor,
    Interactable
};

// We need a static variable to remember which button was clicked.
static ModelType current_model_type_to_load = ModelType::None;

// --- Your main UI rendering function ---
void RenderModelLoaderWidget();

#endif