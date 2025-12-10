#include "Cylinder.h"
#include "iostream"

void Cylinder::updateParams(int param1, int param2) {
    m_vertexData.clear(); // = std::vector<float>();
    m_param1 = std::max(1, param1);
    m_param2 = std::max(3, param2);
    setVertexData();
}


void Cylinder::setVertexData() {
    // TODO for Project 5: Lights, Camera
    float thetaStep = (2*M_PI)/m_param2;
    for (float theta = 0; theta < 2*M_PI; theta += thetaStep) {
        makeCapSlice(theta, theta+thetaStep);
        makeCapSliceTop(theta, theta+thetaStep);
        makeSlopeSlice(theta, theta+thetaStep);
    }

    num_triangles = m_vertexData.size()/(8);
    //num_triangles = 72;
}

// Inserts a glm::vec3 into a vector of floats.
// This will come in handy if you want to take advantage of vectors to build your shape!
void Cylinder::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    m_debugData.push_back("pos/normalx");
    data.push_back(v.y);
    m_debugData.push_back("pos/normaly");
    data.push_back(v.z);
    m_debugData.push_back("pos/normalz");
}

void Cylinder::insertUV(std::vector<float> &data, glm::vec3 v) {
    glm::vec2 uv = glm::vec2((v.x / m_radius) * 0.5f + 0.5f, (v.z / m_radius) * 0.5f + 0.5f);

    //std::cout << "uvx: " << uv.x << std::endl;
    //std::cout << "uvy: " << uv.y << std::endl;
    data.push_back(uv.x);
    m_debugData.push_back("uvx");
    data.push_back(uv.y);
    m_debugData.push_back("uvx");
}

void Cylinder::insertVertex(glm::vec3 vertexPos, glm::vec3 normal) {
    insertVec3(m_vertexData, vertexPos);
    insertVec3(m_vertexData, normal);
    insertUV(m_vertexData, vertexPos);
}

void Cylinder::makeCapTile(glm::vec3 topLeft,
                           glm::vec3 topRight,
                           glm::vec3 bottomLeft,
                           glm::vec3 bottomRight) {
    glm::vec3 topLeftToBottomLeft = bottomLeft-topLeft;
    glm::vec3 topLeftToTopRight = topRight-topLeft;
    glm::vec3 normal = glm::normalize(glm::cross(topLeftToBottomLeft, topLeftToTopRight));
    insertVertex(topLeft, normal);
    insertVertex(bottomLeft, normal);
    insertVertex(bottomRight, normal);
    /*insertVec3(m_vertexData, bottomLeft);
    insertVec3(m_vertexData, normal);
    insertUV(m_vertexData, bottomLeft);
    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, normal);
    insertUV(m_vertexData, bottomRight);
*/
    insertVertex(topLeft, normal);
    insertVertex(bottomRight, normal);
    insertVertex(topRight, normal);
/*
    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, normal);
    insertUV(m_vertexData, topLeft);
    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, normal);
    insertUV(m_vertexData, bottomRight);
    insertVec3(m_vertexData, topRight);
    insertVec3(m_vertexData, normal);
    insertUV(m_vertexData, topRight);*/
}

void Cylinder::makeCapSlice(float currentTheta, float nextTheta) {
    float step = m_radius/m_param1;
    // glm::vec3(0, -0.5, 0)
    // glm::vec3(m_radius*glm::cos(currentTheta), -0.5, m_radius*glm::sin(currentTheta)) is the outer point

    // add base triangle
    glm::vec3 firstTopLeft = glm::vec3((step)*glm::cos(currentTheta), -m_radius, (step)*glm::sin(currentTheta));
    glm::vec3 firstTopRight = glm::vec3((step)*glm::cos(nextTheta), -m_radius, (step)*glm::sin(nextTheta));
    glm::vec3 center = glm::vec3(0, -m_radius, 0);
    glm::vec3 normal = glm::normalize(glm::cross(firstTopLeft-center, firstTopRight-center));
    /*insertVec3(m_vertexData, firstTopRight);
    insertVec3(m_vertexData, normal);
    insertUV(m_vertexData, firstTopRight);
    insertVec3(m_vertexData, center);
    insertVec3(m_vertexData, normal);
    insertUV(m_vertexData, center);
    insertVec3(m_vertexData, firstTopLeft);
    insertVec3(m_vertexData, normal);
    insertUV(m_vertexData, firstTopLeft);*/

    insertVertex(firstTopRight, normal);
    insertVertex(center, normal);
    insertVertex(firstTopLeft, normal);

    float rad = step;
    for (int i = 0; i < m_param1-1; i++) { // float rad = step; rad < m_radius; rad += step
        makeCapTile(glm::vec3((rad)*glm::cos(currentTheta), -m_radius, (rad)*glm::sin(currentTheta)),
                    glm::vec3((rad)*glm::cos(nextTheta), -m_radius, (rad)*glm::sin(nextTheta)),
                    glm::vec3((rad+step)*glm::cos(currentTheta), -m_radius, (rad+step)*glm::sin(currentTheta)),
                    glm::vec3((rad+step)*glm::cos(nextTheta), -m_radius, (rad+step)*glm::sin(nextTheta)));
        rad += step;

    }
}

