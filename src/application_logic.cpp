#include "include/application_logic.hpp"
#include "include/shared_memory.hpp"
#include "include/model_loader.hpp"
#include "include/model_bones.h"
#include "include/FABRIK.h"
#include "include/Interaction.hpp"
#include "include/bone_hierarchy.hpp"
#include "collision.hpp"
#include <GLFW/glfw3.h>
#include <cmath>
#include <iostream>


static ApplicationState app_state;
static std::vector<Shape> target_proxy_boxes;

void init_application_state() {
    // Initialize target positions
    app_state.targetPositionIndex = glm::vec3(-1.2f, 2.0f, 0.0f);
    app_state.targetPositionMiddle = glm::vec3(-0.3f, -1.2f, 0.0f);
    app_state.targetPositionRing = glm::vec3(1.6f, 2.0f, 0.0f);
    app_state.targetPositionPinky = glm::vec3(0.5f, -1.2f, 0.0f);
    app_state.targetPositionRoot = glm::vec3(0.0f, 0.0f, 0.0f); // Added for root movement
    
    // Initialize previous positions for delta tracking
    app_state.prevTargetPositionIndex = app_state.targetPositionIndex;
    app_state.prevTargetPositionMiddle = app_state.targetPositionMiddle;
    app_state.prevTargetPositionRing = app_state.targetPositionRing;
    app_state.prevTargetPositionPinky = app_state.targetPositionPinky;
    
    // Initialize deltas
    app_state.deltaIndex = glm::vec3(0.0f);
    app_state.deltaMiddle = glm::vec3(0.0f);
    app_state.deltaRing = glm::vec3(0.0f);
    app_state.deltaPinky = glm::vec3(0.0f);
    app_state.deltaRoot = glm::vec3(0.0f); // Added for root movement
    
    // Initialize timing
    app_state.deltaTime = 0.0f;
    app_state.lastFrame = 0.0f;
    
    // Initialize manual control mode
    app_state.manualControlMode = false;
    
    // Initialize target proxies
    init_target_proxies();
}

void update_finger_positions() {
    float currentFrame = static_cast<float>(glfwGetTime());
    app_state.deltaTime = currentFrame - app_state.lastFrame;
    app_state.lastFrame = currentFrame; 
}

void calculate_deltas() {
    if (!is_shared_memory_available()) {
        // Set all delta values to 0 when shared memory is not available
        app_state.deltaIndex = glm::vec3(0.0f);
        app_state.deltaRing = glm::vec3(0.0f);
        app_state.deltaMiddle = glm::vec3(0.0f);
        app_state.deltaPinky = glm::vec3(0.0f);
        app_state.deltaRoot = glm::vec3(0.0f);
        static bool notified = false;
        if (!notified) {
            std::cout << "Note: Hand tracking deltas set to zero - shared memory not available" << std::endl;
            notified = true;
        }
        return;
    } else {
        // Reset notification flag when shared memory becomes available
        static bool reset_notification = true;
        if (reset_notification) {
            std::cout << "Hand tracking deltas are now active - shared memory available" << std::endl;
            reset_notification = false;
        }
    }

    const FingerData& finger_data = get_finger_data();
    
    // Calculate movement deltas with scaling factors
    app_state.deltaIndex = glm::vec3(4.0f * finger_data.deltaXIndex, 
                                   2.66f * finger_data.deltaYIndex, 
                                   finger_data.deltaZIndex);
    
    app_state.deltaRing = glm::vec3(4.0f * finger_data.deltaXRing, 
                                  2.66f * finger_data.deltaYRing, 
                                  finger_data.deltaZRing);
    
    app_state.deltaMiddle = glm::vec3(4.0f * finger_data.deltaXMiddle, 
                                    2.66f * finger_data.deltaYMiddle, 
                                    finger_data.deltaZMiddle);
    
    app_state.deltaPinky = glm::vec3(4.0f * finger_data.deltaXPinky, 
                                   2.66f * finger_data.deltaYPinky, 
                                   finger_data.deltaZPinky);
    app_state.deltaRoot = !finger_data.rootLock? glm::vec3(8.0f * finger_data.deltaXRoot,
                                   5.32f * finger_data.deltaYRoot,
                                   finger_data.deltaZRoot): glm::vec3(0.0f); // Added for root movement
    
    // Update target positions - apply deltaRoot directly to all targets
    app_state.targetPositionIndex += app_state.deltaIndex ;
    app_state.targetPositionRing += app_state.deltaRing ;
    app_state.targetPositionMiddle += app_state.deltaMiddle ;
    app_state.targetPositionPinky += app_state.deltaPinky ;
    app_state.targetPositionRoot += app_state.deltaRoot; // Added for root movement
}

