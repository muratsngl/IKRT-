#ifndef MESH_H
#define MESH_H

#include <GL/glew.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shader.h"

#include <string>
#include <vector>
using namespace std;

#define MAX_BONE_INFLUENCE 4

struct DynamicVertex {
    // position
    glm::vec3 Position;
    // normal
    glm::vec3 Normal;
    // texture coordinates
    glm::vec2 TexCoords;
    // tangent vector for normal mapping
    glm::vec3 Tangent;
    // bitangent vector for normal mapping
    glm::vec3 Bitangent;
    int m_BoneIDs[MAX_BONE_INFLUENCE];
    //weights from each bone
    float m_Weights[MAX_BONE_INFLUENCE];
};

struct StaticVertex {
    // position
    glm::vec3 Position;
    // normal
    glm::vec3 Normal;
    // texture coordinates
    glm::vec2 TexCoords;
    // tangent vector for normal mapping
    glm::vec3 Tangent;
    // bitangent vector for normal mapping
    glm::vec3 Bitangent;
};
struct TextureInfo {
    unsigned int id;
    std::string type;
    std::string path;
};


class StaticMesh {
public:
    // mesh Data
    vector<StaticVertex>       vertices;      
    vector<unsigned int> indices;
    vector<TextureInfo> textures;
    unsigned int VAO;
    // constructor
    StaticMesh(vector<StaticVertex> vertices, vector<unsigned int> indices, vector<TextureInfo> textures = {})
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;
        // now that we have all the required data, set the vertex buffers and its attribute pointers.
        setupStaticMesh();
    }

    // UPDATED: Draw function now binds PBR textures to specific samplers
    void Draw(Shader& shader) const {
        // Bind appropriate textures for PBR
        for (unsigned int i = 0; i < textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i); // Activate proper texture unit before binding
            
            string name = textures[i].type;
            string uniformName;

            // Map texture type to a PBR shader uniform
            if (name == "texture_albedo")
                uniformName = "albedoMap";
            else if (name == "texture_metallic")
                uniformName = "metallicMap";
            else if (name == "texture_roughness")
                uniformName = "roughnessMap";
            else if (name == "texture_normal")
                uniformName = "normalMap";
            else if (name == "texture_ao")
                uniformName = "aoMap";

            // Set the sampler to the correct texture unit and bind the texture
            if (!uniformName.empty()) {
                glUniform1i(glGetUniformLocation(shader.ID, uniformName.c_str()), i);
                glBindTexture(GL_TEXTURE_2D, textures[i].id);
            }
        }

        // Draw the mesh
        shader.use();
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Always good practice to set everything back to defaults once configured.
        glActiveTexture(GL_TEXTURE0);
    }
private:
    
    

    unsigned int VBO, EBO;

    void setupStaticMesh()
    {
        // create buffers/arrays
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        // load data into vertex buffers
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // A great thing about structs is that their memory layout is sequential for all its items.
        // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
        // again translates to 3/2 floats which translates to a byte array.
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(StaticVertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        
        // set the vertex attribute pointers
        // vertex Positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(StaticVertex), (void*)0);
        // vertex normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(StaticVertex), (void*)offsetof(StaticVertex, Normal));
       // vertex texture coordinates
       glEnableVertexAttribArray(2);
       glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(StaticVertex), (void*)offsetof(StaticVertex, TexCoords));
       // vertex tangent
       glEnableVertexAttribArray(3);
       glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(StaticVertex), (void*)offsetof(StaticVertex, Tangent));
       // vertex bitangent
       glEnableVertexAttribArray(4);
       glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(StaticVertex), (void*)offsetof(StaticVertex, Bitangent));
       glBindVertexArray(0);
        
        vertices.clear();
        
    }
    
    void drawStaticMesh()
    {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};
class Mesh {
public:
    // mesh Data
    vector<DynamicVertex>       vertices;
    vector<unsigned int> indices;
    vector<TextureInfo> textures;
    unsigned int VAO;
    // constructor
    Mesh(vector<DynamicVertex> vertices, vector<unsigned int> indices, vector<TextureInfo> textures = {})
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;

        // now that we have all the required data, set the vertex buffers and its attribute pointers.
        setupMesh();
    }

    // render the mesh
    void Draw(Shader& shader) const
    {
        // Bind appropriate textures for PBR
        for (unsigned int i = 0; i < textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i); // Activate proper texture unit before binding
            
            string name = textures[i].type;
            string uniformName;

            // Map texture type to a PBR shader uniform
            if (name == "texture_albedo")
                uniformName = "albedoMap";
            else if (name == "texture_metallic")
                uniformName = "metallicMap";
            else if (name == "texture_roughness")
                uniformName = "roughnessMap";
            else if (name == "texture_normal")
                uniformName = "normalMap";
            else if (name == "texture_ao")
                uniformName = "aoMap";
            else
                continue; // Skip unknown texture types

            // Set the sampler uniform to the texture unit
            shader.setInt(uniformName, i);
            
            // Bind the texture
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }
        
        shader.use();
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glActiveTexture(GL_TEXTURE0);
    }

private:
    // render data 
    unsigned int VBO, EBO;

    // initializes all the buffer objects/arrays
    void setupMesh()
    {
        // create buffers/arrays
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        // load data into vertex buffers
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // A great thing about structs is that their memory layout is sequential for all its items.
        // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
        // again translates to 3/2 floats which translates to a byte array.
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(DynamicVertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // set the vertex attribute pointers
        // vertex Positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DynamicVertex), (void*)0);
        // vertex normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(DynamicVertex), (void*)offsetof(DynamicVertex, Normal));
       // vertex texture coordinates
       glEnableVertexAttribArray(2);
       glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(DynamicVertex), (void*)offsetof(DynamicVertex, TexCoords));
       // vertex tangent
       glEnableVertexAttribArray(3);
       glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(DynamicVertex), (void*)offsetof(DynamicVertex, Tangent));
       // vertex bitangent
       glEnableVertexAttribArray(4);
       glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(DynamicVertex), (void*)offsetof(DynamicVertex, Bitangent));

        // bone IDs (moved to end, after PBR attributes)
        glEnableVertexAttribArray(5);
        glVertexAttribIPointer(5, 4, GL_INT, sizeof(DynamicVertex), (void*)offsetof(DynamicVertex, m_BoneIDs));

        // weights (moved to end, after PBR attributes)
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(DynamicVertex), (void*)offsetof(DynamicVertex, m_Weights));
        glBindVertexArray(0);
        vertices.clear();
    }
};
#endif