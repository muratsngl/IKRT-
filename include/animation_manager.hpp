#ifndef ANIMATION_MANAGER_HPP
#define ANIMATION_MANAGER_HPP

#include "anim_data.hpp"
#include <vector>

class AnimationManager {
public:
    static AnimationManager& getInstance() {
        static AnimationManager instance;
        return instance;
    }

    // Global timeline state
    int currentFrame = 0;
    int frameMin = 0;
    int frameMax = 100;
    bool isPlaying = false;
    float fps = 24.0f;
    float timeAccumulator = 0.0f;

    // Container for all sequences
    std::vector<Sequence> sequences;

    // Management
    void addSequence(const Sequence& seq) {
        sequences.push_back(seq);
    }

    Sequence* getSequenceForModel(int modelID) {
        for (auto& seq : sequences) {
            if (seq.modelID == modelID) return &seq;
        }
        return nullptr;
    }
    
    Sequence* getSequenceByIndex(int index) {
        if (index >= 0 && index < sequences.size()) {
            return &sequences[index];
        }
        return nullptr;
    }

    // Core Logic
    void update(float deltaTime);
    void applyFrame(int frame);
    void recordKeyframe(int modelID); // Captures current state of modelID into currentFrame

private:
    AnimationManager() = default;
};

// Global accessor
AnimationManager& get_animation_manager();

#endif
