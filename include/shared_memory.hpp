#ifndef SHARED_MEMORY_HPP
#define SHARED_MEMORY_HPP

// Finger position data structure
struct FingerData {
    float cxHandRoot,cyHandRoot,czHandRoot;
    float cxIndex, cyIndex, czIndex;
    float cxMiddle, cyMiddle, czMiddle;
    float cxRing, cyRing, czRing;
    float cxPinky, cyPinky, czPinky;
    
    
    
    // Previous frame data for delta calculations
    float prevCxHandRoot,prevCyHandRoot,prevCzHandRoot;
    float prevCxIndex, prevCyIndex, prevCzIndex;
    float prevCxMiddle, prevCyMiddle, prevCzMiddle;
    float prevCxRing, prevCyRing, prevCzRing;
    float prevCxPinky, prevCyPinky, prevCzPinky;
    
    // Delta values
    float deltaXRoot,deltaYRoot,deltaZRoot;
    float deltaXIndex, deltaYIndex, deltaZIndex;
    float deltaXMiddle, deltaYMiddle, deltaZMiddle;
    float deltaXRing, deltaYRing, deltaZRing;
    float deltaXPinky, deltaYPinky, deltaZPinky;
    
    // State flags
    bool isMiddleized;
    bool firstTrue;
    bool trueInput;
    bool rootLock = true; // Added for root movement control
};

// Shared memory functions
bool setup_shared_memory();
void update_shared_memory();
void cleanup_shared_memory();
FingerData& get_finger_data();
bool is_shared_memory_available(); // New function to check availability

#endif