void Cylinder::makeCapSliceTop(float currentTheta, float nextTheta) {
    float step = m_radius/m_param1;
    // glm::vec3(0, -0.5, 0)

    // glm::vec3(m_radius*glm::cos(currentTheta), -0.5, m_radius*glm::sin(currentTheta)) is the outer point

    // add base triangle
    glm::vec3 firstTopRight = glm::vec3((step)*glm::cos(currentTheta), m_radius, (step)*glm::sin(currentTheta));
    glm::vec3 firstTopLeft = glm::vec3((step)*glm::cos(nextTheta), m_radius, (step)*glm::sin(nextTheta));
    glm::vec3 center = glm::vec3(0, m_radius, 0);
    glm::vec3 normal = glm::normalize(glm::cross(firstTopLeft-center, firstTopRight-center));
    /*insertVec3(m_vertexData, firstTopRight);
    insertVec3(m_vertexData, normal);
    insertUV(m_vertexData, firstTopRight);
    insertVec3(m_vertexData, center);
    insertVec3(m_vertexData, normal);
    insertUV(m_vertexData, center);
    insertVec3(m_vertexData, firstTopLeft);
    insertVec3(m_vertexData, normal);
    insertUV(m_vertexData, firstTopLeft);*/

    insertVertex(firstTopRight, normal);
    insertVertex(center, normal);
    insertVertex(firstTopLeft, normal);

    float rad = step;
    for (int i = 0; i < m_param1-1; i++) { // float rad = step; rad < m_radius; rad += step
        makeCapTile(glm::vec3((rad)*glm::cos(nextTheta), m_radius, (rad)*glm::sin(nextTheta)),
                    glm::vec3((rad)*glm::cos(currentTheta), m_radius, (rad)*glm::sin(currentTheta)),
                    glm::vec3((rad+step)*glm::cos(nextTheta), m_radius, (rad+step)*glm::sin(nextTheta)),
                    glm::vec3((rad+step)*glm::cos(currentTheta), m_radius, (rad+step)*glm::sin(currentTheta)));
        rad += step;

    }
}

void Cylinder::makeSlopeTile(glm::vec3 topLeft,
                             glm::vec3 topRight,
                             glm::vec3 bottomLeft,
                             glm::vec3 bottomRight) {
    /*insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, calcNorm(topLeft));
    insertUV(m_vertexData, topLeft);
    insertVec3(m_vertexData, bottomLeft);
    insertVec3(m_vertexData, calcNorm(bottomLeft));
    insertUV(m_vertexData, bottomLeft);
    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, calcNorm(bottomRight));
    insertUV(m_vertexData, bottomRight);

    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, calcNorm(topLeft));
    insertUV(m_vertexData, topLeft);
    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, calcNorm(bottomRight));
    insertUV(m_vertexData, bottomRight);
    insertVec3(m_vertexData, topRight);
    insertVec3(m_vertexData, calcNorm(topRight));
    insertUV(m_vertexData, topRight);
*/
    insertVertex(topLeft, calcNorm(topLeft));
    insertVertex(bottomLeft, calcNorm(bottomLeft));
    insertVertex(bottomRight, calcNorm(bottomRight));

    insertVertex(topLeft, calcNorm(topLeft));
    insertVertex(bottomRight, calcNorm(bottomRight));
    insertVertex(topRight, calcNorm(topRight));
}

void Cylinder::makeSlopeSlice(float currentTheta, float nextTheta) {
    float step = m_radius/m_param1;
    glm::vec3 bottomLeft = glm::vec3((m_radius)*glm::cos(currentTheta), -m_radius, (m_radius)*glm::sin(currentTheta));
    glm::vec3 bottomRight = glm::vec3((m_radius)*glm::cos(nextTheta), -m_radius, (m_radius)*glm::sin(nextTheta));
    glm::vec3 goingUpFromLeft = glm::vec3(0, 2*m_radius, 0)/(float)m_param1;
    glm::vec3 goingUpFromRight = glm::vec3(0, 2*m_radius, 0)/(float)m_param1;

    for (int i = 0; i < m_param1; i++) {
        makeSlopeTile(bottomLeft+(float)i*goingUpFromLeft, bottomRight+(float)i*goingUpFromRight,
                      bottomLeft+(float)(i+1)*goingUpFromLeft, bottomRight+(float)(i+1)*goingUpFromRight);
    }

}

glm::vec3 Cylinder::calcNorm(glm::vec3& pt) {
    float xNorm = (pt.x);
    float yNorm = 0;
    float zNorm = (pt.z);

    return glm::normalize(glm::vec3{ xNorm, yNorm, zNorm });
}

