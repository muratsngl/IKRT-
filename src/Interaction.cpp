#include "include/Interaction.hpp"

// Define the global collision queues for each end effector
std::queue<int> right_hand_collisions;
std::queue<int> left_hand_collisions;
std::queue<int> right_leg_collisions;
std::queue<int> left_leg_collisions;

// Helper function to get collision queue for an end effector
std::queue<int>& get_collision_queue(EndEffectorType effector) {
    switch (effector) {
        case RIGHT_HAND:
            return right_hand_collisions;
        case LEFT_HAND:
            return left_hand_collisions;
        case RIGHT_LEG:
            return right_leg_collisions;
        case LEFT_LEG:
            return left_leg_collisions;
        default:
            return right_hand_collisions; // Default fallback
    }
}

// Add a collision to the appropriate end effector queue
void add_collision_to_end_effector(EndEffectorType effector, int interactable_model_id) {
    std::queue<int>& queue = get_collision_queue(effector);
    queue.push(interactable_model_id);
}

// Choose interaction - returns the first collided object in the queue
bool choose_interaction(EndEffectorType effector, int& selected_model_id) {
    std::queue<int>& queue = get_collision_queue(effector);
    
    if (queue.empty()) {
        return false;
    }
    
    selected_model_id = queue.front();
    queue.pop();
    return true;
}

// Check if the end effector is eligible for the given interaction
bool check_object_eligibility(const Interaction& interaction, EndEffectorType effector) {
    // Check if the end effector type matches the object's eligibility
    // object_eligibility: right hand = 0, left hand = 1, right leg = 2, left leg = 3
    
    switch (effector) {
        case RIGHT_HAND:
            return (interaction.object_eligibility == 0);
        case LEFT_HAND:
            return (interaction.object_eligibility == 1);
        case RIGHT_LEG:
            return (interaction.object_eligibility == 2);
        case LEFT_LEG:
            return (interaction.object_eligibility == 3);
        default:
            return false;
    }
}
