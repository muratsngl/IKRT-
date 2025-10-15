#ifndef APPLICATION_LOGIC_HPP
#define APPLICATION_LOGIC_HPP

#include <glm/glm.hpp>
#include <vector>

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

#endif
