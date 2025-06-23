#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>

struct VirtualObject
{
    std::vector<unsigned int> index;
    std::vector<glm::vec3> pos;
    std::vector<glm::vec3> color;
    std::vector<glm::vec3> normal;
    std::vector<glm::vec3> tangent;
    std::vector<glm::vec2> texCoords;

};

std::vector<VirtualObject*> loadModelMeshes(const std::string& modelPath);
