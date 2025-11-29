#include "Sphere.h"
#include <iostream>

void Sphere::updateParams(int param1, int param2) {
    m_vertexData.clear(); // = std::vector<float>();
    m_param1 = std::max(2, param1);
    m_param2 = std::max(3, param2);
    setVertexData();
}

void Sphere::makeTile(glm::vec3 topLeft,
                      glm::vec3 topRight,
                      glm::vec3 bottomLeft,
                      glm::vec3 bottomRight) {
    // Task 5: Implement the makeTile() function for a Sphere
    // Note: this function is very similar to the makeTile() function for Cube,
    //       but the normals are calculated in a different way!
    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, glm::normalize(topLeft));
    insertVec3(m_vertexData, bottomLeft);
    insertVec3(m_vertexData, glm::normalize(bottomLeft));
    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, glm::normalize(bottomRight));

    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, glm::normalize(topLeft));
    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, glm::normalize(bottomRight));
    insertVec3(m_vertexData, topRight);
    insertVec3(m_vertexData, glm::normalize(topRight));
}

void Sphere::makeWedge(float currentTheta, float nextTheta) {
    // Task 6: create a single wedge of the sphere using the
    //         makeTile() function you implemented in Task 5
    // Note: think about how param 1 comes into play here!
    float angleStep = (M_PI)/m_param1;
    float currTheta = currentTheta;
    float nxtTheta = nextTheta;
    // std::cout << currTheta << " " << nxtTheta << std::endl;
    for (float phi = 0.; phi < M_PI; phi += angleStep) {
        // top left is (x,y,z) with currenttheta and phi
        // top right is nextheta and phi
        makeTile(glm::vec3(0.5*glm::sin(phi)*glm::cos(currTheta), 0.5*cos(phi), -0.5 * sin(phi) * sin(currTheta)),
                 glm::vec3(0.5*glm::sin(phi)*glm::cos(nxtTheta), 0.5*cos(phi), -0.5 * sin(phi) * sin(nxtTheta)),
                 glm::vec3(0.5*glm::sin(phi+angleStep)*glm::cos(currTheta), 0.5*cos(phi+angleStep), -0.5 * sin(phi+angleStep) * sin(currTheta)),
                 glm::vec3(0.5*glm::sin(phi+angleStep)*glm::cos(nxtTheta), 0.5*cos(phi+angleStep), -0.5 * sin(phi+angleStep) * sin(nxtTheta)));
    }
}

void Sphere::makeSphere() {
    // Task 7: create a full sphere using the makeWedge() function you
    //         implemented in Task 6
    // Note: think about how param 2 comes into play here!
    float thetaStep = (2*M_PI)/m_param2;
    for (float theta = 0; theta < 2*M_PI; theta += thetaStep) {
        makeWedge(theta, theta+thetaStep);
    }
}

void Sphere::setVertexData() {
    // Uncomment these lines to make a wedge for Task 6, then comment them out for Task 7:

    // float thetaStep = glm::radians(360.f / m_param2);
    // float currentTheta = 0 * thetaStep;
    // float nextTheta = 1 * thetaStep;
    // makeWedge(currentTheta, nextTheta);

    // Uncomment these lines to make sphere for Task 7:

    makeSphere();
    num_triangles = m_vertexData.size()/6;
}

// Inserts a glm::vec3 into a vector of floats.
// This will come in handy if you want to take advantage of vectors to build your shape!
void Sphere::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}
