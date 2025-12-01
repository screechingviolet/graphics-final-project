#include "realtime.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <iostream>
#include "settings.h"
#include "utils/shaderloader.h"

void Realtime::buildGeometry() {
    glGenBuffers(1, &m_sphereIds->shape_vbo);
    glGenVertexArrays(1, &m_sphereIds->shape_vao);
    m_sphere->updateParams(settings.shapeParameter1, settings.shapeParameter2);
    setupPrimitives(m_sphereIds, m_sphere->generateShape());

    glGenBuffers(1, &m_cubeIds->shape_vbo);
    glGenVertexArrays(1, &m_cubeIds->shape_vao);
    m_cube->updateParams(settings.shapeParameter1);
    setupPrimitives(m_cubeIds, m_cube->generateShape());

    glGenBuffers(1, &m_coneIds->shape_vbo);
    glGenVertexArrays(1, &m_coneIds->shape_vao);
    m_cone->updateParams(settings.shapeParameter1, settings.shapeParameter2);
    setupPrimitives(m_coneIds, m_cone->generateShape());

    glGenBuffers(1, &m_cylinderIds->shape_vbo);
    glGenVertexArrays(1, &m_cylinderIds->shape_vao);
    m_cylinder->updateParams(settings.shapeParameter1, settings.shapeParameter2);
    setupPrimitives(m_cylinderIds, m_cylinder->generateShape());
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
        setupPrimitives(&m_meshIds[meshfile], m_meshes[meshfile].generateShape(), m_meshes[meshfile].hasAnimation);
    }
}

void Realtime::setupPrimitives(VboVao* shape_ids, const std::vector<GLfloat>& triangles, bool anim) {
    glBindBuffer(GL_ARRAY_BUFFER, shape_ids->shape_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * triangles.size(), triangles.data(), GL_STATIC_DRAW);
    glBindVertexArray(shape_ids->shape_vao);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    if (anim) {
        std::cout << "anim\n";
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(GLfloat), reinterpret_cast<void*>(0));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));

        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);

        // glVertexAttribIPointer(2, 4, GL_INT, 14 * sizeof(GLfloat), reinterpret_cast<void*>(6 * sizeof(GLfloat)));
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 14 * sizeof(GLfloat), reinterpret_cast<void*>(6 * sizeof(GLfloat)));
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 14 * sizeof(GLfloat), reinterpret_cast<void*>(6 * sizeof(GLfloat) + 4 * sizeof(GLfloat))); // changed last GLfloat to int
    } else {
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void*>(0));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));
    }

    // Task 14: Unbind your VBO and VAO here
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
