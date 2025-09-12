#include "include/shared_memory.hpp"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <cmath>

static int shm_fd = -1;
static void* pBuf = nullptr;
static FingerData finger_data = {};
static bool shared_memory_available = false;

bool setup_shared_memory() {
    const std::string shm_name = "handPositionData";
    shm_fd = shm_open(shm_name.c_str(), O_RDONLY, 0666);
    if (shm_fd == -1) {
        // perror("shm_open");
        // std::cerr << "Could not open shared memory" << std::endl;
        shared_memory_available = false;
        return false;
    }

    pBuf = mmap(nullptr, 62, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (pBuf == MAP_FAILED) {
        // std::cerr << "Could not map view of file" << std::endl;
        close(shm_fd);
        shared_memory_available = false;
        return false;
    }
    
    // Initialize finger data
    finger_data.isMiddleized = false;
    finger_data.firstTrue = false;
    finger_data.trueInput = false;
    shared_memory_available = true;
    
    return true;
}

void update_shared_memory() {
    if (!shared_memory_available || pBuf == nullptr) {
        // Set all delta values to 0 when shared memory is not available
        finger_data.deltaXRoot = finger_data.deltaYRoot = finger_data.deltaZRoot = 0;
        finger_data.deltaXIndex = finger_data.deltaYIndex = finger_data.deltaZIndex = 0;
        finger_data.deltaXMiddle = finger_data.deltaYMiddle = finger_data.deltaZMiddle = 0;
        finger_data.deltaXRing = finger_data.deltaYRing = finger_data.deltaZRing = 0;
        finger_data.deltaXPinky = finger_data.deltaYPinky = finger_data.deltaZPinky = 0;
        return;
    }
    
    // Store previous values
    finger_data.prevCxIndex = finger_data.cxIndex;
    finger_data.prevCyIndex = finger_data.cyIndex;
    finger_data.prevCzIndex = finger_data.czIndex;
    
    finger_data.prevCxMiddle = finger_data.cxMiddle;
    finger_data.prevCyMiddle = finger_data.cyMiddle;
    finger_data.prevCzMiddle = finger_data.czMiddle;
    
    finger_data.prevCxRing = finger_data.cxRing;
    finger_data.prevCyRing = finger_data.cyRing;
    finger_data.prevCzRing = finger_data.czRing;
    
    finger_data.prevCxPinky = finger_data.cxPinky;
    finger_data.prevCyPinky = finger_data.cyPinky;
    finger_data.prevCzPinky = finger_data.czPinky;
    
    finger_data.prevCxHandRoot = finger_data.cxHandRoot;
    finger_data.prevCyHandRoot = finger_data.cyHandRoot;
    finger_data.prevCzHandRoot = finger_data.czHandRoot;

    
    
    // Read new values from shared memory
    
    
    
    memcpy(&finger_data.cxHandRoot,(char*)pBuf,sizeof(float));
    memcpy(&finger_data.cyHandRoot,(char*)pBuf+4,sizeof(float));
    memcpy(&finger_data.czHandRoot, (char*)pBuf + 41, sizeof(float));
    
    
    memcpy(&finger_data.cxIndex, (char*)pBuf + 8, sizeof(float));
    memcpy(&finger_data.cyIndex, (char*)pBuf + 12, sizeof(float));
    memcpy(&finger_data.czIndex, (char*)pBuf + 45, sizeof(float));

    memcpy(&finger_data.cxMiddle, (char*)pBuf + 16, sizeof(float));
    memcpy(&finger_data.cyMiddle, (char*)pBuf + 20, sizeof(float));
    memcpy(&finger_data.czMiddle, (char*)pBuf + 49, sizeof(float));

    memcpy(&finger_data.cxRing, (char*)pBuf + 24, sizeof(float));
    memcpy(&finger_data.cyRing, (char*)pBuf + 28, sizeof(float));
    memcpy(&finger_data.czRing, (char*)pBuf + 43, sizeof(float));

    memcpy(&finger_data.cxPinky, (char*)pBuf + 32, sizeof(float));
    memcpy(&finger_data.cyPinky, (char*)pBuf + 36, sizeof(float));
    memcpy(&finger_data.czPinky, (char*)pBuf + 57, sizeof(float));

    memcpy(&finger_data.trueInput, (char*)pBuf + 40, sizeof(bool));
    
    // Calculate calibration state
    if (!finger_data.firstTrue) {
        finger_data.isMiddleized = fabs(finger_data.cxMiddle - 0.5f) < 0.01f && 
                                  fabs(finger_data.cyMiddle - 0.5f) < 0.01f;
    }
    
    if (finger_data.isMiddleized) 
    {
        finger_data.deltaXRoot = 2 * (finger_data.cxHandRoot - finger_data.prevCxHandRoot);
        finger_data.deltaYRoot = -2 * (finger_data.cyHandRoot - finger_data.prevCyHandRoot);
        finger_data.deltaZRoot = 2 * (finger_data.czHandRoot - finger_data.prevCzHandRoot);

        
        finger_data.deltaXIndex = 2 * (finger_data.cxIndex - finger_data.prevCxIndex);
        finger_data.deltaYIndex = -2 * (finger_data.cyIndex - finger_data.prevCyIndex);
        finger_data.deltaZIndex = 2 * (finger_data.czIndex - finger_data.prevCzIndex);

        finger_data.deltaXRing = 2 * (finger_data.cxRing - finger_data.prevCxRing);
        finger_data.deltaYRing = -2 * (finger_data.cyRing - finger_data.prevCyRing);
        finger_data.deltaZRing = 2 * (finger_data.czRing - finger_data.prevCzRing);

        finger_data.deltaXMiddle = 2 * (finger_data.cxMiddle - finger_data.prevCxMiddle);
        finger_data.deltaYMiddle = -2 * (finger_data.cyMiddle - finger_data.prevCyMiddle);
        finger_data.deltaZMiddle = 2 * (finger_data.czMiddle - finger_data.prevCzMiddle);

        finger_data.deltaXPinky = 2 * (finger_data.cxPinky - finger_data.prevCxPinky);
        finger_data.deltaYPinky = -2 * (finger_data.cyPinky - finger_data.prevCyPinky);
        finger_data.deltaZPinky = 2 * (finger_data.czPinky - finger_data.prevCzPinky);

        finger_data.firstTrue = true;
        
    }

    // Apply threshold filtering
    const float threshold = 0.016f;
    
    finger_data.deltaXIndex = fabs(finger_data.deltaXIndex) < threshold || !finger_data.trueInput ? 0 : finger_data.deltaXIndex;
    finger_data.deltaYIndex = fabs(finger_data.deltaYIndex) < threshold || !finger_data.trueInput ? 0 : finger_data.deltaYIndex;
    finger_data.deltaZIndex = fabs(finger_data.deltaZIndex) < threshold || !finger_data.trueInput ? 0 : finger_data.deltaZIndex;

    finger_data.deltaXRing = fabs(finger_data.deltaXRing) < threshold || !finger_data.trueInput ? 0 : finger_data.deltaXRing;
    finger_data.deltaYRing = fabs(finger_data.deltaYRing) < threshold || !finger_data.trueInput ? 0 : finger_data.deltaYRing;
    finger_data.deltaZRing = fabs(finger_data.deltaZRing) < threshold || !finger_data.trueInput ? 0 : finger_data.deltaZRing;

    finger_data.deltaXMiddle = fabs(finger_data.deltaXMiddle) < threshold || !finger_data.trueInput ? 0 : finger_data.deltaXMiddle;
    finger_data.deltaYMiddle = fabs(finger_data.deltaYMiddle) < threshold || !finger_data.trueInput ? 0 : finger_data.deltaYMiddle;
    finger_data.deltaZMiddle = fabs(finger_data.deltaZMiddle) < threshold || !finger_data.trueInput ? 0 : finger_data.deltaZMiddle;

    finger_data.deltaXPinky = fabs(finger_data.deltaXPinky) < threshold || !finger_data.trueInput ? 0 : finger_data.deltaXPinky;
    finger_data.deltaYPinky = fabs(finger_data.deltaYPinky) < threshold || !finger_data.trueInput ? 0 : finger_data.deltaYPinky;
    finger_data.deltaZPinky = fabs(finger_data.deltaZPinky) < threshold || !finger_data.trueInput ? 0 : finger_data.deltaZPinky;

    finger_data.deltaXRoot = fabs(finger_data.deltaXRoot) < threshold || !finger_data.trueInput ? 0 : finger_data.deltaXRoot;
    finger_data.deltaYRoot = fabs(finger_data.deltaYRoot) < threshold || !finger_data.trueInput ? 0 : finger_data.deltaYRoot;
    finger_data.deltaZRoot = fabs(finger_data.deltaZRoot) < threshold || !finger_data.trueInput ? 0 : finger_data.deltaZRoot;
}

void cleanup_shared_memory() {
    if (pBuf != nullptr) {
        munmap(pBuf, 62);
        pBuf = nullptr;
    }
    if (shm_fd != -1) {
        close(shm_fd);
        shm_fd = -1;
    }
    shared_memory_available = false;
}

FingerData& get_finger_data() {
    return finger_data;
}

bool is_shared_memory_available() {
    return shared_memory_available;
}
