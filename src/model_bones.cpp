#include "model_bones.h"
#include "bone_hierarchy.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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

// InteractorModel constructor implementation
InteractorModel::InteractorModel(string const& path, bool gamma) : gammaCorrection(gamma), boneHierarchy(nullptr) {
    InteractorModelCreator creator;
    creator.loadModel(path, meshes, m_BoneInfoMap, bindPosePositions, bindPoseMatrices, textures_loaded);
    directory = creator.directory;
    
    // Build bone hierarchy from the loaded model
    if (!m_BoneInfoMap.empty()) {
        // Load the scene again to get aiNode structure
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, 
            aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        
        if (scene && scene->mRootNode) {
            boneHierarchy = new BoneHierarchy();
            boneHierarchy->buildFromAssimpNode(scene->mRootNode, m_BoneInfoMap);
            
            std::cout << "Built bone hierarchy for: " << path << std::endl;
            boneHierarchy->printHierarchy();
        } else {
            std::cerr << "Failed to rebuild scene for bone hierarchy extraction" << std::endl;
        }
    }
}

// InteractorModel destructor implementation
InteractorModel::~InteractorModel() {
    if (boneHierarchy) {
        delete boneHierarchy;
        boneHierarchy = nullptr;
    }
}