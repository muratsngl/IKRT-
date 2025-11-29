#pragma once
#include <glm/glm.hpp>
#include <vector>
#define SAMPLES_PER_SECOND 30
//operations on keyframes

typedef enum interpolation{
    LINEAR,
    SLERP,
};


//1. EDIT: on a single keyframe change any bone you want with gizmo controls
//2. CREATE: create keyframe based on the current pose of the selected interactor model
//3. DESTROY: deleting keyframes freeing used memory

struct Keyframe{
    //sth to hold all the matrices
    std::vector<glm::mat4> pose_matrices;
    //sth to identify the animated model
    int id;
};



//operations on Sequences

//1. EDIT: with the sequencer_ui user can edit the keyframe sequences and even the keyframes themselves
//2. INTERPOLATION: user can select the needed interpolation types for the given sequence
//3. TIMESTEP: user can select the timestep that will be applied to the keyframes within
//4. CREATE: user can create a sequence based on the keyframes that have been created or user can create the keyframes on the fly. create->sequence->existing keyframe/new keyframe

struct Sequence{
    //an array that will hold the keyframes(predefined size)
    std::vector<Keyframe> keyframe;
    //duration in terms of keyframe size or in terms of real time ms
    int id;
};





