#ifndef ANIMATION_MANAGER_HPP
#define ANIMATION_MANAGER_HPP

#include "anim_data.hpp"
#include <vector>
#include <iostream>
#include <algorithm>

class AnimationManager {
public:
    static AnimationManager& getInstance() {
        static AnimationManager instance;
        return instance;
    }

    // Global timeline state
    int currentFrame = 0;
    int frameMin = 0;
    int frameMax = 6000;
    bool isPlaying = false;
    float fps = 24.0f;
    float timeAccumulator = 0.0f;

    // Container for all sequences
    std::vector<Sequence> sequences;
    std::vector<SceneElementSequence> sceneElementSequences;
    int activeSequenceIndex = -1;
    int activeSceneElementSequenceIndex = -1;

    // Management
    void addSequence(const Sequence& seq) {
        sequences.push_back(seq);
        if (activeSequenceIndex == -1) {
            activeSequenceIndex = 0;
        }
    }

    void addSceneElementSequence(const SceneElementSequence& seq) {
        sceneElementSequences.push_back(seq);
        if (activeSceneElementSequenceIndex == -1) {
            activeSceneElementSequenceIndex = 0;
        }
    }

    void createNewSequence(int modelID, const std::string& name) {
        sequences.emplace_back(name, modelID);
        activeSequenceIndex = sequences.size() - 1;
    }

    void createNewSceneElementSequence(int modelID, const std::string& name) {
        sceneElementSequences.emplace_back(name, modelID);
        activeSceneElementSequenceIndex = sceneElementSequences.size() - 1;
    }

    Sequence* getActiveSequence() {
        if (activeSequenceIndex >= 0 && activeSequenceIndex < sequences.size()) {
            return &sequences[activeSequenceIndex];
        }
        return nullptr;
    }

