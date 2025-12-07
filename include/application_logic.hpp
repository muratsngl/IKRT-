#ifndef APPLICATION_LOGIC_HPP
#define APPLICATION_LOGIC_HPP

#include <glm/glm.hpp>
#include <vector>
#include "collision.hpp"
#include "model_loader.hpp"

// ============================================================================
// MODEL ID RANGES - DO NOT OVERLAP
// ============================================================================
// Scene element models: 0-4999
const int SCENE_ELEMENT_ID_START = 0;
const int SCENE_ELEMENT_ID_END = 4999;

// Interactable models: 5000-9999
const int INTERACTABLE_ID_START = 5000;
const int INTERACTABLE_ID_END = 9999;

// Interactor models: 15000-19999
const int INTERACTOR_ID_START = 15000;
const int INTERACTOR_ID_END = 19999;

// Bone IDs for raycast selection: 20000+
// Actual bone ID will be BONE_ID_START + bone_index
const int BONE_ID_START = 20000;

// Application state structure
struct ApplicationState {
    // Timing
    float deltaTime;
    float lastFrame;
    
    // Animation mode (FK/IK)
    BoneManipulationMode animationMode = MODE_FORWARD_KINEMATICS;
};

// Application logic functions
void init_application_state();
void update_finger_positions();
void apply_fabrik();
void update_transforms();
void rearrange_finger_positions_based_on_collision();
ApplicationState& get_application_state();

// Reset application state when loading new interactor model
void reset_application_state();

// Bone visualization and selection functions
void update_bone_boxes();
std::vector<Shape>& get_bone_boxes();
int get_bone_id_from_shape_id(int shape_id);

// Target proxy visualization and selection for IK chains
void update_target_proxy_boxes();
std::vector<Shape>& get_target_proxy_boxes();
int get_chain_id_from_target_proxy_id(int proxy_id);

// Bone hierarchy management (FK mode)
void recompute_bone_hierarchy_from(int bone_id, int model_index = -1);

#endif
