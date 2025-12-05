#include "include/animation_manager.hpp"
#include "include/render_setup.hpp"
#include "include/model_loader.hpp"
#include "include/application_logic.hpp"
#include "include/bone_hierarchy.hpp"
#include "include/model_bones.h"
#include "include/misc_math.hpp"
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>

using json = nlohmann::json;

// Helper functions for serialization
static json serializeMat4(const glm::mat4& m) {
    std::vector<float> data;
    const float* p = glm::value_ptr(m);
    for (int i = 0; i < 16; ++i) data.push_back(p[i]);
    return data;
}

static glm::mat4 deserializeMat4(const json& j) {
    std::vector<float> data = j.get<std::vector<float>>();
    return glm::make_mat4(data.data());
}

static json serializeVec3(const glm::vec3& v) {
    return {v.x, v.y, v.z};
}

static glm::vec3 deserializeVec3(const json& j) {
    return glm::vec3(j[0], j[1], j[2]);
}

AnimationManager& get_animation_manager() {
    return AnimationManager::getInstance();
}

// Remove a keyframe at a specific frame from a sequence
bool AnimationManager::removeKeyframeAtFrame(int sequenceIndex, int frame, bool isSceneElement) {
    if (isSceneElement) {
        if (sequenceIndex < 0 || sequenceIndex >= sceneElementSequences.size()) return false;
        
        auto& keyframes = sceneElementSequences[sequenceIndex].keyframes;
        for (auto it = keyframes.begin(); it != keyframes.end(); ++it) {
            if (it->frameIndex == frame) {
                keyframes.erase(it);
                std::cout << "Removed scene element keyframe at frame " << frame << std::endl;
                return true;
            }
        }
    } else {
        if (sequenceIndex < 0 || sequenceIndex >= sequences.size()) return false;
        
        auto& keyframes = sequences[sequenceIndex].keyframes;
        for (auto it = keyframes.begin(); it != keyframes.end(); ++it) {
            if (it->frameIndex == frame) {
                keyframes.erase(it);
                std::cout << "Removed skeletal keyframe at frame " << frame << std::endl;
                return true;
            }
        }
    }
    return false;
}

// Remove an entire sequence
bool AnimationManager::removeSequence(int sequenceIndex, bool isSceneElement) {
    if (isSceneElement) {
        if (sequenceIndex < 0 || sequenceIndex >= sceneElementSequences.size()) return false;
        
        std::cout << "Removed scene element sequence: " << sceneElementSequences[sequenceIndex].name << std::endl;
        sceneElementSequences.erase(sceneElementSequences.begin() + sequenceIndex);
        
        // Update active index if needed
        if (activeSceneElementSequenceIndex >= sceneElementSequences.size()) {
            activeSceneElementSequenceIndex = sceneElementSequences.empty() ? -1 : sceneElementSequences.size() - 1;
        }
        return true;
    } else {
        if (sequenceIndex < 0 || sequenceIndex >= sequences.size()) return false;
        
        std::cout << "Removed skeletal sequence: " << sequences[sequenceIndex].name << std::endl;
        sequences.erase(sequences.begin() + sequenceIndex);
        
        // Update active index if needed
        if (activeSequenceIndex >= sequences.size()) {
            activeSequenceIndex = sequences.empty() ? -1 : sequences.size() - 1;
        }
        return true;
    }
}

// Copy a sequence to clipboard
void AnimationManager::copySequence(int sequenceIndex, bool isSceneElement) {
    clipboard.isValid = false;
    clipboard.isSceneElement = isSceneElement;
    
    if (isSceneElement) {
        if (sequenceIndex < 0 || sequenceIndex >= sceneElementSequences.size()) return;
        
        auto& seq = sceneElementSequences[sequenceIndex];
        clipboard.name = seq.name + " (Copy)";
        clipboard.modelID = seq.modelID;
        clipboard.sceneElementKeyframes = seq.keyframes;
        clipboard.isValid = true;
        std::cout << "Copied scene element sequence: " << seq.name << std::endl;
    } else {
        if (sequenceIndex < 0 || sequenceIndex >= sequences.size()) return;
        
        auto& seq = sequences[sequenceIndex];
        clipboard.name = seq.name + " (Copy)";
        clipboard.modelID = seq.modelID;
        clipboard.skeletalKeyframes = seq.keyframes;
        clipboard.isValid = true;
        std::cout << "Copied skeletal sequence: " << seq.name << std::endl;
    }
}

