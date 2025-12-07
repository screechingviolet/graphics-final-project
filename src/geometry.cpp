#include "realtime.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <iostream>
#include "settings.h"
#include "utils/shaderloader.h"
#include <glm/gtx/transform.hpp>

void Realtime::buildGeometry() {
    glGenBuffers(1, &m_sphereIds->shape_vbo);
    glGenVertexArrays(1, &m_sphereIds->shape_vao);
    m_sphere->updateParams(settings.shapeParameter1, settings.shapeParameter2);
    setupPrimitives(m_sphereIds, m_sphere->generateShape(), false, false);

    glGenBuffers(1, &m_cubeIds->shape_vbo);
    glGenVertexArrays(1, &m_cubeIds->shape_vao);
    m_cube->updateParams(settings.shapeParameter1);
    setupPrimitives(m_cubeIds, m_cube->generateShape(), false, false);

    glGenBuffers(1, &m_coneIds->shape_vbo);
    glGenVertexArrays(1, &m_coneIds->shape_vao);
    m_cone->updateParams(settings.shapeParameter1, settings.shapeParameter2);
    setupPrimitives(m_coneIds, m_cone->generateShape(), false, true);

    glGenBuffers(1, &m_cylinderIds->shape_vbo);
    glGenVertexArrays(1, &m_cylinderIds->shape_vao);
    m_cylinder->updateParams(settings.shapeParameter1, settings.shapeParameter2);
    setupPrimitives(m_cylinderIds, m_cylinder->generateShape(), false, false);

    setupLSystems();
    setupParticles();

    setupSkybox();
}

void Realtime::setupLSystems() {
    Cylinder unitCylinder;
    unitCylinder.updateParams(2, 3);
    m_LcylinderData = unitCylinder.generateShape();

    // Generate, and bind vao
    glGenVertexArrays(1, &m_vaoLcylinder);
    glBindVertexArray(m_vaoLcylinder);
    // Generate and bind VBO
    glGenBuffers(1, &m_vboLcylinder);
    glBindBuffer(GL_ARRAY_BUFFER, m_vboLcylinder);

    // Send data to VBO
    glBufferData(GL_ARRAY_BUFFER, m_LcylinderData.size() * sizeof(GLfloat), m_LcylinderData.data(), GL_STATIC_DRAW);

    // Enable and define attribute 0 to store vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void *>(0));

    // Enable and define attribute 1 to store normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void *>(sizeof(GLfloat) * 3));

    // Clean-up bindings
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Create L System Data
    m_LSystemScaler = 1;
    m_LSystemScaleProgression = 1;
    m_LSystemIterations = 3;
    m_axiom = "X";
    m_rules['X'] = "F+[[X]-X]-F[-FX]+X";
    m_rules['F'] = "FF";
}

