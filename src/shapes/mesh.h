#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "cgltf.h"


class Mesh
{
public:
    void updateMesh(std::string meshfile);
    std::vector<float> generateShape() { return m_vertexData; }
    int num_triangles = 0;

private:
    std::vector<float> m_vertexData;
    void setVertexData(const char* meshfile);
    void fillVecFromAccessor(cgltf_accessor* acc, std::vector<glm::vec3>& vertices);
};
