#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <glm/glm.hpp>

// Structure representing a bone node in the hierarchy
struct BoneNode {
    std::string name;
    int boneId;
    BoneNode* parent;
    std::vector<std::unique_ptr<BoneNode>> children;
    
    BoneNode(const std::string& boneName, int id) 
        : name(boneName), boneId(id), parent(nullptr) {}
    
    // Find a bone node by name in this subtree
    BoneNode* findBone(const std::string& boneName);
    
    // Get depth in hierarchy (root = 0)
    int getDepth() const;
    
    // Get full path from root (e.g., "Hips/Spine/Chest")
    std::string getPath() const;
};

// IK Chain definition
struct IKChainDefinition {
    std::string name;
    std::vector<int> boneIndices;  // Ordered from root to tip
    std::vector<std::string> boneNames;
    glm::vec3 targetPosition;      // Target position for IK solving
    
    IKChainDefinition() : targetPosition(0.0f) {}
    IKChainDefinition(const std::string& chainName) : name(chainName), targetPosition(0.0f) {}
};

// Bone hierarchy manager
class BoneHierarchy {
private:
    std::unique_ptr<BoneNode> rootNode;
    std::map<std::string, BoneNode*> boneMap;  // Quick lookup by name
    std::map<int, BoneNode*> boneIdMap;         // Quick lookup by ID
    
public:
    BoneHierarchy() = default;
    
    // Build hierarchy from Assimp node structure
    void buildFromAssimpNode(const struct aiNode* node, const std::map<std::string, struct BoneInfo>& boneInfoMap);
    
    // Get root node
    BoneNode* getRoot() const { return rootNode.get(); }
    
    // Find bone by name or ID
    BoneNode* findBone(const std::string& name) const;
    BoneNode* findBoneById(int id) const;
    
    // Get all bones as flat list
    void getAllBones(std::vector<BoneNode*>& outBones) const;
    
    // Check if hierarchy is valid
    bool isValid() const { return rootNode != nullptr; }
    
    // Print hierarchy to console (debug)
    void printHierarchy() const;
    
private:
    void buildNodeRecursive(const aiNode* node, BoneNode* parentBone, const std::map<std::string, BoneInfo>& boneInfoMap);
    void collectBones(BoneNode* node, std::vector<BoneNode*>& outBones) const;
    void printNodeRecursive(const BoneNode* node, int depth) const;
};

// IK Chain configuration manager
class IKChainManager {
private:
    std::vector<IKChainDefinition> chains;
    
public:
    // Add/remove chains
    void addChain(const IKChainDefinition& chain);
    void removeChain(size_t index);
    void clearChains();
    
    // Get chains
    const std::vector<IKChainDefinition>& getChains() const { return chains; }
    IKChainDefinition* getChain(size_t index);
    size_t getChainCount() const { return chains.size(); }
    
    // Save/load chain configurations
    bool saveToFile(const std::string& filepath) const;
    bool loadFromFile(const std::string& filepath);
    
    // Validate chain against bone hierarchy
    bool validateChain(const IKChainDefinition& chain, const BoneHierarchy& hierarchy) const;
};
