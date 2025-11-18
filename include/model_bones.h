#ifndef MODEL_H
#define MODEL_H

#include <GL/glew.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/MathFunctions.h>

#include "Mesh.h"

// Forward declarations
class BoneHierarchy;

#include "Shader.h"
#include "Texture.h"
#include "Interaction.hpp"
#include "collision.hpp"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <float.h>
#include <cfloat>
using namespace std;

// Global declaration of scene element boxes
extern std::vector<Shape> scene_element_boxes;
extern std::vector<Shape> interactable_element_boxes;
extern int interactable_element_count;
extern int scene_element_count;

// Global unique model ID counter
extern int global_model_id_counter;

// Function to get next unique model ID
inline int get_next_unique_model_id() {
    return global_model_id_counter++;
}

// Function to get current model ID counter value (next ID that will be assigned)
inline int get_current_model_id_counter() {
    return global_model_id_counter;
}

struct BoneInfo
{
    /*id is index in finalBoneMatrices*/
    int id;

    /*offset matrix transforms vertex from model space to bone space*/
    glm::mat4 offset;

};

// Global bone info map to ensure consistent bone indices across InteractableModel instances
extern std::map<string, BoneInfo> global_bone_info_map;
extern int global_bone_counter;
inline glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4* from)
{
    glm::mat4 to;


    to[0][0] = (GLfloat)from->a1; to[0][1] = (GLfloat)from->b1;  to[0][2] = (GLfloat)from->c1; to[0][3] = (GLfloat)from->d1;
    to[1][0] = (GLfloat)from->a2; to[1][1] = (GLfloat)from->b2;  to[1][2] = (GLfloat)from->c2; to[1][3] = (GLfloat)from->d2;
    to[2][0] = (GLfloat)from->a3; to[2][1] = (GLfloat)from->b3;  to[2][2] = (GLfloat)from->c3; to[2][3] = (GLfloat)from->d3;
    to[3][0] = (GLfloat)from->a4; to[3][1] = (GLfloat)from->b4;  to[3][2] = (GLfloat)from->c4; to[3][3] = (GLfloat)from->d4;

    return to;
}


class InteractableModelCreator {
private:
    std::vector<TextureInfo> textures_loaded;
    std::map<string, BoneInfo> local_bone_info_map;  // Local bone info for this model
    int local_bone_counter = 0;  // Local bone counter for this model

    void SetVertexBoneDataToDefault(DynamicVertex& vertex)
    {
        for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
        {
            vertex.m_BoneIDs[i] = -1;
            vertex.m_Weights[i] = 0.0f;
        }
    }

    void SetVertexBoneData(DynamicVertex& vertex, int boneID, float weight)
    {
        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
        {
            if (vertex.m_BoneIDs[i] < 0)
            {
                vertex.m_Weights[i] = weight;
                vertex.m_BoneIDs[i] = boneID;
                break;
            }
        }
    }

