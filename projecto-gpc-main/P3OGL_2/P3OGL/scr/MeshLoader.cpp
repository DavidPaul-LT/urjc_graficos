#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "MeshLoader.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <iostream>

std::vector<VirtualObject*> loadMesh(const std::string& fileName)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(fileName,
        aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType |
        aiProcess_GenNormals |
        aiProcess_GenUVCoords);

    std::vector<VirtualObject*> outScene;

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cerr << "ASSIMP Error: " << importer.GetErrorString() << std::endl;
        return outScene;
    }

    for (unsigned int i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[i];
        VirtualObject* vo = new VirtualObject;

        for (unsigned int j = 0; j < mesh->mNumVertices; j++)
        {
            vo->pos.push_back(glm::vec3(
                mesh->mVertices[j].x,
                mesh->mVertices[j].y,
                mesh->mVertices[j].z));

            vo->normal.push_back(glm::vec3(
                mesh->mNormals[j].x,
                mesh->mNormals[j].y,
                mesh->mNormals[j].z));

            if (mesh->HasTextureCoords(0))
            {
                vo->textCoord.push_back(glm::vec2(
                    mesh->mTextureCoords[0][j].x,
                    mesh->mTextureCoords[0][j].y));
            }
            else
            {
                vo->textCoord.push_back(glm::vec2(0.0f, 0.0f));
            }

            vo->color.push_back(glm::vec3(0.5f));  // color gris por defecto
            vo->tangent.push_back(glm::vec3(0.0f)); // puedes mejorarlo luego
        }

        for (unsigned int j = 0; j < mesh->mNumFaces; j++)
        {
            aiFace& face = mesh->mFaces[j];
            if (face.mNumIndices == 3)
            {
                vo->idx.push_back(face.mIndices[0]);
                vo->idx.push_back(face.mIndices[1]);
                vo->idx.push_back(face.mIndices[2]);
            }
        }

        outScene.push_back(vo);
    }

    return outScene;
}