void apply_fabrik() {
    // TODO: Apply FABRIK using IK chains from IKChainManager instead of hardcoded indices
    // This function will be reimplemented to use dynamically created IK chains
    
    // if (!is_interactor_model_available()) {
    //     return;
    // }
    // 
    // const InteractorModelData& model_data = get_interactor_model_data();
    // apply_root_offset_to_bones(app_state.deltaRoot);
    // 
    // // Will use IKChainManager to get chains and solve them
}

void rearrange_finger_positions_based_on_collision(){
    if (!is_interactor_model_available()) {
        return; // Early return if model not available
    }
    
    const InteractorModelData& model_data = get_interactor_model_data();
    extern std::vector<Shape> scene_element_boxes;
    
    // Create OBB for bone segment between bone i and i+1
    auto createBoneOBB = [&](int boneIndex1, int boneIndex2, float halfExtentXZ) -> Shape {
        Shape boneShape;
        boneShape.type = OBB;
        
        glm::vec3 pos1 = model_data.bind_pose_matrices[boneIndex1] * glm::vec4(model_data.bind_pose_positions_original[boneIndex1], 1.0f);
        glm::vec3 pos2 = model_data.bind_pose_matrices[boneIndex2] * glm::vec4(model_data.bind_pose_positions_original[boneIndex2], 1.0f);
        // Center is midpoint between bones
        glm::vec3 center = (pos1 + pos2) * 0.5f;
        
        // Direction vector between bones
        glm::vec3 direction = pos2 - pos1;
        float length = glm::length(direction);
        
        if (length > 0.0001f) {
            direction = glm::normalize(direction);
        } else {
            direction = glm::vec3(1, 0, 0); // Default direction
        }
        
        // Create rotation to align with direction vector
        glm::vec3 up = direction;  // Bone direction becomes the up vector
        glm::vec3 forward = glm::vec3(0, 0, 1);  // Default forward
        
        // If bone direction is too close to forward, use a different forward
        if (fabs(glm::dot(up, forward)) > 0.99f) {
            forward = glm::vec3(1, 0, 0);
        }
        
        glm::vec3 right = glm::normalize(glm::cross(up, forward));
        forward = glm::normalize(glm::cross(right, up));
        
        // Build rotation matrix: right=X, up=Y, forward=Z
        glm::mat3 rotMatrix(right, up, forward);
        glm::quat rotation = glm::quat_cast(rotMatrix);
        
        // Set OBB properties
        boneShape.obb.center = center;
        boneShape.obb.rotation = rotation;
        // Half extents: parameterized width/depth (X,Z), length along bone direction (Y)
        boneShape.obb.halfExtents = glm::vec3(halfExtentXZ, length * 0.5f, halfExtentXZ);

        return boneShape;
    };
    
    // Check collision with scene elements only
    auto checkSceneElementCollision = [&](const std::vector<unsigned short>& indices, const std::string& fingerName, float halfExtentXZ) -> bool {
        bool hasCollision = false;
        for (size_t i = 0; i < indices.size() - 1; i++) {
            Shape boneOBB = createBoneOBB(indices[i], indices[i + 1], halfExtentXZ);
            
            // Check collision with scene elements only
            if (check_collision(boneOBB, scene_element_boxes)) {
                std::cout << "Scene collision detected: " << fingerName << " finger segment " << i << " is colliding with scene object!" << std::endl;
                hasCollision = true;
            }
        }
        return hasCollision;
    };
    
    // Check collision with interactable elements only
    auto checkInteractableElementCollision = [&](const std::vector<unsigned short>& indices, const std::string& fingerName, float halfExtentXZ, EndEffectorType effector) -> bool {
        bool hasCollision = false;
        for (size_t i = 0; i < indices.size() - 1; i++) {
            Shape boneOBB = createBoneOBB(indices[i], indices[i + 1], halfExtentXZ);
            
            // Check collision with interactable models only
            int interactable_id = check_collision_with_id(boneOBB, get_interactable_element_boxes());
            if (interactable_id != -1) {
                std::cout << "Interactable collision detected: " << fingerName << " finger segment " << i << " is colliding with interactable model ID: " << interactable_id << std::endl;
                
                // Add collision to the appropriate end effector queue
                add_collision_to_end_effector(effector, interactable_id);
                hasCollision = true;
            }
        }
        return hasCollision;
    };
    
    // TODO: Collision detection will use IK chains from IKChainManager
    // Commented out hardcoded collision checks
    
    // checkSceneElementCollision(chain_indices, "Chain Name", halfExtent);
    // checkInteractableElementCollision(chain_indices, "Chain Name", halfExtent, effector);
    
    int selected_model_id;
    
    // if (choose_interaction(RIGHT_HAND, selected_model_id)) {
    //     std::cout << "Right hand selected interactable model ID: " << selected_model_id << std::endl;
    // }
    
    // if (choose_interaction(LEFT_HAND, selected_model_id)) {
    //     std::cout << "Left hand selected interactable model ID: " << selected_model_id << std::endl;
    // }
    
    // if (choose_interaction(RIGHT_LEG, selected_model_id)) {
    //     std::cout << "Right leg selected interactable model ID: " << selected_model_id << std::endl;
    // }
    
    // if (choose_interaction(LEFT_LEG, selected_model_id)) {
    //     std::cout << "Left leg selected interactable model ID: " << selected_model_id << std::endl;
    // }
};



