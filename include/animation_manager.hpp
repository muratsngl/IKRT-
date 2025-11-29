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
    int activeSequenceIndex = -1;

    // Management
    void addSequence(const Sequence& seq) {
        sequences.push_back(seq);
        if (activeSequenceIndex == -1) {
            activeSequenceIndex = 0;
        }
    }

    void createNewSequence(int modelID, const std::string& name) {
        sequences.emplace_back(name, modelID);
        activeSequenceIndex = sequences.size() - 1;
    }

    Sequence* getActiveSequence() {
        if (activeSequenceIndex >= 0 && activeSequenceIndex < sequences.size()) {
            return &sequences[activeSequenceIndex];
        }
        return nullptr;
    }

    Sequence* getSequenceForModel(int modelID) {
        // Prefer active sequence if it matches the model
        if (activeSequenceIndex >= 0 && activeSequenceIndex < sequences.size()) {
            if (sequences[activeSequenceIndex].modelID == modelID) {
                return &sequences[activeSequenceIndex];
            }
        }

        // Fallback to first matching
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

    // Serialization
    bool saveToFile(const std::string& filepath);
    bool loadFromFile(const std::string& filepath);

private:
    AnimationManager() = default;
};

// Global accessor
AnimationManager& get_animation_manager();

#endif