    SceneElementSequence* getActiveSceneElementSequence() {
        if (activeSceneElementSequenceIndex >= 0 && activeSceneElementSequenceIndex < sceneElementSequences.size()) {
            return &sceneElementSequences[activeSceneElementSequenceIndex];
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

    SceneElementSequence* getSceneElementSequenceForModel(int modelID) {
        // Prefer active sequence if it matches the model
        if (activeSceneElementSequenceIndex >= 0 && activeSceneElementSequenceIndex < sceneElementSequences.size()) {
            if (sceneElementSequences[activeSceneElementSequenceIndex].modelID == modelID) {
                return &sceneElementSequences[activeSceneElementSequenceIndex];
            }
        }

        // Fallback to first matching
        for (auto& seq : sceneElementSequences) {
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

    SceneElementSequence* getSceneElementSequenceByIndex(int index) {
        if (index >= 0 && index < sceneElementSequences.size()) {
            return &sceneElementSequences[index];
        }
        return nullptr;
    }

    // Core Logic
    void update(float deltaTime);
    void applyFrame(int frame);
    void recordKeyframe(int modelID); // Captures current state of modelID into currentFrame
    void recordSceneElementKeyframe(int modelID); // Captures current state of scene element modelID into currentFrame

    // Serialization
    bool saveToFile(const std::string& filepath);
    bool loadFromFile(const std::string& filepath);
    
    // Keyframe management
    bool removeKeyframeAtFrame(int sequenceIndex, int frame, bool isSceneElement = false);
    
    // Sequence management
    bool removeSequence(int sequenceIndex, bool isSceneElement = false);
    
    // Copy/Paste support
    struct SequenceClipboard {
        bool isValid = false;
        bool isSceneElement = false;
        std::string name;
        int modelID;
        std::vector<Keyframe> skeletalKeyframes;
        std::vector<SceneElementKeyframe> sceneElementKeyframes;
    };
    SequenceClipboard clipboard;
    
    void copySequence(int sequenceIndex, bool isSceneElement = false);
    void pasteSequence(int atFrame);
    bool hasClipboardData() const { return clipboard.isValid; }

    // Keyframe Selection & Clipboard
    struct SelectedKeyframe {
        int sequenceIndex; // Index in the combined list (skeletal first, then scene)
        int frameIndex;
        bool isSceneElement;
    };
    std::vector<SelectedKeyframe> selectedKeyframes;

    void clearSelection() {
        selectedKeyframes.clear();
    }

    void addSelection(int sequenceIndex, int frameIndex, bool isSceneElement) {
        for (const auto& sel : selectedKeyframes) {
            if (sel.sequenceIndex == sequenceIndex && sel.frameIndex == frameIndex) return;
        }
        selectedKeyframes.push_back({sequenceIndex, frameIndex, isSceneElement});
    }
    
    bool isSelected(int sequenceIndex, int frameIndex) {
        for (const auto& sel : selectedKeyframes) {
            if (sel.sequenceIndex == sequenceIndex && sel.frameIndex == frameIndex) return true;
        }
        return false;
    }

    struct CopiedKeyframe {
        int relativeFrame;
        // We need to store data. Since Keyframe doesn't have default ctor, we handle it carefully.
        // We'll store copies.
        std::vector<Keyframe> skeletalData; // Vector of size 0 or 1
        std::vector<SceneElementKeyframe> sceneData; // Vector of size 0 or 1
    };
    
    struct KeyframeClipboard {
        bool hasData = false;
        bool isSceneElement = false; 
        std::vector<CopiedKeyframe> keyframes;
    };
    KeyframeClipboard keyframeClipboard;

    void copySelectedKeyframes() {
        if (selectedKeyframes.empty()) return;
        
        keyframeClipboard.keyframes.clear();
        keyframeClipboard.hasData = true;
        
        // Sort selection by frame index to maintain order
        std::sort(selectedKeyframes.begin(), selectedKeyframes.end(), 
            [](const SelectedKeyframe& a, const SelectedKeyframe& b) {
                return a.frameIndex < b.frameIndex;
            });

        int firstFrame = selectedKeyframes[0].frameIndex;
        
        // Determine type from the first selection (assuming homogeneous copy for now, or mixed?)
        // User said "copy several keyframes". If I select from different sequences, what happens?
        // "insert them inside" (singular active sequence).
        // So we probably only support copying from ONE type of sequence at a time, or we filter.
        // Let's assume we take the type of the first selected keyframe.
        keyframeClipboard.isSceneElement = selectedKeyframes[0].isSceneElement;

        for (const auto& sel : selectedKeyframes) {
            // Only copy compatible types
            if (sel.isSceneElement != keyframeClipboard.isSceneElement) continue;

            CopiedKeyframe ck;
            ck.relativeFrame = sel.frameIndex - firstFrame;

            if (!sel.isSceneElement) {
                // Skeletal
                int skeletalCount = (int)sequences.size();
                if (sel.sequenceIndex < skeletalCount) {
                    Sequence* seq = &sequences[sel.sequenceIndex];
                    Keyframe* kf = seq->getKeyframe(sel.frameIndex);
                    if (kf) ck.skeletalData.push_back(*kf);
                }
            } else {
                // Scene Element
                int skeletalCount = (int)sequences.size();
                int sceneIndex = sel.sequenceIndex - skeletalCount;
                if (sceneIndex >= 0 && sceneIndex < sceneElementSequences.size()) {
                    SceneElementSequence* seq = &sceneElementSequences[sceneIndex];
                    SceneElementKeyframe* kf = seq->getKeyframe(sel.frameIndex);
                    if (kf) ck.sceneData.push_back(*kf);
                }
            }
            
            if (!ck.skeletalData.empty() || !ck.sceneData.empty()) {
                keyframeClipboard.keyframes.push_back(ck);
            }
        }
    }

    void pasteKeyframes(int targetSequenceIndex, bool isSceneElement, int atFrame) {
        if (!keyframeClipboard.hasData) return;
        if (keyframeClipboard.isSceneElement != isSceneElement) {
            std::cout << "Cannot paste incompatible keyframes (Skeletal vs Scene Element)" << std::endl;
            return;
        }

        if (!isSceneElement) {
            if (targetSequenceIndex >= 0 && targetSequenceIndex < sequences.size()) {
                Sequence* seq = &sequences[targetSequenceIndex];
                for (const auto& ck : keyframeClipboard.keyframes) {
                    if (!ck.skeletalData.empty()) {
                        Keyframe newKf = ck.skeletalData[0];
                        newKf.frameIndex = atFrame + ck.relativeFrame;
                        seq->addKeyframe(newKf);
                    }
                }
            }
        } else {
            if (targetSequenceIndex >= 0 && targetSequenceIndex < sceneElementSequences.size()) {
                SceneElementSequence* seq = &sceneElementSequences[targetSequenceIndex];
                for (const auto& ck : keyframeClipboard.keyframes) {
                    if (!ck.sceneData.empty()) {
                        SceneElementKeyframe newKf = ck.sceneData[0];
                        newKf.frameIndex = atFrame + ck.relativeFrame;
                        seq->addKeyframe(newKf);
                    }
                }
            }
        }
    }

private:
    AnimationManager() = default;
};

// Global accessor
AnimationManager& get_animation_manager();

#endif