// Paste sequence from clipboard, offsetting all keyframes to start at atFrame
void AnimationManager::pasteSequence(int atFrame) {
    if (!clipboard.isValid) return;
    
    if (clipboard.isSceneElement) {
        // Find the earliest keyframe to calculate offset
        int minFrame = INT_MAX;
        for (const auto& kf : clipboard.sceneElementKeyframes) {
            if (kf.frameIndex < minFrame) minFrame = kf.frameIndex;
        }
        
        int offset = atFrame - minFrame;
        
        // Create new sequence with offset keyframes
        SceneElementSequence newSeq(clipboard.name, clipboard.modelID);
        for (auto kf : clipboard.sceneElementKeyframes) {
            kf.frameIndex += offset;
            newSeq.addKeyframe(kf);
        }
        
        sceneElementSequences.push_back(newSeq);
        activeSceneElementSequenceIndex = sceneElementSequences.size() - 1;
        std::cout << "Pasted scene element sequence at frame " << atFrame << std::endl;
    } else {
        // Find the earliest keyframe to calculate offset
        int minFrame = INT_MAX;
        for (const auto& kf : clipboard.skeletalKeyframes) {
            if (kf.frameIndex < minFrame) minFrame = kf.frameIndex;
        }
        
        int offset = atFrame - minFrame;
        
        // Create new sequence with offset keyframes
        Sequence newSeq(clipboard.name, clipboard.modelID);
        for (auto kf : clipboard.skeletalKeyframes) {
            kf.frameIndex += offset;
            newSeq.addKeyframe(kf);
        }
        
        sequences.push_back(newSeq);
        activeSequenceIndex = sequences.size() - 1;
        std::cout << "Pasted skeletal sequence at frame " << atFrame << std::endl;
    }
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

        const Keyframe* prevKf = seq.getPrevKeyframe(frame);
        const Keyframe* nextKf = seq.getNextKeyframe(frame);
        
        // If no keyframes at all
        if (!prevKf && !nextKf) continue;
        
        // If only one keyframe or we are at the boundaries
        if (!prevKf) prevKf = nextKf; // Before first keyframe
        if (!nextKf) nextKf = prevKf; // After last keyframe
        
        // Calculate interpolation factor
        float alpha = 0.0f;
        if (prevKf != nextKf) {
            float duration = (float)(nextKf->frameIndex - prevKf->frameIndex);
            if (duration > 0.0001f) {
                alpha = (float)(frame - prevKf->frameIndex) / duration;
            }
        }
        
        // Apply bone transforms
        if (seq.modelID >= 0) { // Assuming positive IDs are models
            // Check if it's the interactor model (special handling)
            // Use the model ID from the sequence to get the correct model data
            InteractorModel* model = get_interactor_model_by_index(seq.modelID);
            if (model) {
                InteractorModelData& data = get_interactor_model_data_by_index(seq.modelID);
                
                // We need to iterate over all bones that are present in EITHER keyframe
                // For bones present in both, interpolate
                // For bones present in only one, hold value (step)
                
                // Collect all unique bone IDs from both keyframes
                std::vector<int> boneIDs;
                for (const auto& pair : prevKf->boneTransforms) boneIDs.push_back(pair.first);
                for (const auto& pair : nextKf->boneTransforms) boneIDs.push_back(pair.first);
                
                // Sort and remove duplicates
                std::sort(boneIDs.begin(), boneIDs.end());
                boneIDs.erase(std::unique(boneIDs.begin(), boneIDs.end()), boneIDs.end());
                
                for (int boneID : boneIDs) {
                    if (boneID >= data.tot_transformation_matrices.size()) continue;
                    
                    bool inPrev = prevKf->boneTransforms.count(boneID);
                    bool inNext = nextKf->boneTransforms.count(boneID);
                    
                    glm::mat4 finalTransform;
                    
                    if (inPrev && inNext) {
                        // Interpolate
                        finalTransform = interpolate_transform(
                            prevKf->boneTransforms.at(boneID),
                            nextKf->boneTransforms.at(boneID),
                            alpha
                        );
                    } else if (inPrev) {
                        // Hold previous
                        finalTransform = prevKf->boneTransforms.at(boneID);
                    } else {
                        // Use next (shouldn't happen with getPrevKeyframe logic unless we are before first kf)
                        finalTransform = nextKf->boneTransforms.at(boneID);
                    }
                    
                    data.tot_transformation_matrices[boneID] = finalTransform;
                }
                
                // Trigger hierarchy update if needed (FK)
                recompute_bone_hierarchy_from(0, seq.modelID); // Recompute from root for specific model
            }
        }
        
        // Apply target proxy positions (IK targets)
        ApplicationState& app_state = get_application_state();
        
        // Collect all unique proxy IDs
        std::vector<int> proxyIDs;
        for (const auto& pair : prevKf->targetPositions) proxyIDs.push_back(pair.first);
        for (const auto& pair : nextKf->targetPositions) proxyIDs.push_back(pair.first);
        
        std::sort(proxyIDs.begin(), proxyIDs.end());
        proxyIDs.erase(std::unique(proxyIDs.begin(), proxyIDs.end()), proxyIDs.end());
        
        for (int proxyID : proxyIDs) {
            bool inPrev = prevKf->targetPositions.count(proxyID);
            bool inNext = nextKf->targetPositions.count(proxyID);
            
            glm::vec3 finalPos;
            
            if (inPrev && inNext) {
                finalPos = interpolate_position(
                    prevKf->targetPositions.at(proxyID),
                    nextKf->targetPositions.at(proxyID),
                    alpha
                );
            } else if (inPrev) {
                finalPos = prevKf->targetPositions.at(proxyID);
            } else {
                finalPos = nextKf->targetPositions.at(proxyID);
            }
            
            switch(proxyID) {
                case TARGET_PROXY_INDEX: app_state.targetPositionIndex = finalPos; break;
                case TARGET_PROXY_MIDDLE: app_state.targetPositionMiddle = finalPos; break;
                case TARGET_PROXY_RING: app_state.targetPositionRing = finalPos; break;
                case TARGET_PROXY_PINKY: app_state.targetPositionPinky = finalPos; break;
            }
        }
    }

    // For each enabled scene element sequence
    for (auto& seq : sceneElementSequences) {
        if (!seq.enabled) continue;

        const SceneElementKeyframe* prevKf = seq.getPrevKeyframe(frame);
        const SceneElementKeyframe* nextKf = seq.getNextKeyframe(frame);
        
        // If no keyframes at all
        if (!prevKf && !nextKf) continue;
        
        // If only one keyframe or we are at the boundaries
        if (!prevKf) prevKf = nextKf; // Before first keyframe
        if (!nextKf) nextKf = prevKf; // After last keyframe
        
        // Calculate interpolation factor
        float alpha = 0.0f;
        if (prevKf != nextKf) {
            float duration = (float)(nextKf->frameIndex - prevKf->frameIndex);
            if (duration > 0.0001f) {
                alpha = (float)(frame - prevKf->frameIndex) / duration;
            }
        }

        // Interpolate transform
        glm::mat4 finalTransform = interpolate_transform(prevKf->transform, nextKf->transform, alpha);
        
        // Apply to model - use the sequence's modelID (consistent across all keyframes in sequence)
        int targetModelID = seq.modelID;
        if (targetModelID >= 0) {
            set_model_matrix_by_id(targetModelID, finalTransform);
        }
    }
}

