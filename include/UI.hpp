#ifndef IO_HPP
#define IO_HPP
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "ImGuiFileDialog.h"
#include "ImGuizmo.h"
#include "model_loader.hpp"
#include "light_manager.hpp"

// Forward declarations
struct GLFWwindow;
class IKChainManager;

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
void RenderGizmoUI(const glm::mat4& cameraView, const glm::mat4& cameraProjection, glm::mat4& objectMatrix);
void RenderLightManagerWidget();

// --- IK Chain Building State Access ---
bool is_building_chain();
bool is_bone_in_current_chain(int bone_id);
void add_bone_to_current_chain(int bone_id);
void remove_last_bone_from_current_chain();
IKChainManager& get_chain_manager();

#endif