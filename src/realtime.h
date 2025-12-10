#pragma once

// Defined before including GLEW to suppress deprecation messages on macOS
#include "camerapaths/camerapath.h"
#include "postprocessing/postprocess.h"
#include "postprocessing/colorgrade.h"
#include "postprocessing/convolution.h"
#include "postprocessing/fog.h"
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
#include <particles.h>


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
    void declareSkyboxUniforms();
    void rebuildMatrices();
    void rebuildCamera();
    void setupPrimitives(VboVao* shape_ids, const std::vector<GLfloat>& triangles, bool anim = false, bool texturing = false);
    void deleteAllMeshes();
    void rebuildMeshes();
    void buildGeometry();
    void initializeTextures(std::string filepath);
    void setupSkybox();
    void drawSkybox();
    // void paintTextures();
    glm::mat4 rotationhelper(glm::vec4 u, float angle);
    void activateCameraPath(CameraPath cameraPath);
    void setupLSystems();
    void setupParticles();
    void particleUpdate();
    int FindUnusedParticle();
    QString generateLSystemString(std::map<QChar, QString> rules, QString axiom, int numIterations);
    SceneNode* createLSystemNode(QString data);
    SceneNode* createLSystemNodeHelper(QString data, float localScale, float angle);
    void updateLSystems();
    void paintLSystems();
    void paintParticles();
    void makeFBO();
    void paintFog();

public slots:
    void tick(QTimerEvent* event);                      // Called once per tick of m_timer

protected:
    void initializeGL() override;                       // Called once at the start of the program
    void paintScene();
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
    GLuint m_skybox_shader = 0;
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

    // L-System Details
    std::vector<SceneNode*> m_LSystems;
    RenderData m_LSystemMetaData;
    float m_LSystemScaler;
    float m_LSystemScaleProgression;
    float m_LSystemIterations;
    bool m_LSystemIterationsChanged;
    std::map<QChar, QString> m_rules;
    QString m_axiom;
    float m_angle = (5.0 / 36.0) * M_PI;
    GLuint m_vboLcylinder;
    GLuint m_vaoLcylinder;
    std::vector<GLfloat> m_LcylinderData;

    // Particle Details
    int m_numParticles;
    int m_maxNumParticles;
    std::vector<Particle> m_particles;
    float m_dt;
    GLuint m_particleShader;
    GLuint m_vboParticlesBillboard;
    GLuint m_vboParticlesPositionSize;
    GLuint m_vboParticlesColor;
    GLuint m_vboParticlesUV;
    GLuint m_vaoParticles;
    int m_lastUsedParticle = 0;
    std::vector<GLfloat> m_particleVertexData;
    std::vector<GLfloat> m_particulePositionSizeData;
    std::vector<GLfloat> m_particuleColorData;
    glm::mat4 m_particleCtm;
    float m_particleVelocityGlobal;

    // Fog Details
    GLuint m_fbo_texture;
    GLuint m_fbo_depthTexture;
    GLuint m_fbo_width;
    GLuint m_fbo_height;
    GLuint m_fbo_renderbuffer;
    GLuint m_fbo;
    GLuint m_defaultFBO;
    GLuint m_fogShader;

    // Scrolling Details
    float time_elapsed = 0;

    std::vector<GLuint> m_textures;
    std::map<std::string, int> m_texIndexLUT;
    GLuint m_skybox;
    //postprocessing
    std::vector<std::unique_ptr<PostProcess>> m_postprocesses;

    GLuint m_skybox_vbo_id = 0, m_skybox_vao_id = 0;
    int m_skybox_size = 1;
    // FROM LEARN_OPENGL
    float m_skybox_vbo[108] = {
      -1.0f,  1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f,
      1.0f, -1.0f, -1.0f,
      1.0f, -1.0f, -1.0f,
      1.0f,  1.0f, -1.0f,
      -1.0f,  1.0f, -1.0f,

      -1.0f, -1.0f,  1.0f,
      -1.0f, -1.0f, -1.0f,
      -1.0f,  1.0f, -1.0f,
      -1.0f,  1.0f, -1.0f,
      -1.0f,  1.0f,  1.0f,
      -1.0f, -1.0f,  1.0f,

      1.0f, -1.0f, -1.0f,
      1.0f, -1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,
      1.0f,  1.0f, -1.0f,
      1.0f, -1.0f, -1.0f,

      -1.0f, -1.0f,  1.0f,
      -1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,
      1.0f, -1.0f,  1.0f,
      -1.0f, -1.0f,  1.0f,

      -1.0f,  1.0f, -1.0f,
      1.0f,  1.0f, -1.0f,
      1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,
      -1.0f,  1.0f,  1.0f,
      -1.0f,  1.0f, -1.0f,

      -1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f,  1.0f,
      1.0f, -1.0f, -1.0f,
      1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f,  1.0f,
      1.0f, -1.0f,  1.0f
    };

    // camera paths
    std::chrono::high_resolution_clock::time_point m_pathStartTime;
    float getPathTime();
    std::optional<CameraPath> m_cameraPath; // camera path, if one is active
    void updateCameraFromPath(PosRot posRot);

};
