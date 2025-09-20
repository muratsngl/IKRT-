#ifndef IO_HPP
#define IO_HPP
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "ImGuiFileDialog.h"
#include "ImGuizmo.h"
#include "model_loader.hpp"

// Forward declaration
struct GLFWwindow;

#include "render_setup.hpp"

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

// --- UI System Functions ---
bool init_ui(GLFWwindow* window);
void cleanup_ui();
void render_ui();
ImGuiStyle create_gui_style();

// --- Individual UI Component Functions ---
void RenderModelLoaderWidget();
void RenderModeSwitcher();

#endif