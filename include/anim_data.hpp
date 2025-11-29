#ifndef ANIM_DATA_HPP
#define ANIM_DATA_HPP

#include <vector>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <algorithm>

// Represents the state of the model at a specific point in time
struct Keyframe {
    int frameIndex;
    
    // Sparse storage: maps Bone ID (or Object ID) to its local transform matrix
    // Only stores matrices that have been modified ("dirty")
    std::unordered_map<int, glm::mat4> boneTransforms;
    
    // We can also store target proxy positions if we are animating IK targets
    std::unordered_map<int, glm::vec3> targetPositions;

    Keyframe(int frame) : frameIndex(frame) {}
};

// Represents a timeline of changes for a specific model
struct Sequence {
    std::string name;
    int modelID; // The ID of the model this sequence controls
    bool enabled = true;
    
    // Ordered list of keyframes
    std::vector<Keyframe> keyframes;

    Sequence(const std::string& n, int id) : name(n), modelID(id) {}

    // Helper to find a keyframe at a specific frame
    // Returns nullptr if not found
    Keyframe* getKeyframe(int frame) {
        for (auto& kf : keyframes) {
            if (kf.frameIndex == frame) return &kf;
        }
        return nullptr;
    }

    // Insert or update a keyframe
    void addKeyframe(const Keyframe& kf) {
        // Check if keyframe exists
        for (auto& existing : keyframes) {
            if (existing.frameIndex == kf.frameIndex) {
                // Merge data: overwrite existing transforms with new ones
                for (const auto& pair : kf.boneTransforms) {
                    existing.boneTransforms[pair.first] = pair.second;
                }
                for (const auto& pair : kf.targetPositions) {
                    existing.targetPositions[pair.first] = pair.second;
                }
                return;
            }
        }
        
        // If not found, add and sort
        keyframes.push_back(kf);
        std::sort(keyframes.begin(), keyframes.end(), 
            [](const Keyframe& a, const Keyframe& b) {
                return a.frameIndex < b.frameIndex;
            });
    }
    
    // Get the nearest keyframe before or at the given frame
    const Keyframe* getPrevKeyframe(int frame) const {
        const Keyframe* prev = nullptr;
        for (const auto& kf : keyframes) {
            if (kf.frameIndex > frame) break;
            prev = &kf;
        }
        return prev;
    }
    
    // Get the nearest keyframe after the given frame
    const Keyframe* getNextKeyframe(int frame) const {
        for (const auto& kf : keyframes) {
            if (kf.frameIndex > frame) return &kf;
        }
        return nullptr;
    }
};

#endif

//WHERE IS THE DIRTY MATRIX LOGIC