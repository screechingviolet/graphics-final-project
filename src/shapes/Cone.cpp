#include "Cone.h"

void Cone::updateParams(int param1, int param2) {
    m_vertexData = std::vector<float>();
    m_param1 = param1;
    m_param2 = param2;
    setVertexData();
}

void Cone::makeCapTile(glm::vec3 topLeft,
                       glm::vec3 topRight,
                       glm::vec3 bottomLeft,
                       glm::vec3 bottomRight) {
    glm::vec3 topLeftToBottomLeft = bottomLeft-topLeft;
    glm::vec3 topLeftToTopRight = topRight-topLeft;
    glm::vec3 normal = glm::normalize(glm::cross(topLeftToBottomLeft, topLeftToTopRight));
    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, normal);
    insertVec3(m_vertexData, bottomLeft);
    insertVec3(m_vertexData, normal);
    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, normal);

    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, normal);
    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, normal);
    insertVec3(m_vertexData, topRight);
    insertVec3(m_vertexData, normal);
}

void Cone::makeCapSlice(float currentTheta, float nextTheta) {
    float step = m_radius/m_param1;
    // glm::vec3(0, -0.5, 0)

    // glm::vec3(m_radius*glm::cos(currentTheta), -0.5, m_radius*glm::sin(currentTheta)) is the outer point

    // add base triangle
    glm::vec3 firstTopLeft = glm::vec3((step)*glm::cos(currentTheta), -m_radius, (step)*glm::sin(currentTheta));
    glm::vec3 firstTopRight = glm::vec3((step)*glm::cos(nextTheta), -m_radius, (step)*glm::sin(nextTheta));
    glm::vec3 center = glm::vec3(0, -m_radius, 0);
    glm::vec3 normal = glm::normalize(glm::cross(firstTopLeft-center, firstTopRight-center));
    insertVec3(m_vertexData, firstTopRight);
    insertVec3(m_vertexData, normal);
    insertVec3(m_vertexData, center);
    insertVec3(m_vertexData, normal);
    insertVec3(m_vertexData, firstTopLeft);
    insertVec3(m_vertexData, normal);

    for (float rad = step; rad < m_radius; rad += step) {
        makeCapTile(glm::vec3((rad)*glm::cos(currentTheta), -m_radius, (rad)*glm::sin(currentTheta)),
                    glm::vec3((rad)*glm::cos(nextTheta), -m_radius, (rad)*glm::sin(nextTheta)),
                    glm::vec3((rad+step)*glm::cos(currentTheta), -m_radius, (rad+step)*glm::sin(currentTheta)),
                    glm::vec3((rad+step)*glm::cos(nextTheta), -m_radius, (rad+step)*glm::sin(nextTheta)));

    }

}

glm::vec3 Cone::calcNorm(glm::vec3& pt) {
    float xNorm = (2 * pt.x);
    float yNorm = -(1.f/4.f) * (2.f * pt.y - 1.f);
    float zNorm = (2 * pt.z);

    return glm::normalize(glm::vec3{ xNorm, yNorm, zNorm });
}

void Cone::makeSlopeTile(glm::vec3 topLeft,
                         glm::vec3 topRight,
                         glm::vec3 bottomLeft,
                         glm::vec3 bottomRight) {
    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, calcNorm(topLeft));
    insertVec3(m_vertexData, bottomLeft);
    insertVec3(m_vertexData, calcNorm(bottomLeft));
    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, calcNorm(bottomRight));

    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, calcNorm(topLeft));
    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, calcNorm(bottomRight));
    insertVec3(m_vertexData, topRight);
    insertVec3(m_vertexData, calcNorm(topRight));
}

void Cone::makeSlopeSlice(float currentTheta, float nextTheta) {
    float step = m_radius/m_param1;
    glm::vec3 topPoint = glm::vec3(0, 0.5, 0);
    glm::vec3 bottomLeft = glm::vec3((m_radius)*glm::cos(currentTheta), -m_radius, (m_radius)*glm::sin(currentTheta));
    glm::vec3 bottomRight = glm::vec3((m_radius)*glm::cos(nextTheta), -m_radius, (m_radius)*glm::sin(nextTheta));
    glm::vec3 goingUpFromLeft = (topPoint-bottomLeft)/(float)m_param1;
    glm::vec3 goingUpFromRight = (topPoint-bottomRight)/(float)m_param1;

    glm::vec3 baseTop = bottomLeft+(float)(m_param1-1)*goingUpFromLeft;
    glm::vec3 baseTop2 = bottomRight+(float)(m_param1-1)*goingUpFromRight;
    insertVec3(m_vertexData, baseTop);
    insertVec3(m_vertexData, calcNorm(baseTop));
    insertVec3(m_vertexData, topPoint);
    insertVec3(m_vertexData, glm::normalize((calcNorm(bottomRight)+calcNorm(bottomLeft))/2.f));
    insertVec3(m_vertexData, baseTop2);
    insertVec3(m_vertexData, calcNorm(baseTop2));

    for (int i = 0; i < m_param1-1; i++) {
        makeSlopeTile(bottomLeft+(float)i*goingUpFromLeft, bottomRight+(float)i*goingUpFromRight,
                      bottomLeft+(float)(i+1)*goingUpFromLeft, bottomRight+(float)(i+1)*goingUpFromRight);
    }

}

void Cone::setVertexData() {
    // TODO for Project 5: Lights, Camera
    float thetaStep = (2*M_PI)/m_param2;
    for (float theta = 0; theta < 2*M_PI; theta += thetaStep) {
        makeCapSlice(theta, theta+thetaStep);
        makeSlopeSlice(theta, theta+thetaStep);
    }

    num_triangles = m_vertexData.size()/6;
}


// Inserts a glm::vec3 into a vector of floats.
// This will come in handy if you want to take advantage of vectors to build your shape!
void Cone::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}