    void ExtractBoneWeightForVertices(std::vector<DynamicVertex>& vertices, aiMesh* mesh, const aiScene* scene, vector<glm::vec3>& bindPosePositions, vector<glm::mat4>& bindPoseMatrices)
    {
        for (int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
        {
            int boneID = -1;
            std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
            if (local_bone_info_map.find(boneName) == local_bone_info_map.end())
            {
                BoneInfo newBoneInfo;
                newBoneInfo.id = local_bone_counter;
                std::cout << "Interactable Model Bone: " << newBoneInfo.id << " " << boneName << std::endl;
                newBoneInfo.offset = aiMatrix4x4ToGlm(
                    &mesh->mBones[boneIndex]->mOffsetMatrix);
                glm::mat4 currentTransform = glm::inverse(newBoneInfo.offset);
                bindPoseMatrices.push_back(glm::mat4(1.0f));
                bindPosePositions.push_back(currentTransform * glm::vec4(0.f, 0.f, 0.f, 1.0f));
                
                local_bone_info_map[boneName] = newBoneInfo;
                boneID = local_bone_counter;
                local_bone_counter++;
                
            }
            else
            {
                boneID = local_bone_info_map[boneName].id;
            }
            assert(boneID != -1);
            auto weights = mesh->mBones[boneIndex]->mWeights;
            int numWeights = mesh->mBones[boneIndex]->mNumWeights;

            for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
            {
                int vertexId = weights[weightIndex].mVertexId;
                float weight = weights[weightIndex].mWeight;
                assert(vertexId <= vertices.size());
                SetVertexBoneData(vertices[vertexId], boneID, weight);
            }
        }
    }

    std::vector<TextureInfo> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName) {
        std::vector<TextureInfo> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);
            // Prevent duplicate loading of textures
            bool skip = false;
            for (unsigned int j = 0; j < textures_loaded.size(); j++) {
                if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }
            if (!skip) {
                // If texture hasn't been loaded already, load it
                TextureInfo texture;
                texture.id = LoadTexture(str.C_Str(), directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture); // Add to loaded textures
            }
        }
        return textures;
    }

        void processNode(aiNode* node, const aiScene* scene, vector<Mesh>& meshes, string& directory, vector<glm::vec3>& bindPosePos, vector<glm::mat4>& bindPoseMat, glm::vec3& minBounds, glm::vec3& maxBounds)
    {
        // process each mesh located at the current node
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene, directory, bindPosePos, bindPoseMat, minBounds, maxBounds));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene, meshes, directory, bindPosePos, bindPoseMat, minBounds, maxBounds);
        }
    }
    
    // Texture loading methods (copied from SceneElementModelCreator)
    unsigned int LoadTexture(char const* path, const std::string& directory) {
        std::string filename = std::string(path);
        filename = directory + '/' + filename;

        unsigned int textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
        if (data) {
            GLenum format;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
        } else {
            std::cout << "Texture failed to load at path: " << path << std::endl;
            stbi_image_free(data);
        }

        return textureID;
    }

    unsigned int LoadTextureFromData(const aiTexture* texture) {
        unsigned int textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        unsigned char* data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(texture->pcData), texture->mWidth, &width, &height, &nrComponents, 0);
        if (data) {
            GLenum format;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
        } else {
            std::cout << "Failed to load embedded texture" << std::endl;
        }

        return textureID;
    }

    std::vector<TextureInfo> loadMaterialTextures(aiMaterial* mat, const aiScene* scene, aiTextureType type, std::string typeName) {
        std::vector<TextureInfo> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);
            
            bool skip = false;
            for (unsigned int j = 0; j < textures_loaded.size(); j++) {
                if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }
            if (!skip) {
                TextureInfo texture;
                if (str.C_Str()[0] == '*') {
                    int textureIndex = std::stoi(std::string(str.C_Str()).substr(1));
                    if (textureIndex < scene->mNumTextures) {
                        texture.id = LoadTextureFromData(scene->mTextures[textureIndex]);
                    } else {
                        std::cout << "Invalid embedded texture index: " << textureIndex << std::endl;
                        continue;
                    }
                } else {
                    texture.id = LoadTexture(str.C_Str(), directory);
                }
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
        }
        return textures;
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene, string& directory, vector<glm::vec3>& bindPosePos, vector<glm::mat4>& bindPoseMat, glm::vec3& minBounds, glm::vec3& maxBounds)
    {
        // data to fill
        vector<DynamicVertex> vertices;
        vector<unsigned int> indices;
        //will be used for inverse kinematics base bone locations
        // walk through each of the mesh's vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            DynamicVertex vertex;
            SetVertexBoneDataToDefault(vertex);
            glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
            // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;

            // Update bounds as we process vertices
            minBounds.x = std::min(minBounds.x, vector.x);
            minBounds.y = std::min(minBounds.y, vector.y);
            minBounds.z = std::min(minBounds.z, vector.z);
            
            maxBounds.x = std::max(maxBounds.x, vector.x);
            maxBounds.y = std::max(maxBounds.y, vector.y);
            maxBounds.z = std::max(maxBounds.z, vector.z);
            // normals
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            // texture coordinates
            if (mesh->mTextureCoords[0]) {
                vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
                if (mesh->HasTangentsAndBitangents()) {
                    vertex.Tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
                    vertex.Bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
                } else {
                    vertex.Tangent = glm::vec3(0.0f);
                    vertex.Bitangent = glm::vec3(0.0f);
                }
            } else {
                vertex.TexCoords = glm::vec2(0.0f);
                vertex.Tangent = glm::vec3(0.0f);
                vertex.Bitangent = glm::vec3(0.0f);
            }

            vertices.push_back(vertex);
        }
        // now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        ExtractBoneWeightForVertices(vertices, mesh, scene, bindPosePos, bindPoseMat);
        
        // Load PBR material textures, similar to SceneElementModelCreator
        vector<TextureInfo> textures;
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        
        vector<TextureInfo> albedoMaps = loadMaterialTextures(material, scene, aiTextureType_DIFFUSE, "texture_albedo");
        textures.insert(textures.end(), albedoMaps.begin(), albedoMaps.end());
        
        vector<TextureInfo> metallicMaps = loadMaterialTextures(material, scene, aiTextureType_METALNESS, "texture_metallic");
        textures.insert(textures.end(), metallicMaps.begin(), metallicMaps.end());

        vector<TextureInfo> roughnessMaps = loadMaterialTextures(material, scene, aiTextureType_DIFFUSE_ROUGHNESS, "texture_roughness");
        textures.insert(textures.end(), roughnessMaps.begin(), roughnessMaps.end());
        
        vector<TextureInfo> normalMaps = loadMaterialTextures(material, scene, aiTextureType_NORMALS, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        
        vector<TextureInfo> aoMaps = loadMaterialTextures(material, scene, aiTextureType_AMBIENT_OCCLUSION, "texture_ao");
        textures.insert(textures.end(), aoMaps.begin(), aoMaps.end());
        
        // return a mesh object created from the extracted mesh data
        return Mesh(vertices, indices, textures);
    }

