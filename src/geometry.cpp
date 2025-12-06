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

    setupSkybox();

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
