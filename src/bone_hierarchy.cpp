#include "include/bone_hierarchy.hpp"
#include "include/model_bones.h"
#include "json.hpp"
#include <assimp/scene.h>
#include <iostream>
#include <fstream>
#include <queue>

using json = nlohmann::json;

// BoneNode implementations
BoneNode* BoneNode::findBone(const std::string& boneName) {
    if (name == boneName) return this;
    
    for (auto& child : children) {
        BoneNode* found = child->findBone(boneName);
        if (found) return found;
    }
    return nullptr;
}

int BoneNode::getDepth() const {
    int depth = 0;
    const BoneNode* current = parent;
    while (current) {
        depth++;
        current = current->parent;
    }
    return depth;
}

std::string BoneNode::getPath() const {
    if (!parent) return name;
    return parent->getPath() + "/" + name;
}

// BoneHierarchy implementations
void BoneHierarchy::buildFromAssimpNode(const aiNode* node, const std::map<std::string, BoneInfo>& boneInfoMap) {
    if (!node) return;
    
    // Find the root bone (first node that has bone info or is ancestor of bones)
    std::string nodeName(node->mName.C_Str());
    
    // Check if this node is a bone
    auto it = boneInfoMap.find(nodeName);
    if (it != boneInfoMap.end()) {
        // This is the root bone
        rootNode = std::make_unique<BoneNode>(nodeName, it->second.id);
        boneMap[nodeName] = rootNode.get();
        boneIdMap[it->second.id] = rootNode.get();
        
        // Build children recursively
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            buildNodeRecursive(node->mChildren[i], rootNode.get(), boneInfoMap);
        }
    } else {
        // Not a bone, check children to find root bone
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            buildFromAssimpNode(node->mChildren[i], boneInfoMap);
            if (rootNode) break;  // Found root in child tree
        }
    }
}

void BoneHierarchy::buildNodeRecursive(const aiNode* node, BoneNode* parentBone, 
                                       const std::map<std::string, BoneInfo>& boneInfoMap) {
    if (!node) return;
    
    std::string nodeName(node->mName.C_Str());
    auto it = boneInfoMap.find(nodeName);
    
    if (it != boneInfoMap.end()) {
        // This node is a bone
        auto newBone = std::make_unique<BoneNode>(nodeName, it->second.id);
        newBone->parent = parentBone;
        
        BoneNode* newBonePtr = newBone.get();
        boneMap[nodeName] = newBonePtr;
        boneIdMap[it->second.id] = newBonePtr;
        
        parentBone->children.push_back(std::move(newBone));
        
        // Recursively build children
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            buildNodeRecursive(node->mChildren[i], newBonePtr, boneInfoMap);
        }
    } else {
        // Not a bone, but might have bone children - check all children
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            buildNodeRecursive(node->mChildren[i], parentBone, boneInfoMap);
        }
    }
}

BoneNode* BoneHierarchy::findBone(const std::string& name) const {
    auto it = boneMap.find(name);
    return (it != boneMap.end()) ? it->second : nullptr;
}

BoneNode* BoneHierarchy::findBoneById(int id) const {
    auto it = boneIdMap.find(id);
    return (it != boneIdMap.end()) ? it->second : nullptr;
}

void BoneHierarchy::getAllBones(std::vector<BoneNode*>& outBones) const {
    outBones.clear();
    if (rootNode) {
        collectBones(rootNode.get(), outBones);
    }
}

void BoneHierarchy::collectBones(BoneNode* node, std::vector<BoneNode*>& outBones) const {
    if (!node) return;
    outBones.push_back(node);
    for (auto& child : node->children) {
        collectBones(child.get(), outBones);
    }
}

void BoneHierarchy::printHierarchy() const {
    if (!rootNode) {
        std::cout << "No bone hierarchy available" << std::endl;
        return;
    }
    std::cout << "=== Bone Hierarchy ===" << std::endl;
    printNodeRecursive(rootNode.get(), 0);
}

void BoneHierarchy::printNodeRecursive(const BoneNode* node, int depth) const {
    if (!node) return;
    
    std::string indent(depth * 2, ' ');
    std::cout << indent << "- " << node->name << " (ID: " << node->boneId << ")" << std::endl;
    
    for (const auto& child : node->children) {
        printNodeRecursive(child.get(), depth + 1);
    }
}

// IKChainManager implementations
void IKChainManager::addChain(const IKChainDefinition& chain) {
    chains.push_back(chain);
}

void IKChainManager::removeChain(size_t index) {
    if (index < chains.size()) {
        chains.erase(chains.begin() + index);
    }
}

void IKChainManager::clearChains() {
    chains.clear();
}

IKChainDefinition* IKChainManager::getChain(size_t index) {
    if (index < chains.size()) {
        return &chains[index];
    }
    return nullptr;
}

bool IKChainManager::saveToFile(const std::string& filepath) const {
    try {
        json j;
        j["version"] = "1.0";
        j["chains"] = json::array();
        
        for (const auto& chain : chains) {
            json chainData;
            chainData["name"] = chain.name;
            chainData["bone_indices"] = chain.boneIndices;
            chainData["bone_names"] = chain.boneNames;
            j["chains"].push_back(chainData);
        }
        
        std::ofstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << filepath << std::endl;
            return false;
        }
        
        file << j.dump(4);
        file.close();
        
        std::cout << "IK chain configuration saved to: " << filepath << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error saving IK chain config: " << e.what() << std::endl;
        return false;
    }
}

bool IKChainManager::loadFromFile(const std::string& filepath) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for reading: " << filepath << std::endl;
            return false;
        }
        
        json j;
        file >> j;
        file.close();
        
        chains.clear();
        
        if (j.contains("chains")) {
            for (const auto& chainData : j["chains"]) {
                IKChainDefinition chain;
                chain.name = chainData["name"];
                chain.boneIndices = chainData["bone_indices"].get<std::vector<int>>();
                chain.boneNames = chainData["bone_names"].get<std::vector<std::string>>();
                chains.push_back(chain);
            }
        }
        
        std::cout << "IK chain configuration loaded from: " << filepath << std::endl;
        std::cout << "Loaded " << chains.size() << " chain(s)" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading IK chain config: " << e.what() << std::endl;
        return false;
    }
}

bool IKChainManager::validateChain(const IKChainDefinition& chain, const BoneHierarchy& hierarchy) const {
    if (chain.boneIndices.empty()) return false;
    
    // Check all bones exist
    for (int boneId : chain.boneIndices) {
        if (!hierarchy.findBoneById(boneId)) {
            std::cerr << "Chain validation failed: Bone ID " << boneId << " not found" << std::endl;
            return false;
        }
    }
    
    return true;
}