void update_transforms() {
    if (!is_interactor_model_available()) {
        return; // Early return if model not available
    }
    
    // TODO: Transform updates will use IK chains from IKChainManager
    // Commented out hardcoded transform updates
    
    // const InteractorModelData& model_data = get_interactor_model_data();
    // update_bone_transforms(chain_indices);
    // end_effector_align(chain_indices);
}

ApplicationState& get_application_state() {
    return app_state;
}

void reset_application_state() {
    // Reset target positions to default values
    // app_state.targetPositionIndex = glm::vec3(-1.2f, 2.0f, 0.0f);
    // app_state.targetPositionMiddle = glm::vec3(-0.3f, -1.2f, 0.0f);
    // app_state.targetPositionRing = glm::vec3(1.6f, 2.0f, 0.0f);
    // app_state.targetPositionPinky = glm::vec3(0.5f, -1.2f, 0.0f);
    // app_state.targetPositionRoot = glm::vec3(0.0f, 0.0f, 0.0f);
    
    // Reset deltas to zero
    app_state.deltaIndex = glm::vec3(0.0f);
    app_state.deltaMiddle = glm::vec3(0.0f);
    app_state.deltaRing = glm::vec3(0.0f);
    app_state.deltaPinky = glm::vec3(0.0f);
    app_state.deltaRoot = glm::vec3(0.0f);
    
    // Reset interaction states
    for (int i = 0; i < 4; i++) {
        app_state.isInteracting[i] = false;
    }
    
    // Reset timing (keep current time to avoid jump)
    app_state.deltaTime = 0.0f;
    // Don't reset lastFrame to avoid timing issues
    
    std::cout << "Application state reset for new interactor model" << std::endl;
}

// ============================================================================
// Target Position Proxy System
// ============================================================================

void init_target_proxies() {
    target_proxy_boxes.clear();
    
    // Create bounding boxes for each target position
    // Box size for interaction
    float boxSize = 0.15f;
    
    // Index finger target proxy
    Shape indexBox;
    indexBox.type = AABB;
    indexBox.id = TARGET_PROXY_INDEX;
    indexBox.aabb.min = app_state.targetPositionIndex - glm::vec3(boxSize);
    indexBox.aabb.max = app_state.targetPositionIndex + glm::vec3(boxSize);
    target_proxy_boxes.push_back(indexBox);
    
    // Middle finger target proxy
    Shape middleBox;
    middleBox.type = AABB;
    middleBox.id = TARGET_PROXY_MIDDLE;
    middleBox.aabb.min = app_state.targetPositionMiddle - glm::vec3(boxSize);
    middleBox.aabb.max = app_state.targetPositionMiddle + glm::vec3(boxSize);
    target_proxy_boxes.push_back(middleBox);
    
    // Ring finger target proxy
    Shape ringBox;
    ringBox.type = AABB;
    ringBox.id = TARGET_PROXY_RING;
    ringBox.aabb.min = app_state.targetPositionRing - glm::vec3(boxSize);
    ringBox.aabb.max = app_state.targetPositionRing + glm::vec3(boxSize);
    target_proxy_boxes.push_back(ringBox);
    
    // Pinky finger target proxy
    Shape pinkyBox;
    pinkyBox.type = AABB;
    pinkyBox.id = TARGET_PROXY_PINKY;
    pinkyBox.aabb.min = app_state.targetPositionPinky - glm::vec3(boxSize);
    pinkyBox.aabb.max = app_state.targetPositionPinky + glm::vec3(boxSize);
    target_proxy_boxes.push_back(pinkyBox);
    
    std::cout << "Target proxies initialized" << std::endl;
}

