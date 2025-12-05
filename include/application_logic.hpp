#ifndef APPLICATION_LOGIC_HPP
#define APPLICATION_LOGIC_HPP

#include <glm/glm.hpp>
#include <vector>
#include "collision.hpp"

// ============================================================================
// MODEL ID RANGES - DO NOT OVERLAP
// ============================================================================
// Scene element models: 0-4999
const int SCENE_ELEMENT_ID_START = 0;
const int SCENE_ELEMENT_ID_END = 4999;

// Interactable models: 5000-9999
const int INTERACTABLE_ID_START = 5000;
const int INTERACTABLE_ID_END = 9999;

// Target proxy IDs for raycast selection: 10000-10003
enum TargetProxyID {
    TARGET_PROXY_INDEX = 10000,
    TARGET_PROXY_MIDDLE = 10001,
    TARGET_PROXY_RING = 10002,
    TARGET_PROXY_PINKY = 10003
};
const int TARGET_PROXY_ID_START = 10000;
const int TARGET_PROXY_ID_END = 10003;

// Interactor models: 15000-19999
const int INTERACTOR_ID_START = 15000;
const int INTERACTOR_ID_END = 19999;

// Bone IDs for raycast selection: 20000+
// Actual bone ID will be BONE_ID_START + bone_index
const int BONE_ID_START = 20000;

// Application state structure
struct ApplicationState {
    // Target positions for each finger
    glm::vec3 targetPositionIndex;
    glm::vec3 targetPositionMiddle;
    glm::vec3 targetPositionRing;
    glm::vec3 targetPositionPinky;
    glm::vec3 targetPositionRoot;

    // Delta vectors for movement
    glm::vec3 deltaIndex;
    glm::vec3 deltaMiddle;
    glm::vec3 deltaRing;
    glm::vec3 deltaPinky;
    glm::vec3 deltaRoot; // Added for root movement
    
    // Timing
    float deltaTime;
    float lastFrame;

    // Interaction state for different end effectors
    // 0: right hand, 1: left hand, 2: right foot, 3: left foot
    // 4-8: right fingers (thumb, index, middle, ring, pinky)
    // 9-13: left fingers (thumb, index, middle, ring, pinky)
    bool isInteracting[4] = { false };
    
    // Manual control mode (hand tweaking without shared memory)
    bool manualControlMode = false;
    
    // Previous target positions for delta calculation
    glm::vec3 prevTargetPositionIndex;
    glm::vec3 prevTargetPositionMiddle;
    glm::vec3 prevTargetPositionRing;
    glm::vec3 prevTargetPositionPinky;
};

// Application logic functions
void init_application_state();
void update_finger_positions();
void apply_fabrik();
void update_transforms();
void calculate_deltas();
void rearrange_finger_positions_based_on_collision();
ApplicationState& get_application_state();

// Reset application state when loading new interactor model
void reset_application_state();

// Target position proxy functions
void init_target_proxies();
void update_target_proxies();
std::vector<Shape>& get_target_proxy_boxes();
void update_target_from_gizmo(int proxyID, const glm::vec3& newPosition);

// Bone visualization and selection functions
void update_bone_boxes();
std::vector<Shape>& get_bone_boxes();
int get_bone_id_from_shape_id(int shape_id);

// Bone hierarchy management
void recompute_bone_hierarchy_from(int bone_id, int model_index = -1);

#endif
