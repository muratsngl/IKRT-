#include "include/application_logic.hpp"
#include "include/model_loader.hpp"
#include "include/model_bones.h"
#include "include/bone_hierarchy.hpp"
#include "collision.hpp"
#include <GLFW/glfw3.h>
#include <cmath>
#include <iostream>


static ApplicationState app_state;

void init_application_state() {
    // Initialize timing
    app_state.deltaTime = 0.0f;
    app_state.lastFrame = 0.0f;
    
    // Initialize animation mode
    app_state.animationMode = MODE_FORWARD_KINEMATICS;
}

void update_finger_positions() {
    float currentFrame = static_cast<float>(glfwGetTime());
    app_state.deltaTime = currentFrame - app_state.lastFrame;
    app_state.lastFrame = currentFrame; 
}

// User will implement IK logic using IK chains
void apply_fabrik() {
    // TODO: User will implement FABRIK solving using IKChainManager
}

// User will implement collision detection using IK chains
void rearrange_finger_positions_based_on_collision(){
    // TODO: User will implement collision detection using IKChainManager
};



// User will implement transform updates using IK chains
void update_transforms() {
    // TODO: User will implement transform updates using IKChainManager
}

ApplicationState& get_application_state() {
    return app_state;
}

void reset_application_state() {
    // Reset timing (keep current time to avoid jump)
    app_state.deltaTime = 0.0f;
    
    // Reset to FK mode
    app_state.animationMode = MODE_FORWARD_KINEMATICS;
    
    std::cout << "Application state reset for new interactor model" << std::endl;
}

// User will implement IK target management using IKChainManager

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
void recompute_bone_hierarchy_from(int bone_id, int model_index) {
    InteractorModel* model = nullptr;
    InteractorModelData* model_data_ptr = nullptr;
    
    if (model_index >= 0) {
        model = get_interactor_model_by_index(model_index);
        if (model) {
            model_data_ptr = &get_interactor_model_data_by_index(model_index);
        }
    } else {
        if (is_interactor_model_available()) {
            model = get_interactor_model();
            model_data_ptr = &get_interactor_model_data_mutable();
        }
    }
    
    if (!model || !model_data_ptr) return;
    
    InteractorModelData& model_data = *model_data_ptr;
    
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
            model_data.bind_pose_positions[childId];
            updateChildren(child);
            
        }
    };
    // Start recursive update from this bone
    updateChildren(bone);
    
}
