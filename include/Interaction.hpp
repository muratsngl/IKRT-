#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <queue>


/*BRIEF:
        This file contains necessary animation datatypes.
         InteractionEndEffectorType: Which of the puppets extremities does this interaction support
         Phase: Atomic element of an animation sequence either a rotation or a translation
                1.rotation->axis of rotation,relative position to the axis, rotation amount for each end effector;
                2.translation-> new positions of the end effector. relative to the center point.
*/

// End effector types for collision tracking
enum EndEffectorType {
    RIGHT_HAND = 0,
    LEFT_HAND = 1,
    RIGHT_LEG = 2,
    LEFT_LEG = 3,
    NUM_END_EFFECTORS = 4
};

// Global collision queues for each end effector
extern std::queue<int> right_hand_collisions;
extern std::queue<int> left_hand_collisions;
extern std::queue<int> right_leg_collisions;
extern std::queue<int> left_leg_collisions;








typedef struct Phase{
    int duration; //in milliseconds
    union{
        struct{
            glm::vec3 relative_position[5];
            float rotation_amount[5];
            glm::vec3 axis;
        }rotation;
        struct{
            glm::vec3 new_position;
        }translation;
    };
    bool isRotation;
}Phase;

typedef struct Interaction{
    int object_eligibility;
    std::vector<Phase> interaction_parts;
}Interaction;

// Function declarations for collision tracking and interaction selection
void add_collision_to_end_effector(EndEffectorType effector, int interactable_model_id);
bool choose_interaction(EndEffectorType effector, int& selected_model_id);
bool check_object_eligibility(const Interaction& interaction, EndEffectorType effector);

// Helper function to get collision queue for an end effector
std::queue<int>& get_collision_queue(EndEffectorType effector);