public:
    string directory;

    // Getter for local bone info map
    std::map<string, BoneInfo>& getLocalBoneInfoMap() { return local_bone_info_map; }
    int getLocalBoneCount() { return local_bone_counter; }

    void loadModel(string const& path, vector<Mesh>& meshes, vector<glm::vec3>& bindPosePos, vector<glm::mat4>& bindPoseMat, std::vector<TextureInfo>& texLoaded, std::map<string, BoneInfo>& boneInfoMap, int modelId)
    {
        // read file via ASSIMP
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        // check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));

        // Initialize bounds tracking
        glm::vec3 minBounds(FLT_MAX);
        glm::vec3 maxBounds(-FLT_MAX);

        // process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene, meshes, directory, bindPosePos, bindPoseMat, minBounds, maxBounds);

        // Create AABB from calculated bounds
        if (minBounds.x != FLT_MAX) { // Check if we found any vertices
            Shape s;
            s.type = AABB;
            s.id = modelId;
            
            // Create AABB directly from bounds
            s.aabb.min = minBounds;
            s.aabb.max = maxBounds;
            
            interactable_element_boxes.push_back(s);
        }

        // Copy data back
        texLoaded = textures_loaded;
        boneInfoMap = local_bone_info_map;  // Copy local bone map to model
    }
};

class InteractableModel {
public:
    // model data 
    
    vector<Mesh> meshes;
    string directory;
    bool gammaCorrection;
    vector<glm::vec3> bindPosePositions;
    vector<glm::mat4> bindPoseMatrices;
    std::vector<TextureInfo> textures_loaded; // Store loaded textures to prevent duplicates
    std::vector<Interaction> model_interactions;
    std::map<string, BoneInfo> local_bone_info_map;  // Local bone info for this model
    int id;
    unsigned int model_index;  // UBO model matrix index
    std::string original_file_path;  // Store original file path for serialization
    
    // constructor, expects a filepath to a 3D model.
    InteractableModel(string const& path, bool gamma = false) : gammaCorrection(gamma), original_file_path(path)
    {
        InteractableModelCreator creator;
        id = get_next_unique_model_id();  // Use utility function for unique ID
        interactable_element_count++;     // Keep count for other purposes
        creator.loadModel(path, meshes, bindPosePositions, bindPoseMatrices, textures_loaded, local_bone_info_map, id);
        directory = creator.directory;
        model_index = 0; // Will be set during loading
        // Bounding box is now created during model loading
    }

    // draws the model, and thus all its meshes
    void Draw(Shader& shader) const
    {
        // Get the model index using the ID from the hashmap
        unsigned int index = model_index;
        shader.setUInt("modelIndex", index);
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }

    // Access to local bone info map for this model
    auto& GetBoneInfoMap() { return local_bone_info_map; }
    
    // Get local bone count
    int GetBoneCount() { return local_bone_info_map.size(); }

    // Update bounding box based on bone transformations
    void UpdateBoundingBox(const std::vector<glm::mat4>& boneTransforms) {
        // Find the bounding box in the vector that belongs to this model
        for (auto& box : interactable_element_boxes) {
            if (box.id == id && box.type == OBB) {
                // Calculate new OBB based on bone transformations
                // This is a simplified approach - you might want to use specific bone indices
                // that control this interactable model
                
                if (!boneTransforms.empty()) {
                    glm::mat4 transform = boneTransforms[0]; // Use first bone or specify controlling bones
                    
                    // Get original bounds (you might want to store these separately)
                    glm::vec3 originalMin = box.obb.center - box.obb.halfExtents;
                    glm::vec3 originalMax = box.obb.center + box.obb.halfExtents;
                    
                    // Create new OBB with the transform
                    box.obb = createOBBFromBounds(originalMin, originalMax, transform);
                }
                break;
            }
        }
    }

private:

};