void update_target_proxies() {
    // Update bounding box positions based on current target positions
    float boxSize = 0.15f;
    
    if (target_proxy_boxes.size() >= 4) {
        target_proxy_boxes[0].aabb.min = app_state.targetPositionIndex - glm::vec3(boxSize);
        target_proxy_boxes[0].aabb.max = app_state.targetPositionIndex + glm::vec3(boxSize);
        
        target_proxy_boxes[1].aabb.min = app_state.targetPositionMiddle - glm::vec3(boxSize);
        target_proxy_boxes[1].aabb.max = app_state.targetPositionMiddle + glm::vec3(boxSize);
        
        target_proxy_boxes[2].aabb.min = app_state.targetPositionRing - glm::vec3(boxSize);
        target_proxy_boxes[2].aabb.max = app_state.targetPositionRing + glm::vec3(boxSize);
        
        target_proxy_boxes[3].aabb.min = app_state.targetPositionPinky - glm::vec3(boxSize);
        target_proxy_boxes[3].aabb.max = app_state.targetPositionPinky + glm::vec3(boxSize);
    }
}

std::vector<Shape>& get_target_proxy_boxes() {
    return target_proxy_boxes;
}

void update_target_from_gizmo(int proxyID, const glm::vec3& newPosition) {
    // Calculate delta and update the appropriate target position
    // This maintains the existing framework where deltas drive the system
    
    glm::vec3 delta(0.0f);
    
    switch(proxyID) {
        case TARGET_PROXY_INDEX:
            delta = newPosition - app_state.prevTargetPositionIndex;
            app_state.deltaIndex = delta;
            app_state.targetPositionIndex = newPosition;
            app_state.prevTargetPositionIndex = newPosition;
            break;
            
        case TARGET_PROXY_MIDDLE:
            delta = newPosition - app_state.prevTargetPositionMiddle;
            app_state.deltaMiddle = delta;
            app_state.targetPositionMiddle = newPosition;
            app_state.prevTargetPositionMiddle = newPosition;
            break;
            
        case TARGET_PROXY_RING:
            delta = newPosition - app_state.prevTargetPositionRing;
            app_state.deltaRing = delta;
            app_state.targetPositionRing = newPosition;
            app_state.prevTargetPositionRing = newPosition;
            break;
            
        case TARGET_PROXY_PINKY:
            delta = newPosition - app_state.prevTargetPositionPinky;
            app_state.deltaPinky = delta;
            app_state.targetPositionPinky = newPosition;
            app_state.prevTargetPositionPinky = newPosition;
            break;
    }
    
    // Update the proxy bounding boxes
    update_target_proxies();
}

// ============================================================================
// Bone Box Management for Raycast Selection
// ============================================================================

static std::vector<Shape> bone_boxes;

