#include "include/animation_manager.hpp"
#include "include/render_setup.hpp"
#include "include/model_loader.hpp"
#include "include/application_logic.hpp"
#include "include/bone_hierarchy.hpp"
#include "include/model_bones.h"
#include <iostream>

AnimationManager& get_animation_manager() {
    return AnimationManager::getInstance();
}

void AnimationManager::update(float deltaTime) {
    if (!isPlaying) return;

    timeAccumulator += deltaTime;
    float frameTime = 1.0f / fps;

    if (timeAccumulator >= frameTime) {
        currentFrame++;
        timeAccumulator -= frameTime;

        if (currentFrame > frameMax) {
            currentFrame = frameMin; // Loop
        }
        
        applyFrame(currentFrame);
    }
}

void AnimationManager::applyFrame(int frame) {
    // For each enabled sequence
    for (auto& seq : sequences) {
        if (!seq.enabled) continue;

        // Simple step interpolation (hold previous value)
        // In a real system, we would interpolate between prev and next keyframes
        const Keyframe* kf = seq.getPrevKeyframe(frame);
        
        if (kf) {
            // Apply bone transforms
            if (seq.modelID >= 0) { // Assuming positive IDs are models
                // Check if it's the interactor model (special handling)
                if (is_interactor_model_available() && get_interactor_model()) {
                    // TODO: We need a way to map seq.modelID to the actual interactor pointer if we have multiple
                    // For now, assume seq.modelID corresponds to the loaded interactor
                    
                    InteractorModelData& data = get_interactor_model_data_mutable();
                    
                    for (const auto& pair : kf->boneTransforms) {
                        int boneID = pair.first;
                        const glm::mat4& transform = pair.second;
                        
                        if (boneID < data.tot_transformation_matrices.size()) {
                            data.tot_transformation_matrices[boneID] = transform;
                        }
                    }
                    
                    // Trigger hierarchy update if needed (FK)
                    // This is expensive to do for every bone, ideally we do it once per frame per model
                    recompute_bone_hierarchy_from(0); // Recompute from root
                }
            }
            
            // Apply target proxy positions (IK targets)
            ApplicationState& app_state = get_application_state();
            for (const auto& pair : kf->targetPositions) {
                int proxyID = pair.first;
                glm::vec3 pos = pair.second;
                
                switch(proxyID) {
                    case TARGET_PROXY_INDEX: app_state.targetPositionIndex = pos; break;
                    case TARGET_PROXY_MIDDLE: app_state.targetPositionMiddle = pos; break;
                    case TARGET_PROXY_RING: app_state.targetPositionRing = pos; break;
                    case TARGET_PROXY_PINKY: app_state.targetPositionPinky = pos; break;
                }
            }
        }
    }
}

void AnimationManager::recordKeyframe(int modelID) {
    Sequence* seq = getSequenceForModel(modelID);
    if (!seq) {
        // Create new sequence if none exists
        Sequence newSeq("New Sequence", modelID);
        addSequence(newSeq);
        seq = getSequenceForModel(modelID);
    }

    Keyframe newKf(currentFrame);

    // Capture state based on model type
    // 1. Capture Target Proxies (IK)
    // We treat target proxies as part of the "Interactor" model logic for now
    // or we could have a separate "IK Control" sequence.
    // Let's assume modelID 0 is the main interactor.
    
    if (is_interactor_model_available()) {
        // Capture IK Targets
        ApplicationState& app_state = get_application_state();
        newKf.targetPositions[TARGET_PROXY_INDEX] = app_state.targetPositionIndex;
        newKf.targetPositions[TARGET_PROXY_MIDDLE] = app_state.targetPositionMiddle;
        newKf.targetPositions[TARGET_PROXY_RING] = app_state.targetPositionRing;
        newKf.targetPositions[TARGET_PROXY_PINKY] = app_state.targetPositionPinky;
        
        // Capture Bone Transforms (FK)
        // Use the dirty bone indices list to determine which bones to keyframe
        const InteractorModelData& data = get_interactor_model_data();
        
        for (int boneID : data.dirty_bone_indices) {
            if (boneID < data.tot_transformation_matrices.size()) {
                newKf.boneTransforms[boneID] = data.tot_transformation_matrices[boneID];
            }
        }
        
        // Recompute hierarchy on root as requested
        recompute_bone_hierarchy_from(0);
    }

    seq->addKeyframe(newKf);
    std::cout << "Recorded keyframe at frame " << currentFrame << " for model " << modelID << std::endl;
}
