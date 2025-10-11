#include "model_bones.h"

// Definition of global scene element boxes
std::vector<Shape> scene_element_boxes;
std::vector<Shape> interactable_element_boxes;
int interactable_element_count = 0;
int scene_element_count = 0;

// Global unique model ID counter for all model types
int global_model_id_counter = 0;

// Definition of global bone info map and counter for InteractableModel consistency
std::map<string, BoneInfo> global_bone_info_map;
int global_bone_counter = 0;