class InteractorModelCreator {
private:
    int m_BoneCounter = 0;
    std::map<string, BoneInfo> m_BoneInfoMap;
    vector<glm::vec3> bindPosePositions;
    vector<glm::mat4> bindPoseMatrices;
    std::vector<TextureInfo> textures_loaded;

    auto& GetBoneInfoMap() { return m_BoneInfoMap; }
    int& GetBoneCount() { return m_BoneCounter; }

    void SetVertexBoneDataToDefault(DynamicVertex& vertex)
    {
        for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
        {
            vertex.m_BoneIDs[i] = -1;
            vertex.m_Weights[i] = 0.0f;
        }
    }

    void SetVertexBoneData(DynamicVertex& vertex, int boneID, float weight)
    {
        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
        {
            if (vertex.m_BoneIDs[i] < 0)
            {
                vertex.m_Weights[i] = weight;
                vertex.m_BoneIDs[i] = boneID;
                break;
            }
        }
    }

    void ExtractBoneWeightForVertices(std::vector<DynamicVertex>&  vertices, aiMesh* mesh, const aiScene* scene)
    {
        for (int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
        {
            int boneID = -1;
            std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
            if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end())
            {
                BoneInfo newBoneInfo;
                newBoneInfo.id = m_BoneCounter;
                std::cout<<newBoneInfo.id<<" " << boneName<<std::endl;
                newBoneInfo.offset = aiMatrix4x4ToGlm(
                    &mesh->mBones[boneIndex]->mOffsetMatrix);
                glm::mat4 currentTransform = glm::inverse(newBoneInfo.offset);
                bindPoseMatrices.push_back(glm::mat4(1.0f));
                bindPosePositions.push_back(currentTransform*glm::vec4(0.f,0.f,0.f,1.0f));
                
                m_BoneInfoMap[boneName] = newBoneInfo;
                boneID = m_BoneCounter;
                m_BoneCounter++;
                
            }
            else
            {
                boneID = m_BoneInfoMap[boneName].id;
            }
            assert(boneID != -1);
            auto weights = mesh->mBones[boneIndex]->mWeights;
            int numWeights = mesh->mBones[boneIndex]->mNumWeights;

            for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
            {
                int vertexId = weights[weightIndex].mVertexId;
                float weight = weights[weightIndex].mWeight;
                assert(vertexId <= vertices.size());
                SetVertexBoneData(vertices[vertexId], boneID, weight);
            }
        }
       
    }

    std::vector<TextureInfo> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName) {
        std::vector<TextureInfo> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);
            // Prevent duplicate loading of textures
            bool skip = false;
            for (unsigned int j = 0; j < textures_loaded.size(); j++) {
                if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }
            if (!skip) {
                // If texture hasn't been loaded already, load it
                TextureInfo texture;
                texture.id = LoadTexture(str.C_Str(), directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture); // Add to loaded textures
            }
        }
        return textures;
    }

    void processNode(aiNode* node, const aiScene* scene, vector<Mesh>& meshes, string& directory)
    {
        // process each mesh located at the current node
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene, directory));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        
        
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene, meshes, directory);
        }

    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene, string& directory)
    {
        // data to fill
        vector<DynamicVertex> vertices;
        vector<unsigned int> indices;
        //will be used for inverse kinematics base bone locations
        // walk through each of the mesh's vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            DynamicVertex vertex;
            SetVertexBoneDataToDefault(vertex);
            glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
            // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;
            // normals
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            // texture coordinates
            if (mesh->mTextureCoords[0]) {
                vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
                if (mesh->HasTangentsAndBitangents()) {
                    vertex.Tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
                    vertex.Bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
                } else {
                    vertex.Tangent = glm::vec3(0.0f);
                    vertex.Bitangent = glm::vec3(0.0f);
                }
            } else {
                vertex.TexCoords = glm::vec2(0.0f);
                vertex.Tangent = glm::vec3(0.0f);
                vertex.Bitangent = glm::vec3(0.0f);
            }

            vertices.push_back(vertex);
        }
        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
        
        ExtractBoneWeightForVertices(vertices, mesh, scene);
        // return a mesh object created from the extracted mesh data
        std::cout << m_BoneCounter;
        
        return Mesh(vertices, indices);
    }