void Realtime::setupParticles() {
    m_dt = 0.01;
    m_numParticles = 0;
    m_maxNumParticles = 100;

    // Initialise Particles
    for (unsigned int i = 0; i < m_maxNumParticles; ++i) {
        m_particles.push_back(Particle(glm::vec3(0, 0, 0)));

        for (int j = 0; j < 4; j++) {
            m_particulePositionSizeData.push_back(0);
        }

        for (int j = 0; j < 3; j++) {
            m_particuleColorData.push_back(0);
        }
    }

    m_particleCtm = glm::translate(glm::vec3(0, 0, 0));
    //m_particleCtm = glm::scale(glm::vec3(5, 5, 5));

    // The VBO containing the 4 vertices of the particles.
    // Thanks to instancing, they will be shared by all particles.
    m_particleVertexData = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        -0.5f, 0.5f, 0.0f,
        0.5f, 0.5f, 0.0f,
    };

    // Billboard
    glGenVertexArrays(1, &m_vaoParticles);
    glBindVertexArray(m_vaoParticles);

    glGenBuffers(1, &m_vboParticlesBillboard);
    glBindBuffer(GL_ARRAY_BUFFER, m_vboParticlesBillboard);

    glBufferData(GL_ARRAY_BUFFER, m_particleVertexData.size() * sizeof(GLfloat), m_particleVertexData.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2, // attribute. must match the layout in the shader.
        3, // size
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        3 * sizeof(GLfloat), // stride
        //0, // stride
        (void*)0 // array buffer offset
        );
    glVertexAttribDivisor(2, 0);

    // Positions and sizes of the particles
    glGenBuffers(1, &m_vboParticlesPositionSize);
    glBindBuffer(GL_ARRAY_BUFFER, m_vboParticlesPositionSize);

    // Initialize with empty (NULL) buffer : it will be updated later, each frame.
    glBufferData(GL_ARRAY_BUFFER, m_maxNumParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(
        3, // attribute. No particular reason for 3, but must match the layout in the shader.
        4, // size : x + y + z + size => 4
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        4 * sizeof(GLfloat), // stride
        //0, // stride
        (void*)0 // array buffer offset
        );
    glVertexAttribDivisor(3, 1);

    // Colors of the particles
    glGenBuffers(1, &m_vboParticlesColor);
    glBindBuffer(GL_ARRAY_BUFFER, m_vboParticlesColor);

    // Initialize with empty (NULL) buffer : it will be updated later, each frame.
    glBufferData(GL_ARRAY_BUFFER, m_maxNumParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(
        4, // attribute. No particular reason for 4, but must match the layout in the shader.
        4, // size : r + g + b + a => 4
        GL_UNSIGNED_BYTE, // type
        GL_TRUE, // normalized? *** YES, this means that the unsigned char[4] will be accessible with a vec4 (floats) in the shader ***
        4 * sizeof(GLfloat), // stride
        //0, // stride
        (void*)0 // array buffer offset
        );
    glVertexAttribDivisor(4, 1);

    std::vector<GLfloat> uvData = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f
    };

    glGenBuffers(1, &m_vboParticlesUV);
    glBindBuffer(GL_ARRAY_BUFFER, m_vboParticlesUV);

    glBufferData(GL_ARRAY_BUFFER, uvData.size() * sizeof(GLfloat), uvData.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
}


void Realtime::setupSkybox() {
    glGenBuffers(1, &m_skybox_vbo_id);
    glGenVertexArrays(1, &m_skybox_vao_id);
    glBindBuffer(GL_ARRAY_BUFFER, m_skybox_vbo_id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 108, m_skybox_vbo, GL_STATIC_DRAW);
    glBindVertexArray(m_skybox_vao_id);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), reinterpret_cast<void*>(0));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Realtime::deleteAllMeshes() {
    for (auto &[meshfile, vaovbo]: m_meshIds) {
        glDeleteVertexArrays(1, &vaovbo.shape_vao);
        glDeleteBuffers(1, &vaovbo.shape_vbo);
    }

    m_meshIds.clear();
    m_meshes.clear();
}

void Realtime::rebuildMeshes() {
    deleteAllMeshes();
    std::unordered_set<std::string> meshfiles;
    for (RenderShapeData& shape: m_renderdata.shapes) {
        if (shape.primitive.type == PrimitiveType::PRIMITIVE_MESH) {
            std::cout << "Found a mesh" << std::endl;
            meshfiles.insert(shape.primitive.meshfile);
        }
    }
    for (std::string meshfile: meshfiles) {
        glGenBuffers(1, &m_meshIds[meshfile].shape_vbo);
        glGenVertexArrays(1, &m_meshIds[meshfile].shape_vao);
        // create mesh and call update on the new mesh
        m_meshes[meshfile].updateMesh(meshfile);
        setupPrimitives(&m_meshIds[meshfile], m_meshes[meshfile].generateShape(), m_meshes[meshfile].hasAnimation, m_meshes[meshfile].hasTextures);
    }
}

void Realtime::setupPrimitives(VboVao* shape_ids, const std::vector<GLfloat>& triangles, bool anim, bool texturing) {
    glBindBuffer(GL_ARRAY_BUFFER, shape_ids->shape_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * triangles.size(), triangles.data(), GL_STATIC_DRAW);
    glBindVertexArray(shape_ids->shape_vao);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    std::cout << texturing << " am texturing\n";

    if (anim && texturing) {
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 16 * sizeof(GLfloat), reinterpret_cast<void*>(0));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 16 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 16 * sizeof(GLfloat), reinterpret_cast<void*>(6 * sizeof(GLfloat)));
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(GLfloat), reinterpret_cast<void*>(8 * sizeof(GLfloat)));
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(GLfloat), reinterpret_cast<void*>(12 * sizeof(GLfloat)));
    } else if (texturing) {
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void*>(0));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void*>(6 * sizeof(GLfloat)));
    } else if (anim) {
        std::cout << "anim\n";
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(GLfloat), reinterpret_cast<void*>(0));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));

        glEnableVertexAttribArray(3);
        glEnableVertexAttribArray(4);

        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 14 * sizeof(GLfloat), reinterpret_cast<void*>(6 * sizeof(GLfloat)));
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 14 * sizeof(GLfloat), reinterpret_cast<void*>(6 * sizeof(GLfloat) + 4 * sizeof(GLfloat))); // changed last GLfloat to int
    } else {
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void*>(0));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));
    }

    // Task 14: Unbind your VBO and VAO here
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
