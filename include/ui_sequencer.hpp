#ifndef UI_SEQUENCER_HPP
#define UI_SEQUENCER_HPP

#include "ImSequencer.h"
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
        return (int)get_animation_manager().sequences.size();
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
        Sequence* seq = get_animation_manager().getSequenceByIndex(index);
        if (seq) return seq->name.c_str();
        return "Unknown";
    }

    virtual void Get(int index, int** start, int** end, int* type, unsigned int* color) override {
        Sequence* seq = get_animation_manager().getSequenceByIndex(index);
        if (seq) {
            // For now, let's say the sequence spans the whole timeline or min/max keyframes
            // ImSequencer expects a start/end for the "block" representation
            // We can calculate this from the first and last keyframe
            static int s = 0;
            static int e = 0;
            
            if (!seq->keyframes.empty()) {
                s = seq->keyframes.front().frameIndex;
                e = seq->keyframes.back().frameIndex;
            } else {
                s = 0;
                e = 10; // Default length
            }
            
            if (start) *start = &s;
            if (end) *end = &e;
            if (color) *color = 0xFFAA8080; // A nice color
            if (type) *type = 0;
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
        Sequence* seq = get_animation_manager().getSequenceByIndex(index);
        if (!seq) return;

        for (const auto& kf : seq->keyframes) {
            // Calculate pixel position of the keyframe
            // We need access to the sequencer's internal state or pass it in, 
            // but ImSequencer doesn't expose the frame-to-pixel conversion easily in CustomDraw without calculation.
            // However, we can approximate or use the rc bounds.
            
            // Actually, ImSequencer draws the "block" defined by Get().
            // If we want to draw dots *inside* that block or on the track, we need to know the frame width.
            // This is a bit complex without modifying ImSequencer or calculating the ratio manually.
            
            // For now, we will rely on the "Block" representation.
        }
    }
};

#endif