public:
    string directory;

    void loadModel(string const& path, vector<Mesh>& meshes, std::map<string, BoneInfo>& boneInfoMap, vector<glm::vec3>& bindPosePos, vector<glm::mat4>& bindPoseMat, std::vector<TextureInfo>& texLoaded)
    {
        // read file via ASSIMP
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        // check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));

        // process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene, meshes, directory);

        // Copy data back
        boneInfoMap = m_BoneInfoMap;
        bindPosePos = bindPosePositions;
        bindPoseMat = bindPoseMatrices;
        texLoaded = textures_loaded;
    }
};

class InteractorModel
{
public:
    // model data 
    vector<Mesh>    meshes;
    string directory;
    bool gammaCorrection;
    std::map<string, BoneInfo> m_BoneInfoMap;
    vector<glm::vec3> bindPosePositions;
    vector<glm::mat4> bindPoseMatrices;
    std::vector<TextureInfo> textures_loaded; // Store loaded textures to prevent duplicates
    
    // Bone hierarchy for runtime inspection
    class BoneHierarchy* boneHierarchy;
    
    // constructor, expects a filepath to a 3D model.
    InteractorModel(string const& path, bool gamma = false);

    // Destructor to clean up bone hierarchy
    ~InteractorModel();

    // draws the model, and thus all its meshes
    void Draw(Shader& shader)
    {
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }
    
    // Access bone hierarchy
    BoneHierarchy* getBoneHierarchy() { return boneHierarchy; }
    const BoneHierarchy* getBoneHierarchy() const { return boneHierarchy; }

private:
    auto& GetBoneInfoMap() { return m_BoneInfoMap; }
};
 
//=====================================================================================
// MODIFIED SECTION FOR PBR AND EMBEDDED TEXTURES
//=====================================================================================

class SceneElementModelCreator {
private:
    std::vector<TextureInfo> textures_loaded;

    // Helper function to load texture from memory (for embedded textures)
    unsigned int LoadTextureFromData(const aiTexture* texture) {
        unsigned int textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        // Use stbi_load_from_memory to load the image data from the buffer
        // For compressed formats (like png/jpg), texture->mWidth holds the size of the buffer in bytes.
        // If texture->mHeight is 0, it's a compressed format.
        unsigned char *data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(texture->pcData), texture->mWidth, &width, &height, &nrComponents, 0);
        if (data) {
            GLenum format;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;
            else
                format = GL_RGB; // Default case

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
            std::cout << "Loaded embedded texture successfully." << std::endl;
        } else {
            std::cout << "Embedded texture failed to load: " << stbi_failure_reason() << std::endl;
            stbi_image_free(data);
        }