void AnimationManager::recordSceneElementKeyframe(int modelID) {
    SceneElementSequence* seq = getSceneElementSequenceForModel(modelID);
    if (!seq) {
        // Check if active sequence is unassigned (modelID == -1)
        if (activeSceneElementSequenceIndex >= 0 && activeSceneElementSequenceIndex < sceneElementSequences.size()) {
            SceneElementSequence* activeSeq = &sceneElementSequences[activeSceneElementSequenceIndex];
            if (activeSeq->modelID == -1) {
                // Assign this unassigned sequence to the model
                activeSeq->modelID = modelID;
                seq = activeSeq;
                std::cout << "Assigned sequence '" << activeSeq->name << "' to model " << modelID << std::endl;
            }
        }
        
        // Create new sequence if still none exists
        if (!seq) {
            SceneElementSequence newSeq("New Scene Element Sequence", modelID);
            addSceneElementSequence(newSeq);
            seq = getSceneElementSequenceForModel(modelID);
        }
    }

    // Get current transform
    glm::mat4 currentTransform = get_model_matrix_by_id(modelID);
    
    SceneElementKeyframe newKf(currentFrame, modelID, currentTransform);
    seq->addKeyframe(newKf);
    
    std::cout << "Recorded scene element keyframe at frame " << currentFrame << " for model " << modelID << std::endl;
}

void AnimationManager::recordKeyframe(int modelID) {
    Sequence* seq = getSequenceForModel(modelID);
    if (!seq) {
        // Check if active sequence is unassigned (modelID == -1)
        if (activeSequenceIndex >= 0 && activeSequenceIndex < sequences.size()) {
            Sequence* activeSeq = &sequences[activeSequenceIndex];
            if (activeSeq->modelID == -1) {
                // Assign this unassigned sequence to the model
                activeSeq->modelID = modelID;
                seq = activeSeq;
                std::cout << "Assigned sequence '" << activeSeq->name << "' to model " << modelID << std::endl;
            }
        }
        
        // Create new sequence if still none exists
        if (!seq) {
            Sequence newSeq("New Sequence", modelID);
            addSequence(newSeq);
            seq = getSequenceForModel(modelID);
        }
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
        // Use the model ID passed to the function
        const InteractorModelData& data = get_interactor_model_data_by_index(modelID);
        
        for (int boneID : data.dirty_bone_indices) {
            if (boneID < data.tot_transformation_matrices.size()) {
                newKf.boneTransforms[boneID] = data.tot_transformation_matrices[boneID];
            }
        }
        
        // Recompute hierarchy on root as requested
        recompute_bone_hierarchy_from(0, modelID);
    }

    seq->addKeyframe(newKf);
    std::cout << "Recorded keyframe at frame " << currentFrame << " for model " << modelID << std::endl;
}

