#pragma once

// Defined before including GLEW to suppress deprecation messages on macOS
#include "camerapaths/camerapath.h"
#include "postprocessing/postprocess.h"
#include "postprocessing/colorgrade.h"
#include "postprocessing/convolution.h"
#include "postprocessing/fog.h"
#include "postprocessing/crepuscular.h"
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
    void finish();
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

    // Shadow mapping methods
    void createShadowResources();
    void deleteShadowResources();
    void renderShadowMaps();
    void bindShadowMapsToShader(GLuint shader);

public slots:
    void tick(QTimerEvent* event);

protected:
    void initializeGL() override;
    void paintScene();
    void paintGL() override;
    void resizeGL(int width, int height) override;

private:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

    // Tick Related Variables
    int m_timer;
    QElapsedTimer m_elapsedTimer;

    // Input Related Variables
    bool m_mouseDown = false;
    glm::vec2 m_prev_mouse_pos;
    std::unordered_map<Qt::Key, bool> m_keyMap;

    // Device Correction Variables
    double m_devicePixelRatio;

    GLuint m_shader = 0;
    GLuint m_skybox_shader = 0;
    RenderData m_renderdata;
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
    GLuint m_l_system_shader;
    bool updateToggle = true;

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
    std::optional<CameraPath> m_cameraPath;
    void updateCameraFromPath(PosRot posRot);

    // --- Shadow mapping ---
    static constexpr int MAX_SHADOW_CASTERS = 8;
    static constexpr int DEFAULT_SHADOW_SIZE = 2048;

    enum class ShadowType { None = 0, Directional = 1, Spot = 2, Point = 3 };

    struct ShadowMap {
        ShadowType type = ShadowType::None;
        bool enabled = false;
        GLuint fbo = 0;
        GLuint depthTex = 0;
        int size = DEFAULT_SHADOW_SIZE;
        int lightIndex = -1;
        glm::mat4 lightSpaceMatrix;
        GLuint rbo = 0;
    };

    std::vector<ShadowMap> m_shadowMaps;
    GLuint m_depthShader = 0;
    GLuint m_depthPointShader = 0;
    bool m_enablePCF = true;
    float m_shadowBias = 0.003f;
    float m_pointLightFar = 50.0f;
    bool m_shadowSettingsDirty = false;
};
