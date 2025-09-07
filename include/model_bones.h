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
#include <cfloat>
using namespace std;

// Global declaration of scene element boxes
extern std::vector<Shape> scene_element_boxes;
extern std::vector<Shape> interactable_element_boxes;
extern int interactable_element_count;
extern int scene_element_count;

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
            if (global_bone_info_map.find(boneName) == global_bone_info_map.end())
            {
                BoneInfo newBoneInfo;
                newBoneInfo.id = global_bone_counter;
                std::cout << newBoneInfo.id << " " << boneName << std::endl;
                newBoneInfo.offset = aiMatrix4x4ToGlm(
                    &mesh->mBones[boneIndex]->mOffsetMatrix);
                glm::mat4 currentTransform = glm::inverse(newBoneInfo.offset);
                bindPoseMatrices.push_back(glm::mat4(1.0f));
                bindPosePositions.push_back(currentTransform * glm::vec4(0.f, 0.f, 0.f, 1.0f));
                
                global_bone_info_map[boneName] = newBoneInfo;
                boneID = global_bone_counter;
                global_bone_counter++;
            }
            else
            {
                boneID = global_bone_info_map[boneName].id;
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

    void processNode(aiNode* node, const aiScene* scene, vector<Mesh>& meshes, string& directory, vector<glm::vec3>& bindPosePositions, vector<glm::mat4>& bindPoseMatrices)
    {
        // process each mesh located at the current node
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene, directory, bindPosePositions, bindPoseMatrices));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene, meshes, directory, bindPosePositions, bindPoseMatrices);
        }
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene, string& directory, vector<glm::vec3>& bindPosePositions, vector<glm::mat4>& bindPoseMatrices)
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
        // now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        ExtractBoneWeightForVertices(vertices, mesh, scene, bindPosePositions, bindPoseMatrices);
        // return a mesh object created from the extracted mesh data
        return Mesh(vertices, indices);
    }

public:
    string directory;

    void loadModel(string const& path, vector<Mesh>& meshes, vector<glm::vec3>& bindPosePos, vector<glm::mat4>& bindPoseMat, std::vector<TextureInfo>& texLoaded)
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
        processNode(scene->mRootNode, scene, meshes, directory, bindPosePos, bindPoseMat);

        // Copy data back
        texLoaded = textures_loaded;
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
    int id;
    // constructor, expects a filepath to a 3D model.
    InteractableModel(string const& path, bool gamma = false) : gammaCorrection(gamma)
    {
        InteractableModelCreator creator;
        creator.loadModel(path, meshes, bindPosePositions, bindPoseMatrices, textures_loaded);
        directory = creator.directory;
        id = interactable_element_count++;
        CreateBoundingBox(); // Create initial bounding box after loading
    }

    // draws the model, and thus all its meshes
    void Draw(Shader& shader) const
    {
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }

    // Access to global bone info map for consistent bone indices
    auto& GetBoneInfoMap() { return global_bone_info_map; }

    // Create bounding box for the interactable model
    void CreateBoundingBox() {
        if (meshes.empty()) return;
        
        // Calculate overall bounding box from all meshes
        glm::vec3 minBounds(FLT_MAX);
        glm::vec3 maxBounds(-FLT_MAX);
        bool hasVertices = false;
        
        for (const auto& mesh : meshes) {
            // Access mesh vertex data to calculate actual bounds
            // Note: This assumes you have access to vertex positions from the mesh
            // You might need to modify this based on your Mesh class implementation
            
            // For now, use default bounds - you should replace this with actual vertex iteration
            if (!hasVertices) {
                minBounds = glm::vec3(-1.0f);
                maxBounds = glm::vec3(1.0f);
                hasVertices = true;
            }
        }
        
        // Create OBB instead of AABB
        Shape s;
        s.type = OBB;
        s.id = id; // Use the model's ID
        
        // Create OBB with identity transform initially
        glm::mat4 identityTransform = glm::mat4(1.0f);
        s.obb = createOBBFromBounds(minBounds, maxBounds, identityTransform);
        
        interactable_element_boxes.push_back(s);
    }
    
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
    // constructor, expects a filepath to a 3D model.
    InteractorModel(string const& path, bool gamma = false) : gammaCorrection(gamma)
    {
        InteractorModelCreator creator;
        creator.loadModel(path, meshes, m_BoneInfoMap, bindPosePositions, bindPoseMatrices, textures_loaded);
        directory = creator.directory;
    }

    // draws the model, and thus all its meshes
    void Draw(Shader& shader)
    {
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }

private:
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
     //
    

    auto& GetBoneInfoMap() { return m_BoneInfoMap; }

};
 




class SceneElementModelCreator {
private:
    std::vector<TextureInfo> textures_loaded;

    void processNode(aiNode* node, const aiScene* scene, vector<StaticMesh>& meshes, string& directory)
    {
        // Process each mesh located at the current node
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene, directory));
        }
        // Recursively process each of the children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene, meshes, directory);
        }
    }

    StaticMesh processMesh(aiMesh* mesh, const aiScene* scene, string& directory)
    {
        // Data to fill
        std::vector<StaticVertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<TextureInfo> textures;
        Aabb boundingBox;
        // Initialize bounding box min/max to first vertex or large/small values
        if (mesh->mNumVertices > 0) {
            boundingBox.min = glm::vec3(mesh->mVertices[0].x, mesh->mVertices[0].y, mesh->mVertices[0].z);
            boundingBox.max = glm::vec3(mesh->mVertices[0].x, mesh->mVertices[0].y, mesh->mVertices[0].z);
        } else {
            boundingBox.min = glm::vec3(0.0f);
            boundingBox.max = glm::vec3(0.0f);
        }
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
            // Update bounding box
            boundingBox.min.x = std::min(boundingBox.min.x, vector.x);
            boundingBox.min.y = std::min(boundingBox.min.y, vector.y);
            boundingBox.min.z = std::min(boundingBox.min.z, vector.z);
            boundingBox.max.x = std::max(boundingBox.max.x, vector.x);
            boundingBox.max.y = std::max(boundingBox.max.y, vector.y);
            boundingBox.max.z = std::max(boundingBox.max.z, vector.z);
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
        {
            Shape s;
            s.type = AABB;
            s.id = scene_element_count++; // Assign unique ID to each scene element
            s.aabb = boundingBox;
            scene_element_boxes.push_back(s);
        }
        // Walk through each of the mesh's faces and retrieve the corresponding vertex indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        // Load material textures
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        
        // Diffuse maps
        std::vector<TextureInfo> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        
        // Specular maps
        std::vector<TextureInfo> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        
        // Normal maps
        std::vector<TextureInfo> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        
        // Height maps
        std::vector<TextureInfo> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

        // Return a StaticMesh object created from the extracted mesh data
        return StaticMesh(vertices, indices, textures);
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

public:
    string directory;

    void loadModel(std::string const& path, vector<StaticMesh>& meshes, std::vector<TextureInfo>& texLoaded)
    {
        // Read file via ASSIMP
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        // Check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return;
        }
        // Retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));

        // Process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene, meshes, directory);

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
    // Constructor, expects a filepath to a 3D model.
    SceneElementModel(std::string const& path, bool gamma = false) : gammaCorrection(gamma)
    {
        SceneElementModelCreator creator;
        creator.loadModel(path, meshes, textures_loaded);
        directory = creator.directory;
        id = scene_element_count;
    }

    // Add a draw function to the SceneElementModel class
    void Draw(Shader& shader) const {
        for (unsigned int i = 0; i < meshes.size(); i++) {
            meshes[i].Draw(shader);
        }
    }

private:
};
 




#endif