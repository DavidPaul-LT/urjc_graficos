#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "MeshLoader.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <iostream>

std::vector<VirtualObject*> loadModelMeshes(const std::string& modelPath)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(modelPath,
        aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType |
        aiProcess_GenNormals |
        aiProcess_GenUVCoords);

    std::vector<VirtualObject*> objects;

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cerr << "ASSIMP Error: " << importer.GetErrorString() << std::endl;
        return objects;
    }

    for (unsigned int i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[i];
        VirtualObject* object = new VirtualObject;

        for (unsigned int j = 0; j < mesh->mNumVertices; j++)
        {
            object->pos.push_back(glm::vec3(
                mesh->mVertices[j].x,
                mesh->mVertices[j].y,
                mesh->mVertices[j].z));

            object->normal.push_back(glm::vec3(
                mesh->mNormals[j].x,
                mesh->mNormals[j].y,
                mesh->mNormals[j].z));

            if (mesh->HasTextureCoords(0))
            {
                object->texCoords.push_back(glm::vec2(
                    mesh->mTextureCoords[0][j].x,
                    mesh->mTextureCoords[0][j].y));
            }
            else
            {
                object->texCoords.push_back(glm::vec2(0.0f, 0.0f));
            }

            object->color.push_back(glm::vec3(0.5f));
            object->tangent.push_back(glm::vec3(0.0f));
        }

        for (unsigned int j = 0; j < mesh->mNumFaces; j++)
        {
            aiFace& face = mesh->mFaces[j];
            if (face.mNumIndices == 3)
            {
                object->index.push_back(face.mIndices[0]);
                object->index.push_back(face.mIndices[1]);
                object->index.push_back(face.mIndices[2]);
            }
        }

        objects.push_back(object);
    }

    return objects;
}