void update_bone_boxes() {
    bone_boxes.clear();
    
    if (!is_interactor_model_available()) return;
    
    InteractorModel* model = get_interactor_model();
    if (!model) return;
    
    const InteractorModelData& model_data = get_interactor_model_data();
    
    if (model_data.bind_pose_positions.empty() || model_data.bind_pose_matrices.empty()) return;
    
    BoneHierarchy* hierarchy = model->getBoneHierarchy();
    if (!hierarchy || !hierarchy->isValid()) return;
    
    std::vector<BoneNode*> allBones;
    hierarchy->getAllBones(allBones);
    
    // Create AABB for each bone segment (in local space, like scene elements)
    for (const BoneNode* bone : allBones) {
        if (!bone->parent) continue; // Skip root
        
        int childBoneId = bone->boneId;
        int parentBoneId = bone->parent->boneId;
        
        if (childBoneId >= model_data.bind_pose_positions.size() || 
            parentBoneId >= model_data.bind_pose_positions.size()) continue;
        
        // Get bone positions in LOCAL space (bind pose positions)
        glm::vec3 childPos = model_data.bind_pose_positions[childBoneId];
        glm::vec3 parentPos = model_data.bind_pose_positions[parentBoneId];
        
        // Calculate bone length for adaptive sizing
        float boneLength = glm::length(childPos - parentPos);
        float boneThickness = boneLength * 0.15f; // Scale thickness with bone length
        boneThickness = glm::clamp(boneThickness, 0.01f, 0.3f); // Clamp to reasonable range
        
        // Create AABB that encompasses both bone endpoints with some thickness
        glm::vec3 minBounds = glm::min(childPos, parentPos) - glm::vec3(boneThickness);
        glm::vec3 maxBounds = glm::max(childPos, parentPos) + glm::vec3(boneThickness);
        
        // Create AABB shape (in local space)
        Shape boneShape;
        boneShape.type = AABB;
        boneShape.id = BONE_ID_START + parentBoneId; // Use parent bone ID so manipulating the segment affects the parent joint
        boneShape.aabb.min = minBounds;
        boneShape.aabb.max = maxBounds;
        
        bone_boxes.push_back(boneShape);
    }
}

std::vector<Shape>& get_bone_boxes() {
    return bone_boxes;
}

int get_bone_id_from_shape_id(int shape_id) {
    if (shape_id >= BONE_ID_START) {
        return shape_id - BONE_ID_START;
    }
    return -1;
}

// Recompute bone hierarchy in FK mode starting from a bone
void recompute_bone_hierarchy_from(int bone_id) {
    if (!is_interactor_model_available()) return;
    
    InteractorModel* model = get_interactor_model();
    if (!model) return;
    
    InteractorModelData& model_data = get_interactor_model_data_mutable();
    
    // Only recompute in FK mode
    if (model_data.manipulation_mode != MODE_FORWARD_KINEMATICS) return;
    
    BoneHierarchy* hierarchy = model->getBoneHierarchy();
    if (!hierarchy || !hierarchy->isValid()) return;
    if (bone_id >= model_data.bind_pose_matrices.size()) return;
    
    BoneNode* bone = hierarchy->findBoneById(bone_id);
    if (!bone) return;
    
    // Add to dirty list
    bool alreadyDirty = false;
    for (int dirtyId : model_data.dirty_bone_indices) {
        if (dirtyId == bone_id) {
            alreadyDirty = true;
            break;
        }
    }
    if (!alreadyDirty) {
        model_data.dirty_bone_indices.push_back(bone_id);
    }

    //TODO another issue is that we should use the gizmo update only once for every click on the gizmo not a constant delta
    //CAUTION CURRENTLY NOT APPLYING THE BINDPOSE POSITION LOGIC IT WILL BE NEEEDED WHEN IKRT MODE ENABLED
    // Update this bone's local transform based on its new world matrix
    //this works dont change xD
    glm::mat4 parent = glm::mat4(1.0f);
    if (bone->parent) {
        parent = model_data.bind_pose_matrices[bone->parent->boneId];
    }
    
    // model_data.tot_transformation_matrices[bone_id] is now the source of truth
    model_data.bind_pose_matrices[bone_id] = parent * glm::inverse(model_data.offset_matrices[bone_id])* model_data.tot_transformation_matrices[bone_id]*model_data.offset_matrices[bone_id];
    glm::mat4 transform = model_data.bind_pose_matrices[bone_id]; 
    
    // offset should always map the vertex to its local coordinate system hence the modification
    
    // Recursively update all children's world transforms
    std::function<void(BoneNode*)> updateChildren = [&](BoneNode* node) {
        
        
        for (const auto& childPtr : node->children) {
            BoneNode* child = childPtr.get();
            int childId = child->boneId;
            
        if (childId >= model_data.bind_pose_matrices.size()) continue;
            
            // Recursively update this child's children
            model_data.bind_pose_matrices[childId] = model_data.bind_pose_matrices[node->boneId]*glm::inverse(model_data.offset_matrices[childId])* model_data.tot_transformation_matrices[childId]*model_data.offset_matrices[childId];
            updateChildren(child);
            
        }
    };
    // Start recursive update from this bone
    updateChildren(bone);
    
}