bool AnimationManager::saveToFile(const std::string& filepath) {
    try {
        json j;
        j["version"] = "1.0";
        j["sequences"] = json::array();

        for (const auto& seq : sequences) {
            json seqJson;
            seqJson["name"] = seq.name;
            seqJson["modelID"] = seq.modelID;
            seqJson["enabled"] = seq.enabled;
            seqJson["keyframes"] = json::array();

            for (const auto& kf : seq.keyframes) {
                json kfJson;
                kfJson["frame"] = kf.frameIndex;
                
                kfJson["bones"] = json::object();
                for (const auto& pair : kf.boneTransforms) {
                    kfJson["bones"][std::to_string(pair.first)] = serializeMat4(pair.second);
                }

                kfJson["targets"] = json::object();
                for (const auto& pair : kf.targetPositions) {
                    kfJson["targets"][std::to_string(pair.first)] = serializeVec3(pair.second);
                }

                seqJson["keyframes"].push_back(kfJson);
            }
            j["sequences"].push_back(seqJson);
        }

        j["sceneElementSequences"] = json::array();
        for (const auto& seq : sceneElementSequences) {
            json seqJson;
            seqJson["name"] = seq.name;
            seqJson["modelID"] = seq.modelID;
            seqJson["enabled"] = seq.enabled;
            seqJson["keyframes"] = json::array();

            for (const auto& kf : seq.keyframes) {
                json kfJson;
                kfJson["frame"] = kf.frameIndex;
                kfJson["modelID"] = kf.modelID;
                kfJson["transform"] = serializeMat4(kf.transform);
                seqJson["keyframes"].push_back(kfJson);
            }
            j["sceneElementSequences"].push_back(seqJson);
        }

        std::ofstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << filepath << std::endl;
            return false;
        }
        file << j.dump(4);
        file.close();
        std::cout << "Animation data saved to: " << filepath << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving animation data: " << e.what() << std::endl;
        return false;
    }
}

bool AnimationManager::loadFromFile(const std::string& filepath) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for reading: " << filepath << std::endl;
            return false;
        }

        json j;
        file >> j;
        file.close();

        if (j.contains("sequences")) {
            for (const auto& seqJson : j["sequences"]) {
                Sequence seq(seqJson["name"], seqJson["modelID"]);
                seq.enabled = seqJson.value("enabled", true);

                if (seqJson.contains("keyframes")) {
                    for (const auto& kfJson : seqJson["keyframes"]) {
                        Keyframe kf(kfJson["frame"]);

                        if (kfJson.contains("bones")) {
                            for (auto& el : kfJson["bones"].items()) {
                                int boneID = std::stoi(el.key());
                                kf.boneTransforms[boneID] = deserializeMat4(el.value());
                            }
                        }

                        if (kfJson.contains("targets")) {
                            for (auto& el : kfJson["targets"].items()) {
                                int targetID = std::stoi(el.key());
                                kf.targetPositions[targetID] = deserializeVec3(el.value());
                            }
                        }
                        seq.addKeyframe(kf);
                    }
                }
                sequences.push_back(seq);
            }
        }

        if (j.contains("sceneElementSequences")) {
            for (const auto& seqJson : j["sceneElementSequences"]) {
                SceneElementSequence seq(seqJson["name"], seqJson["modelID"]);
                seq.enabled = seqJson.value("enabled", true);

                if (seqJson.contains("keyframes")) {
                    for (const auto& kfJson : seqJson["keyframes"]) {
                        int frame = kfJson["frame"];
                        int modelID = kfJson["modelID"];
                        glm::mat4 transform = deserializeMat4(kfJson["transform"]);
                        SceneElementKeyframe kf(frame, modelID, transform);
                        seq.addKeyframe(kf);
                    }
                }
                sceneElementSequences.push_back(seq);
            }
        }

        std::cout << "Animation data loaded from: " << filepath << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading animation data: " << e.what() << std::endl;
        return false;
    }
}
