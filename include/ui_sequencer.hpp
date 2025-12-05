#ifndef UI_SEQUENCER_HPP
#define UI_SEQUENCER_HPP

#include "ImSequencer.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "animation_manager.hpp"
#include <vector>

class IKRTSequencer : public ImSequencer::SequenceInterface {
public:
    IKRTSequencer() {}

    virtual int GetFrameMin() const override {
        return get_animation_manager().frameMin;
    }
    
    virtual int GetFrameMax() const override {
        return get_animation_manager().frameMax;
    }
    
    virtual int GetItemCount() const override {
        // Include both skeletal sequences and scene element sequences
        return (int)(get_animation_manager().sequences.size() + 
                     get_animation_manager().sceneElementSequences.size());
    }

    virtual void BeginEdit(int index) override {
        // Called when dragging starts
    }
    
    virtual void EndEdit() override {
        // Called when dragging ends
    }

    virtual int GetItemTypeCount() const override { return 0; }
    virtual const char* GetItemTypeName(int typeIndex) const override { return ""; }
    
    virtual const char* GetItemLabel(int index) const override {
        AnimationManager& animMgr = get_animation_manager();
        int skeletalCount = (int)animMgr.sequences.size();
        
        // First show skeletal sequences, then scene element sequences
        if (index < skeletalCount) {
            Sequence* seq = animMgr.getSequenceByIndex(index);
            if (seq) return seq->name.c_str();
        } else {
            int sceneIndex = index - skeletalCount;
            SceneElementSequence* seq = animMgr.getSceneElementSequenceByIndex(sceneIndex);
            if (seq) return seq->name.c_str();
        }
        return "Unknown";
    }

    virtual void Get(int index, int** start, int** end, int* type, unsigned int* color) override {
        AnimationManager& animMgr = get_animation_manager();
        int skeletalCount = (int)animMgr.sequences.size();
        
        static int s = 0;
        static int e = 0;
        
        // First show skeletal sequences, then scene element sequences
        if (index < skeletalCount) {
            Sequence* seq = animMgr.getSequenceByIndex(index);
            if (seq) {
                if (!seq->keyframes.empty()) {
                    s = seq->keyframes.front().frameIndex;
                    e = seq->keyframes.back().frameIndex;
                } else {
                    s = 0;
                    e = 10; // Default length
                }
                
                if (start) *start = &s;
                if (end) *end = &e;
                if (color) *color = 0xFFAA8080; // Red-ish color for skeletal
                if (type) *type = 0;
            }
        } else {
            int sceneIndex = index - skeletalCount;
            SceneElementSequence* seq = animMgr.getSceneElementSequenceByIndex(sceneIndex);
            if (seq) {
                if (!seq->keyframes.empty()) {
                    s = seq->keyframes.front().frameIndex;
                    e = seq->keyframes.back().frameIndex;
                } else {
                    s = 0;
                    e = 10; // Default length
                }
                
                if (start) *start = &s;
                if (end) *end = &e;
                if (color) *color = 0xFF80AAFF; // Blue-ish color for scene elements
                if (type) *type = 0;
            }
        }
    }

    virtual void CustomDrawCompact(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& clippingRect) override {
        AnimationManager& animMgr = get_animation_manager();
        int skeletalCount = (int)animMgr.sequences.size();
        
        int frameMin = GetFrameMin();
        int frameMax = GetFrameMax();
        
        // Determine if this is a skeletal or scene element sequence
        bool isSceneElement = (index >= skeletalCount);
        
        auto drawKeyframes = [&](const auto& keyframes) {
            draw_list->PushClipRect(clippingRect.Min, clippingRect.Max, true);
            for (const auto& kf : keyframes) {
                int p = kf.frameIndex;
                if (p < frameMin || p > frameMax) continue;

                float width = rc.Max.x - rc.Min.x;
                float range = (float)(frameMax - frameMin) + 2.0f;
                float w = width / range;
                float x = rc.Min.x + (float)(p - frameMin + 0.5f) * w;
                
                // Draw diamond
                float cy = rc.Min.y + (rc.Max.y - rc.Min.y) * 0.5f;
                float sz = 4.0f; // Size of diamond
                
                ImVec2 p1(x, cy - sz);
                ImVec2 p2(x + sz, cy);
                ImVec2 p3(x, cy + sz);
                ImVec2 p4(x - sz, cy);
                
                // Check selection
                bool isSelected = animMgr.isSelected(index, p);
                unsigned int color = isSelected ? 0xFF00FFFF : 0xFF0000FF; // Yellow if selected, Red otherwise
                
                draw_list->AddQuadFilled(p1, p2, p3, p4, color);
                
                // Interaction
                ImVec2 mousePos = ImGui::GetMousePos();
                // Simple bounding box check with some padding
                if (mousePos.x >= x - sz - 2 && mousePos.x <= x + sz + 2 &&
                    mousePos.y >= cy - sz - 2 && mousePos.y <= cy + sz + 2) {
                    
                    if (ImGui::IsMouseClicked(0)) { // Left click
                        if (ImGui::GetIO().KeyShift) {
                            // Add to selection
                            animMgr.addSelection(index, p, isSceneElement);
                        } else {
                            // Clear and select this one
                            animMgr.clearSelection();
                            animMgr.addSelection(index, p, isSceneElement);
                        }
                    }
                }
            }
            draw_list->PopClipRect();
        };

        // Handle Right Click to break sequence (clear selection)
        if (rc.Contains(ImGui::GetMousePos()) && ImGui::IsMouseClicked(1) && !ImGui::GetIO().KeyShift) {
            animMgr.clearSelection();
        }

        if (index < skeletalCount) {
            Sequence* seq = animMgr.getSequenceByIndex(index);
            if (seq) drawKeyframes(seq->keyframes);
        } else {
            int sceneIndex = index - skeletalCount;
            SceneElementSequence* seq = animMgr.getSceneElementSequenceByIndex(sceneIndex);
            if (seq) drawKeyframes(seq->keyframes);
        }
    }

    virtual void Add(int type) override {
        // Add new sequence
    }
    
    virtual void Del(int index) override {
        // Delete sequence
    }
    
    virtual void Duplicate(int index) override {
        // Duplicate sequence
    }

    virtual void Copy() override {}
    virtual void Paste() override {}

    virtual size_t GetCustomHeight(int index) override { return 0; }

    // We can use CustomDraw to render the individual keyframes as dots
    virtual void CustomDraw(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& legendRect, const ImRect& clippingRect, const ImRect& legendClippingRect) override {
        AnimationManager& animMgr = get_animation_manager();
        int skeletalCount = (int)animMgr.sequences.size();
        
        // Draw keyframes for both skeletal and scene element sequences
        if (index < skeletalCount) {
            Sequence* seq = animMgr.getSequenceByIndex(index);
            if (!seq) return;

            for (const auto& kf : seq->keyframes) {
                // For now, we will rely on the "Block" representation
                // Keyframe dots can be added later with proper frame-to-pixel conversion
            }
        } else {
            int sceneIndex = index - skeletalCount;
            SceneElementSequence* seq = animMgr.getSceneElementSequenceByIndex(sceneIndex);
            if (!seq) return;

            for (const auto& kf : seq->keyframes) {
                // For now, we will rely on the "Block" representation
                // Keyframe dots can be added later with proper frame-to-pixel conversion
            }
        }
    }
};

#endif