        return textureID;
    }

    void processNode(aiNode* node, const aiScene* scene, vector<StaticMesh>& meshes, string& directory, int modelId, glm::vec3& minBounds, glm::vec3& maxBounds)
    {
        // Process each mesh located at the current node
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene, directory, modelId, minBounds, maxBounds));
        }
        // Recursively process each of the children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene, meshes, directory, modelId, minBounds, maxBounds);
        }
    }

    StaticMesh processMesh(aiMesh* mesh, const aiScene* scene, string& directory, int modelId, glm::vec3& minBounds, glm::vec3& maxBounds)
    {
        // Data to fill
        std::vector<StaticVertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<TextureInfo> textures;
        
        // Walk through each of the mesh's vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            StaticVertex vertex;
            glm::vec3 vector;
            // Positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;
            
            // Update model bounds (accumulate across all meshes)
            minBounds.x = std::min(minBounds.x, vector.x);
            minBounds.y = std::min(minBounds.y, vector.y);
            minBounds.z = std::min(minBounds.z, vector.z);
            maxBounds.x = std::max(maxBounds.x, vector.x);
            maxBounds.y = std::max(maxBounds.y, vector.y);
            maxBounds.z = std::max(maxBounds.z, vector.z);
            
            // Normals
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            // Texture coordinates
            if (mesh->mTextureCoords[0])
            {
                vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
                if (mesh->HasTangentsAndBitangents())
                {
                    vertex.Tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
                    vertex.Bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
                }
                else
                {
                    vertex.Tangent = glm::vec3(0.0f);
                    vertex.Bitangent = glm::vec3(0.0f);
                }
            }
            else
            {
                vertex.TexCoords = glm::vec2(0.0f);
                vertex.Tangent = glm::vec3(0.0f);
                vertex.Bitangent = glm::vec3(0.0f);
            }

            vertices.push_back(vertex);
        }
        
        // Walk through each of the mesh's faces and retrieve the corresponding vertex indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        // Load PBR material textures, passing the 'scene' pointer
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        
        vector<TextureInfo> albedoMaps = loadMaterialTextures(material, scene, aiTextureType_DIFFUSE, "texture_albedo");
        textures.insert(textures.end(), albedoMaps.begin(), albedoMaps.end());
        
        vector<TextureInfo> metallicMaps = loadMaterialTextures(material, scene, aiTextureType_METALNESS, "texture_metallic");
        textures.insert(textures.end(), metallicMaps.begin(), metallicMaps.end());

        vector<TextureInfo> roughnessMaps = loadMaterialTextures(material, scene, aiTextureType_DIFFUSE_ROUGHNESS, "texture_roughness");
        textures.insert(textures.end(), roughnessMaps.begin(), roughnessMaps.end());
        
        vector<TextureInfo> normalMaps = loadMaterialTextures(material, scene, aiTextureType_NORMALS, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        
        vector<TextureInfo> aoMaps = loadMaterialTextures(material, scene, aiTextureType_AMBIENT_OCCLUSION, "texture_ao");
        textures.insert(textures.end(), aoMaps.begin(), aoMaps.end());

        return StaticMesh(vertices, indices, textures);
    }

    // Function now accepts the 'scene' pointer to handle embedded textures
    std::vector<TextureInfo> loadMaterialTextures(aiMaterial* mat, const aiScene* scene, aiTextureType type, std::string typeName) {
        std::vector<TextureInfo> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);
            
            bool skip = false;
            for (unsigned int j = 0; j < textures_loaded.size(); j++) {
                if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }
            if (!skip) {
                TextureInfo texture;
                // Check if the texture is an embedded texture (path starts with '*')
                if (str.C_Str()[0] == '*') {
                    int textureIndex = std::stoi(std::string(str.C_Str()).substr(1));
                    if (textureIndex < scene->mNumTextures) {
                        texture.id = LoadTextureFromData(scene->mTextures[textureIndex]);
                    } else {
                        std::cout << "Invalid embedded texture index: " << textureIndex << std::endl;
                        continue;
                    }
                } else {
                    // It's a regular file-based texture
                    texture.id = LoadTexture(str.C_Str(), directory);
                }
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
        }
        return textures;
    }

public:
    string directory;

    void loadModel(std::string const& path, vector<StaticMesh>& meshes, std::vector<TextureInfo>& texLoaded, int modelId)
    {
        // Read file via ASSIMP
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        // Check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return;
        }
        // Retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));

        // Initialize bounds tracking for the entire model
        glm::vec3 minBounds(FLT_MAX);
        glm::vec3 maxBounds(-FLT_MAX);

        // Process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene, meshes, directory, modelId, minBounds, maxBounds);

        // Create one AABB for the entire model after processing all meshes
        if (minBounds.x != FLT_MAX) { // Check if we found any vertices
            Shape s;
            s.type = AABB;
            s.id = modelId;
            
            // Create AABB from calculated bounds
            s.aabb.min = minBounds;
            s.aabb.max = maxBounds;
            
            scene_element_boxes.push_back(s);
        }

        // Copy data back
        texLoaded = textures_loaded;
    }
};

class SceneElementModel
{
public:
    // model data
    vector<StaticMesh> meshes;
    bool gammaCorrection;   
    std::string directory;
    std::vector<TextureInfo> textures_loaded;
    int id;
    unsigned int model_index;  // UBO model matrix index
    std::string original_file_path;  // Store original file path for serialization
    
    // Constructor, expects a filepath to a 3D model.
    SceneElementModel(std::string const& path, bool gamma = false) : gammaCorrection(gamma), original_file_path(path)
    {
        SceneElementModelCreator creator;
        id = get_next_unique_model_id();  // Use utility function for unique ID
        scene_element_count++;            // Keep count for other purposes
        creator.loadModel(path, meshes, textures_loaded, id);
        directory = creator.directory;
        model_index = 0; // Will be set during loading
    }

    // Add a draw function to the SceneElementModel class
    void Draw(Shader& shader) const {
        // Get the model index using the ID from the hashmap
        unsigned int index = model_index;
        shader.setUInt("modelIndex", index);
        for (unsigned int i = 0; i < meshes.size(); i++) {
            meshes[i].Draw(shader);
        }
    }

private:
};
 
#endif