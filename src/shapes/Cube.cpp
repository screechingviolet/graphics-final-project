#include "Cube.h"

void Cube::updateParams(int param1) {
    m_vertexData = std::vector<float>();
    m_param1 = param1;
    setVertexData();
}

void Cube::makeTile(glm::vec3 topLeft,
                    glm::vec3 topRight,
                    glm::vec3 bottomLeft,
                    glm::vec3 bottomRight) {
    // Task 2: create a tile (i.e. 2 triangles) based on 4 given points.
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

void Cube::makeFace(glm::vec3 topLeft,
                    glm::vec3 topRight,
                    glm::vec3 bottomLeft,
                    glm::vec3 bottomRight) {
    // Task 3: create a single side of the cube out of the 4
    //         given points and makeTile()
    // Note: think about how param 1 affects the number of triangles on
    //       the face of the cube

    glm::vec3 tileXVector = (topRight-topLeft)/(float)m_param1;
    glm::vec3 tileYVector = (bottomLeft-topLeft)/(float)m_param1;
    float tileWidth = glm::length(tileXVector);
    float tileHeight = glm::length(tileYVector);
    for (int col = 0; col < m_param1; col++) {
        for (int row = 0; row < m_param1; row++) {
            makeTile(topLeft+(float)col*tileXVector+(float)row*tileYVector,
                     topLeft+(float)(col+1)*tileXVector+(float)row*tileYVector,
                     topLeft+(float)col*tileXVector+(float)(row+1)*tileYVector,
                     topLeft+(float)(col+1)*tileXVector+(float)(row+1)*tileYVector);
        }
    }
    // for (float col = topLeft.x; col < topRight.x; col += tileWidth) {
    //     for (float row = topLeft.y; row < bottomLeft.y; row += tileHeight) {
    //         makeTile(
    //     }
    // }


}

void Cube::setVertexData() {
    // Uncomment these lines for Task 2, then comment them out for Task 3:

    // makeTile(glm::vec3(-0.5f,  0.5f, 0.5f),
    //          glm::vec3( 0.5f,  0.5f, 0.5f),
    //          glm::vec3(-0.5f, -0.5f, 0.5f),
    //          glm::vec3( 0.5f, -0.5f, 0.5f));

    // Uncomment these lines for Task 3:

    makeFace(glm::vec3(-0.5f,  0.5f, 0.5f),
             glm::vec3( 0.5f,  0.5f, 0.5f),
             glm::vec3(-0.5f, -0.5f, 0.5f),
             glm::vec3( 0.5f, -0.5f, 0.5f));

    // Task 4: Use the makeFace() function to make all 6 sides of the

    makeFace(glm::vec3( 0.5f,  0.5f, -0.5f),
             glm::vec3(-0.5f,  0.5f, -0.5f),
             glm::vec3( 0.5f, -0.5f, -0.5f),
             glm::vec3(-0.5f, -0.5f, -0.5f));

    makeFace(glm::vec3( 0.5f,  0.5f, 0.5f),
             glm::vec3(-0.5f,  0.5f, 0.5f),
             glm::vec3( 0.5f, 0.5f, -0.5f),
             glm::vec3(-0.5f, 0.5f, -0.5f));

    makeFace(glm::vec3(-0.5f,  -0.5f, 0.5f),
             glm::vec3( 0.5f,  -0.5f, 0.5f),
             glm::vec3(-0.5f, -0.5f, -0.5f),
             glm::vec3( 0.5f, -0.5f, -0.5f));

    makeFace(glm::vec3(0.5f,  -0.5f, 0.5f),
             glm::vec3(0.5f,  0.5f, 0.5f),
             glm::vec3(0.5f, -0.5f, -0.5f),
             glm::vec3(0.5f, 0.5f, -0.5f));

    makeFace(glm::vec3(-0.5f,  0.5f, 0.5f),
             glm::vec3(-0.5f,  -0.5f, 0.5f),
             glm::vec3(-0.5f, 0.5f, -0.5f),
             glm::vec3(-0.5f, -0.5f, -0.5f));

    num_triangles = m_vertexData.size()/6;
}

// Inserts a glm::vec3 into a vector of floats.
// This will come in handy if you want to take advantage of vectors to build your shape!
void Cube::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}
