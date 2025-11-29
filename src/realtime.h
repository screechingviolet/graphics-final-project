#pragma once

// Defined before including GLEW to suppress deprecation messages on macOS
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GL/glew.h>
#include <glm/glm.hpp>

#include <unordered_map>
#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QTime>
#include <QTimer>
#include "utils/scenedata.h"
#include "utils/sceneparser.h"
#include "camera.h"
#include <shapes/Sphere.h>
#include <shapes/Cube.h>
#include <shapes/Cone.h>
#include <shapes/Cylinder.h>
#include <shapes/mesh.h>


struct VboVao {
    GLuint shape_vbo = 0;
    GLuint shape_vao = 0;
};

class Realtime : public QOpenGLWidget
{
public:
    Realtime(QWidget *parent = nullptr);
    void finish();                                      // Called on program exit
    void sceneChanged();
    void settingsChanged();
    void saveViewportImage(std::string filePath);
    void declGeneralUniforms();
    void declSpecificUniforms(RenderShapeData& shape);
    void declareCameraUniforms();
    void rebuildMatrices();
    void rebuildCamera();
    void setupPrimitives(VboVao* shape_ids, const std::vector<GLfloat>& triangles);
    void deleteAllMeshes();
    void rebuildMeshes();
    glm::mat4 rotationhelper(glm::vec4 u, float angle);

public slots:
    void tick(QTimerEvent* event);                      // Called once per tick of m_timer

protected:
    void initializeGL() override;                       // Called once at the start of the program
    void paintGL() override;                            // Called whenever the OpenGL context changes or by an update() request
    void resizeGL(int width, int height) override;      // Called when window size changes

private:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

    // Tick Related Variables
    int m_timer;                                        // Stores timer which attempts to run ~60 times per second
    QElapsedTimer m_elapsedTimer;                       // Stores timer which keeps track of actual time between frames

    // Input Related Variables
    bool m_mouseDown = false;                           // Stores state of left mouse button
    glm::vec2 m_prev_mouse_pos;                         // Stores mouse position
    std::unordered_map<Qt::Key, bool> m_keyMap;         // Stores whether keys are pressed or not

    // Device Correction Variables
    double m_devicePixelRatio;

    GLuint m_shader = 0; // Stores id of shader program
    RenderData m_renderdata;
    // GLuint m_vbo;    // Stores id of VBO
    // GLuint m_vao;    // Stores id of VAO
    glm::mat4 m_proj, m_zoom;
    Camera m_cam;
    float near, far;

    Sphere* m_sphere = new Sphere();
    VboVao* m_sphereIds = new VboVao();

    Cube* m_cube = new Cube();
    VboVao* m_cubeIds = new VboVao();

    Cone* m_cone = new Cone();
    VboVao* m_coneIds = new VboVao();

    Cylinder* m_cylinder = new Cylinder();
    VboVao* m_cylinderIds = new VboVao();

    std::unordered_map<std::string, Mesh> m_meshes;
    std::unordered_map<std::string, VboVao> m_meshIds;



};
