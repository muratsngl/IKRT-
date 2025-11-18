#ifndef APPLICATION_LOGIC_HPP
#define APPLICATION_LOGIC_HPP

#include <glm/glm.hpp>
#include <vector>
#include "collision.hpp"

// Target proxy IDs for raycast selection (must not conflict with scene/interactable model IDs)
enum TargetProxyID {
    TARGET_PROXY_INDEX = 10000,
    TARGET_PROXY_MIDDLE = 10001,
    TARGET_PROXY_RING = 10002,
    TARGET_PROXY_PINKY = 10003
};

// Bone IDs for raycast selection (start at 20000 to avoid conflicts)
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
void recompute_bone_hierarchy_from(int bone_id);

#endif
