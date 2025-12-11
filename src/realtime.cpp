#include "realtime.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <iostream>
#include "settings.h"
#include "utils/shaderloader.h"
#include <glm/gtx/transform.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "postprocessing/crepuscular.h"

// Helper structures for light space calculations
struct LightSpace {
    glm::mat4 view;
    glm::mat4 proj;
};

struct CubeLightSpace {
    glm::mat4 views[6];
    glm::mat4 proj;
};

void checkGLError(const char* location) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL Error at " << location << ": " << err
                  << " (0x" << std::hex << err << std::dec << ")" << std::endl;
    }
}

// ================== Rendering the Scene!

Realtime::Realtime(QWidget *parent)
    : QOpenGLWidget(parent)
{
    m_prev_mouse_pos = glm::vec2(size().width()/2, size().height()/2);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    m_keyMap[Qt::Key_W]       = false;
    m_keyMap[Qt::Key_A]       = false;
    m_keyMap[Qt::Key_S]       = false;
    m_keyMap[Qt::Key_D]       = false;
    m_keyMap[Qt::Key_Control] = false;
    m_keyMap[Qt::Key_Space]   = false;

    // If you must use this function, do not edit anything above this
}

void Realtime::finish() {
    killTimer(m_timer);
    this->makeCurrent();

    // Students: anything requiring OpenGL calls when the program exits should be done here

    glDeleteBuffers(1, &m_sphereIds->shape_vbo);
    glDeleteBuffers(1, &m_cubeIds->shape_vbo);
    glDeleteBuffers(1, &m_coneIds->shape_vbo);
    glDeleteBuffers(1, &m_cylinderIds->shape_vbo);
    glDeleteVertexArrays(1, &m_sphereIds->shape_vao);
    glDeleteVertexArrays(1, &m_cubeIds->shape_vao);
    glDeleteVertexArrays(1, &m_coneIds->shape_vao);
    glDeleteVertexArrays(1, &m_cylinderIds->shape_vao);

    delete m_sphere;
    delete m_cube;
    delete m_cylinder;
    delete m_cone;
    delete m_sphereIds;
    delete m_cubeIds;
    delete m_cylinderIds;
    delete m_coneIds;
    glDeleteProgram(m_shader);

    for (int i = 0; i < m_postprocesses.size(); i++) {
        m_postprocesses[i]->destroyFBO();
        glDeleteProgram(m_postprocesses[i]->getShader());
        m_postprocesses[i]->destroyVertex();
    }

    deleteShadowResources();

    this->doneCurrent();
}

void Realtime::initializeGL() {
    m_devicePixelRatio = this->devicePixelRatio();

    m_timer = startTimer(1000/60);
    m_elapsedTimer.start();

    // Initializing GL.
    // GLEW (GL Extension Wrangler) provides access to OpenGL functions.
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Error while initializing GL: " << glewGetErrorString(err) << std::endl;
    }
    std::cout << "Initialized GL: Version " << glewGetString(GLEW_VERSION) << std::endl;

    // Allows OpenGL to draw objects appropriately on top of one another
    glEnable(GL_DEPTH_TEST);
    // Tells OpenGL to only draw the front face
    glEnable(GL_CULL_FACE);
    // Tells OpenGL how big the screen is
    glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);

    // ------ my code starts here -----
    glClearColor(0, 0, 0, 1.0);


    m_shader = ShaderLoader::createShaderProgram(":/resources/shaders/anim.vert", ":/resources/shaders/anim.frag");
    //m_l_system_shader = ShaderLoader::createShaderProgram(":/resources/shaders/default.vert", ":/resources/shaders/default.frag");
    m_skybox_shader = ShaderLoader::createShaderProgram(":/resources/shaders/skybox.vert", ":/resources/shaders/skybox.frag");
    m_particleShader = ShaderLoader::createShaderProgram(":/resources/shaders/particles.vert", ":/resources/shaders/particles.frag");

    for (int i = 0; i < 108; i++) {
        m_skybox_vbo[i] *= m_skybox_size;
    }

    buildGeometry();
    rebuildMeshes();
    sceneChanged();

    if (!m_renderdata.lights.empty()) {
        createShadowResources();
    }

    // postprocessing pipeline initialization
    m_postprocesses.push_back(std::make_unique<Colorgrade>(":/resources/images/greeny.png", 16, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio));
    //m_postprocesses.push_back(std::make_unique<Fog>(5.0f, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio));
    m_postprocesses.push_back(std::make_unique<Crepuscular>(
        size().width() * m_devicePixelRatio,         // width
        size().height() * m_devicePixelRatio,        // height
        &m_cam,                                      // Camera pointer
        &m_renderdata,                               // RenderData pointer
        &m_proj                                      // Projection matrix pointer
        ));
}

void Realtime::drawSkybox() {
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glUseProgram(m_skybox_shader);
    declareSkyboxUniforms();

    glBindVertexArray(m_skybox_vao_id);
    glActiveTexture(GL_TEXTURE15);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_skybox);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // CLEANUP
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);  // <-- ADD THIS LINE!

    glUseProgram(0);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void Realtime::updateLSystems() {
    QString output = generateLSystemString(m_rules, m_axiom, m_LSystemIterations);
    SceneNode *LSystem;
    LSystem = createLSystemNode(output);
    //m_LSystems.push_back(&LSystem);
    m_LSystemMetaData.shapes.clear();
    glm::mat4 identityMat(1.0f);
    SceneParser::parseRecursive(m_LSystemMetaData, LSystem, identityMat);
    m_LSystemScaler = 0.25;

    for (int i = 0; i < m_LSystemMetaData.shapes.size(); i++) {
        m_LSystemMetaData.shapes[i].ctm *= glm::translate(glm::vec3(0, 0, 7));
        m_LSystemMetaData.shapes[i].ctm *= glm::scale(m_LSystemScaler * glm::vec3(0.15, 1, 0.15));
    }

    m_LSystemIterationsChanged = false;
}

void Realtime::paintLSystems() {
    glUseProgram(m_shader);
    glBindVertexArray(m_vaoLcylinder);

    //In case you updated a buffer
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (updateToggle) {
        updateLSystems();
        updateToggle = true;
    }

    for (int i = 0; i < m_LSystemMetaData.shapes.size(); i++) {
        //Shininess and ctm uniforms depends on specific shape
        // Task 6: pass in m_model as a uniform into the shader program
        GLint modelLocation = glGetUniformLocation(m_shader, "model");
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &m_LSystemMetaData.shapes[i].ctm[0][0]);
        GLint shininessLocation = glGetUniformLocation(m_shader, "m_shininess");
        glUniform1f(shininessLocation, m_LSystemMetaData.shapes[i].primitive.material.shininess);
        //GLint animatingLocation = glGetUniformLocation(m_shader, "animating");
        //glUniform1i(animatingLocation, 0);
        glUniform1i(glGetUniformLocation(m_shader, "animating"), 0);

        // Shape-specific colours
        GLint cAmbientLocation = glGetUniformLocation(m_shader, "cAmbient");
        GLint cDiffuseLocation = glGetUniformLocation(m_shader, "cDiffuse");
        GLint cSpecularLocation = glGetUniformLocation(m_shader, "cSpecular");
        glm::vec4 ambient = m_LSystemMetaData.shapes[i].primitive.material.cAmbient;
        glm::vec4 diffuse = m_LSystemMetaData.shapes[i].primitive.material.cDiffuse;
        glm::vec4 specular = m_LSystemMetaData.shapes[i].primitive.material.cSpecular;
        glUniform4f(cAmbientLocation, ambient[0], ambient[1], ambient[2], ambient[3]);
        glUniform4f(cDiffuseLocation, diffuse[0], diffuse[1], diffuse[2], diffuse[3]);
        glUniform4f(cSpecularLocation, specular[0], specular[1], specular[2], specular[3]);

        // Draw Command
        //std::cout << "size of data" << m_LcylinderData.size() << std::endl;
        glDrawArrays(GL_TRIANGLES, 0, m_LcylinderData.size() / 6);

    }

    glUseProgram(0);
}

void Realtime::paintParticles() {
    //if (m_tesselationChanged) {
    if (true) {
        //m_particleVelocityGlobal = m_param1;
        m_particleVelocityGlobal = 5;
    }

    glUseProgram(m_particleShader);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(m_vaoParticles);

    // Task 7: pass in m_view and m_proj
    GLint viewLocation = glGetUniformLocation(m_particleShader, "viewMatrix");
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &m_cam.view[0][0]);
    GLint projLocation = glGetUniformLocation(m_particleShader, "projectionMatrix");
    glUniformMatrix4fv(projLocation, 1, GL_FALSE, &m_proj[0][0]);
    GLint particleCTMLocation = glGetUniformLocation(m_particleShader, "modelMatrix");
    glUniformMatrix4fv(particleCTMLocation, 1, GL_FALSE, &m_particleCtm[0][0]);

    particleUpdate();

    //glm::vec3 worldSpacePos = m_particleCtm * glm::vec4(m_particulePositionSizeData[0], m_particulePositionSizeData[1], m_particulePositionSizeData[2], 1);
    //glm::vec3 glPos = m_sceneCam.getProjectionMatrix() * m_sceneCam.getViewMatrix() * glm::vec4(worldSpacePos, 1);

    glBindBuffer(GL_ARRAY_BUFFER, m_vboParticlesPositionSize);
    glBufferData(GL_ARRAY_BUFFER, m_maxNumParticles * 4 * sizeof(GLfloat), m_particulePositionSizeData.data(), GL_STREAM_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_vboParticlesColor);
    glBufferData(GL_ARRAY_BUFFER, m_maxNumParticles * 4 * sizeof(GLfloat), m_particuleColorData.data(), GL_STREAM_DRAW);

    //glDrawArrays(GL_TRIANGLES, 0, m_particleVertexData.size() / 6);
    glDepthMask(GL_FALSE);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, m_numParticles);
    glDepthMask(GL_TRUE);
}

void Realtime::paintScene() {
    // 1. Render shadow maps FIRST
    //renderShadowMaps();

    // 2. Then render the main scene
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawSkybox();
    glUseProgram(m_shader);

    if (m_cameraPath != std::nullopt) {
        std::optional<PosRot> pathPosRot = m_cameraPath->get(getPathTime());
        if (pathPosRot == std::nullopt) {
            m_cameraPath = std::nullopt;
        }
        else {
            updateCameraFromPath(*pathPosRot);
            declareCameraUniforms();
        }
    }

    // Bind shadow maps to the shader
    //bindShadowMapsToShader(m_shader);

    GLuint vertices;
    int animating;
    bool usingTexture;

    // for each shape: bind vao, decl shape uniforms, draw, unbind, repeat
    for (RenderShapeData &shape: m_renderdata.shapes) {
        animating = 0;
        switch (shape.primitive.type) {
        case PrimitiveType::PRIMITIVE_CONE:
            vertices = m_cone->num_triangles;
            usingTexture = shape.primitive.material.textureMap.isUsed;
            glBindVertexArray(m_coneIds->shape_vao);
            break;
        case PrimitiveType::PRIMITIVE_CUBE:
            vertices = m_cube->num_triangles;
            usingTexture = shape.primitive.material.textureMap.isUsed;
            glBindVertexArray(m_cubeIds->shape_vao);
            break;
        case PrimitiveType::PRIMITIVE_CYLINDER:
            vertices = m_cylinder->num_triangles;
            usingTexture = shape.primitive.material.textureMap.isUsed;
            glBindVertexArray(m_cylinderIds->shape_vao);
            break;
        case PrimitiveType::PRIMITIVE_SPHERE:
            vertices = m_sphere->num_triangles;
            usingTexture = shape.primitive.material.textureMap.isUsed;
            glBindVertexArray(m_sphereIds->shape_vao);
            break;
        default:
            if (m_meshes.count(shape.primitive.meshfile) == 0) {
                vertices = 0;
            } else {
                vertices = m_meshes[shape.primitive.meshfile].num_triangles;
                glBindVertexArray(m_meshIds[shape.primitive.meshfile].shape_vao);
            }
            if (m_meshes[shape.primitive.meshfile].hasAnimation) animating = 1;
            usingTexture = m_meshes[shape.primitive.meshfile].hasTextures;
            break;
        }

        // TEXTURING
        glUniform1i(glGetUniformLocation(m_shader, "usingTexture"), usingTexture);
        if (usingTexture) {
            int texIndex = m_texIndexLUT[shape.primitive.material.textureMap.filename];
            glUniform1i(glGetUniformLocation(m_shader, "txtIndex"), texIndex); // depends on individual mesh

            glActiveTexture(GL_TEXTURE20 + texIndex);
            glBindTexture(GL_TEXTURE_2D, m_textures[texIndex]);
        }

        // GEOMETRY
        declSpecificUniforms(shape);
        checkGLError("After declSpecificUniforms");  // <-- ADD THIS

        // ANIMATION
        glUniform1i(glGetUniformLocation(m_shader, "animating"), animating);
        if (animating) {
            int num = m_meshes[shape.primitive.meshfile].m_meshAnim.m_finalBoneMatrices.size();
            float finalMatrices[num*16];
            for (int i = 0; i < num; i++) {
                for (int j = 0; j < 16; j++) {
                    finalMatrices[16*i + j] = m_meshes[shape.primitive.meshfile].m_meshAnim.m_finalBoneMatrices[i][j/4][j%4];
                }
            }
            glUniform1i(glGetUniformLocation(m_shader, "numBones"), num);
            glUniformMatrix4fv(glGetUniformLocation(m_shader, "finalBoneMatrices"), num, GL_FALSE, finalMatrices);
        }

        // DRAWING
        glDrawArrays(GL_TRIANGLES, 0, vertices);

        // UNBINDING
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
    }

    paintLSystems();
    paintParticles();

    glUseProgram(0);
}

void Realtime::paintGL() {
    if (m_postprocesses.size() == 0) {
        paintScene();
        return;
    }

    glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);
    glBindFramebuffer(GL_FRAMEBUFFER, m_postprocesses[0]->getFramebuffer());
    paintScene();

    // Outputs contents of each framebuffer into the next post-process as input
    for (int i = 1; i < m_postprocesses.size(); i++) {
        // glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);
        glBindFramebuffer(GL_FRAMEBUFFER, m_postprocesses[i]->getFramebuffer());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        m_postprocesses[i-1]->paintTexture();
    }

    // Draws contents of final post-process
    // glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_postprocesses[m_postprocesses.size()-1]->paintTexture();
}

void Realtime::resizeGL(int w, int h) {
    // Tells OpenGL how big the screen is
    glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);
    for (int i = 0; i < m_postprocesses.size(); i++) {
        m_postprocesses[i]->destroyFBO();
        m_postprocesses[i]->updateRes(size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);
        m_postprocesses[i]->makeFBO();
    }

    // Students: anything requiring OpenGL calls when the program starts should be done here
}

void Realtime::sceneChanged() {
    makeCurrent();
    std::string filepath = "scenefiles/realtime/extra_credit/finalscene.json";
    SceneParser::parse(filepath, m_renderdata);

    initializeTextures(filepath);
    rebuildCamera();
    rebuildMatrices();
    rebuildMeshes();

    // Recreate shadow resources for new scene
    // if (!m_renderdata.lights.empty()) {
    //     createShadowResources();
    // }

    // Update crepuscular rays if active
    for (auto& pp : m_postprocesses) {
        if (auto* crep = dynamic_cast<Crepuscular*>(pp.get())) {
            crep->updateCameraAndScene(&m_cam, &m_renderdata, &m_proj);
        }
    }

    glUseProgram(m_shader);
    declareCameraUniforms();
    declGeneralUniforms();
    glUseProgram(0);
    update();
}

void Realtime::settingsChanged() {
    makeCurrent();
    if (m_sphereIds->shape_vao != 0 && m_sphereIds->shape_vbo != 0) {
        m_sphere->updateParams(settings.shapeParameter1, settings.shapeParameter2);
        setupPrimitives(m_sphereIds, m_sphere->generateShape());
    }

    if (m_cubeIds->shape_vao != 0 && m_cubeIds->shape_vbo != 0) {
        m_cube->updateParams(settings.shapeParameter1);
        setupPrimitives(m_cubeIds, m_cube->generateShape());
    }

    if (m_coneIds->shape_vao != 0 && m_coneIds->shape_vbo != 0) {
        m_cone->updateParams(settings.shapeParameter1, settings.shapeParameter2);
        setupPrimitives(m_coneIds, m_cone->generateShape(), false, true);
    }

    if (m_cylinderIds->shape_vao != 0 && m_cylinderIds->shape_vbo != 0) {
        m_cylinder->updateParams(settings.shapeParameter1, settings.shapeParameter2);
        setupPrimitives(m_cylinderIds, m_cylinder->generateShape(), false, true);
    }

    if (m_shader != 0 && (settings.nearPlane != near || settings.farPlane != far)) {
        rebuildMatrices();
        glUseProgram(m_shader);
        declareCameraUniforms(); // maybe not needed
        glUseProgram(0);
    }
    update(); // asks for a PaintGL() call to occur
    // if nearPlane or farPlane changed update camera settings
}

// ============================================================================
// SHADOW MAPPING HELPER FUNCTIONS
// ============================================================================

static LightSpace makeOrthoForDirectional(const SceneLightData &L)
{
    LightSpace ls;

    glm::vec3 center(0.0f, 0.0f, 0.0f);
    glm::vec3 lightDir = glm::normalize(glm::vec3(L.dir));
    float distance = 100.0f;
    glm::vec3 pos = center - lightDir * distance;

    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    if (fabs(glm::dot(up, lightDir)) > 0.99f) {
        up = glm::vec3(1.0f, 0.0f, 0.0f);
    }

    ls.view = glm::lookAt(pos, center, up);

    float orthoSize = 5.0f;
    float nearP = 1.0f;
    float farP = distance + 100.0f;
    ls.proj = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearP, farP);

    return ls;
}

static LightSpace makePerspectiveForSpot(const SceneLightData &L)
{
    LightSpace ls;

    glm::vec3 pos = glm::vec3(L.pos);
    glm::vec3 dir = glm::normalize(glm::vec3(L.dir));

    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    if (fabs(glm::dot(up, dir)) > 0.99f) up = glm::vec3(1.0f, 0.0f, 0.0f);

    ls.view = glm::lookAt(pos, pos + dir, up);

    float angle = L.angle;
    if (angle > 3.14159265f) {
        angle = glm::radians(angle);
    }
    float fovy = glm::clamp(2.0f * angle, 0.01f, glm::radians(170.0f));

    float aspect = 1.0f;
    float nearP = 0.1f;
    float farP = 100.0f;

    ls.proj = glm::perspective(fovy, aspect, nearP, farP);

    return ls;
}

static CubeLightSpace makeCubeFaceMatrices(const SceneLightData &L)
{
    CubeLightSpace cs;

    glm::vec3 pos = glm::vec3(L.pos);

    std::array<glm::vec3,6> looks = {
        glm::vec3( 1, 0, 0),
        glm::vec3(-1, 0, 0),
        glm::vec3( 0, 1, 0),
        glm::vec3( 0,-1, 0),
        glm::vec3( 0, 0, 1),
        glm::vec3( 0, 0,-1)
    };

    std::array<glm::vec3,6> ups = {
        glm::vec3(0,-1,0),
        glm::vec3(0,-1,0),
        glm::vec3(0,0,1),
        glm::vec3(0,0,-1),
        glm::vec3(0,-1,0),
        glm::vec3(0,-1,0)
    };

    float nearP = 0.1f;
    float farP  = 100.0f;
    cs.proj = glm::perspective(glm::radians(90.0f), 1.0f, nearP, farP);

    for (int i = 0; i < 6; ++i) {
        cs.views[i] = glm::lookAt(pos, pos + looks[i], ups[i]);
    }

    return cs;
}

// ============================================================================
// SHADOW RESOURCE MANAGEMENT
// ============================================================================

void Realtime::createShadowResources() {
    checkGLError("START of createShadowResources");

    deleteShadowResources();
    checkGLError("After deleteShadowResources");

    int count = std::min<int>((int)m_renderdata.lights.size(), MAX_SHADOW_CASTERS);
    m_shadowMaps.resize(count);

    std::cout << "Creating shadow resources for " << count << " lights" << std::endl;

    for (int i = 0; i < count; ++i) {
        std::cout << "Creating shadow map " << i << std::endl;
        checkGLError("Start of loop iteration");

        const SceneLightData &L = m_renderdata.lights[i];
        ShadowMap sm;
        sm.lightIndex = i;

        if (L.type == LightType::LIGHT_DIRECTIONAL) {
            std::cout << "  Light " << i << " is DIRECTIONAL" << std::endl;
            sm.type = ShadowType::Directional;
            sm.enabled = true;
            sm.size = DEFAULT_SHADOW_SIZE;

            glGenTextures(1, &sm.depthTex);
            checkGLError("After glGenTextures");

            glBindTexture(GL_TEXTURE_2D, sm.depthTex);
            checkGLError("After glBindTexture");

            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, sm.size, sm.size, 0,
                         GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
            checkGLError("After glTexImage2D");

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            checkGLError("After texture parameters");

            float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
            checkGLError("After border color");

            glGenFramebuffers(1, &sm.fbo);
            checkGLError("After glGenFramebuffers");

            // **BIND AND CONFIGURE THE FRAMEBUFFER**
            glBindFramebuffer(GL_FRAMEBUFFER, sm.fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, sm.depthTex, 0);

            // **CRITICAL: Tell OpenGL we're not rendering any color data**
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);

            // **Check framebuffer completeness**
            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE) {
                std::cerr << "Framebuffer incomplete for light " << i << ": " << status << std::endl;
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            checkGLError("After framebuffer setup");

        } else if (L.type == LightType::LIGHT_SPOT) {
            std::cout << "  Light " << i << " is SPOT" << std::endl;
            // Same checks for spot...

        } else if (L.type == LightType::LIGHT_POINT) {
            std::cout << "  Light " << i << " is POINT" << std::endl;
            sm.type = ShadowType::Point;
            sm.enabled = true;

            glGenTextures(1, &sm.depthTex);
            checkGLError("After glGenTextures (point)");

            glBindTexture(GL_TEXTURE_CUBE_MAP, sm.depthTex);
            checkGLError("After glBindTexture CUBE_MAP");

            for (unsigned int face = 0; face < 6; ++face) {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_R32F,
                             sm.size, sm.size, 0, GL_RED, GL_FLOAT, NULL);
                checkGLError("After glTexImage2D for cube face");
            }

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            checkGLError("After cube map texture parameters");

            glGenFramebuffers(1, &sm.fbo);
            checkGLError("After glGenFramebuffers (point)");

            GLuint rboDepth = 0;
            glGenRenderbuffers(1, &rboDepth);
            checkGLError("After glGenRenderbuffers");

            glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
            checkGLError("After glBindRenderbuffer");

            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, sm.size, sm.size);
            checkGLError("After glRenderbufferStorage");

            sm.rbo = rboDepth;

        } else {
            std::cout << "  Light " << i << " is NONE/UNKNOWN" << std::endl;
            sm.type = ShadowType::None;
            sm.enabled = false;
        }

        m_shadowMaps[i] = sm;
        checkGLError("After storing shadow map");
    }

    // Load depth shaders
    std::cout << "Loading shadow shaders..." << std::endl;
    checkGLError("Before loading depth shader");

    m_depthShader = ShaderLoader::createShaderProgram(
        ":/resources/shaders/shadow_depth.vert",
        ":/resources/shaders/shadow_depth.frag"
        );
    checkGLError("After loading depth shader");

    std::cout << "Depth shader ID: " << m_depthShader << std::endl;

    m_depthPointShader = ShaderLoader::createShaderProgram(
        ":/resources/shaders/shadow_depth_point.vert",
        ":/resources/shaders/shadow_depth_point.frag"
        );
    checkGLError("After loading depth point shader");

    std::cout << "Depth point shader ID: " << m_depthPointShader << std::endl;

    checkGLError("END of createShadowResources");
}

void Realtime::deleteShadowResources() {
    for (auto &sm : m_shadowMaps) {
        if (sm.fbo) {
            glDeleteFramebuffers(1, &sm.fbo);
            sm.fbo = 0;
        }
        if (sm.depthTex) {
            glDeleteTextures(1, &sm.depthTex);
            sm.depthTex = 0;
        }
        if (sm.rbo) {
            glDeleteRenderbuffers(1, &sm.rbo);
            sm.rbo = 0;
        }
    }
    m_shadowMaps.clear();

    if (m_depthShader) {
        glDeleteProgram(m_depthShader);
        m_depthShader = 0;
    }
    if (m_depthPointShader) {
        glDeleteProgram(m_depthPointShader);
        m_depthPointShader = 0;
    }
}

// ============================================================================
// SHADOW MAP RENDERING
// ============================================================================

void Realtime::renderShadowMaps() {
    if (m_shadowMaps.empty()) return;
    if (!m_depthShader) return;

    // checkGLError("START of renderShadowMaps");

    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    glUseProgram(m_depthShader);
    // checkGLError("After glUseProgram(m_depthShader)");

    for (size_t si = 0; si < m_shadowMaps.size(); ++si) {
        ShadowMap &sm = m_shadowMaps[si];
        if (!sm.enabled || sm.type == ShadowType::None) continue;

        // std::cout << "Processing shadow map " << si << " type: " << (int)sm.type << std::endl;
        // checkGLError("Start of shadow map iteration");

        const SceneLightData &L = m_renderdata.lights[sm.lightIndex];

        // Directional and Spot lights (2D depth maps)
        if (sm.type == ShadowType::Directional || sm.type == ShadowType::Spot) {
            glBindFramebuffer(GL_FRAMEBUFFER, sm.fbo);
            // checkGLError("After glBindFramebuffer");

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                   GL_TEXTURE_2D, sm.depthTex, 0);
            // checkGLError("After glFramebufferTexture2D");

            // glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
            // checkGLError("After glDrawBuffer(GL_NONE)");

            glViewport(0, 0, sm.size, sm.size);
            // checkGLError("After glViewport");

            glClear(GL_DEPTH_BUFFER_BIT);
            // checkGLError("After glClear");

            LightSpace LS;
            if (sm.type == ShadowType::Directional) {
                LS = makeOrthoForDirectional(L);
            } else {
                LS = makePerspectiveForSpot(L);
            }

            glm::mat4 lightSpace = LS.proj * LS.view;
            sm.lightSpaceMatrix = lightSpace;

            GLint locModel = glGetUniformLocation(m_depthShader, "model");
            GLint locLightSpace = glGetUniformLocation(m_depthShader, "lightSpace");

            if (locLightSpace != -1)
                glUniformMatrix4fv(locLightSpace, 1, GL_FALSE, glm::value_ptr(lightSpace));

            // Render each shape
            for (const RenderShapeData &shape : m_renderdata.shapes) {
                if (locModel != -1)
                    glUniformMatrix4fv(locModel, 1, GL_FALSE, &shape.ctm[0][0]);

                // Bind appropriate VAO
                switch (shape.primitive.type) {
                case PrimitiveType::PRIMITIVE_CONE:
                    glBindVertexArray(m_coneIds->shape_vao);
                    glDrawArrays(GL_TRIANGLES, 0, m_cone->num_triangles);
                    break;
                case PrimitiveType::PRIMITIVE_CUBE:
                    glBindVertexArray(m_cubeIds->shape_vao);
                    glDrawArrays(GL_TRIANGLES, 0, m_cube->num_triangles);
                    break;
                case PrimitiveType::PRIMITIVE_CYLINDER:
                    glBindVertexArray(m_cylinderIds->shape_vao);
                    glDrawArrays(GL_TRIANGLES, 0, m_cylinder->num_triangles);
                    break;
                case PrimitiveType::PRIMITIVE_SPHERE:
                    glBindVertexArray(m_sphereIds->shape_vao);
                    glDrawArrays(GL_TRIANGLES, 0, m_sphere->num_triangles);
                    break;
                default:
                    if (m_meshes.count(shape.primitive.meshfile) > 0) {
                        glBindVertexArray(m_meshIds[shape.primitive.meshfile].shape_vao);
                        glDrawArrays(GL_TRIANGLES, 0, m_meshes[shape.primitive.meshfile].num_triangles);
                    }
                    break;
                }
            }

            glBindVertexArray(0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        // Point lights (cubemap)
        else if (sm.type == ShadowType::Point) {
            glUseProgram(m_depthPointShader);

            float nearp = 0.1f;
            float farp = m_pointLightFar;

            CubeLightSpace CS = makeCubeFaceMatrices(L);

            GLint locModel = glGetUniformLocation(m_depthPointShader, "model");
            GLint locView = glGetUniformLocation(m_depthPointShader, "view");
            GLint locProj = glGetUniformLocation(m_depthPointShader, "proj");
            GLint locLightPos = glGetUniformLocation(m_depthPointShader, "lightPos");
            GLint locFarPlane = glGetUniformLocation(m_depthPointShader, "farPlane");

            glm::vec3 lightPos = glm::vec3(L.pos);
            if (locLightPos != -1) glUniform3f(locLightPos, lightPos.x, lightPos.y, lightPos.z);
            if (locFarPlane != -1) glUniform1f(locFarPlane, farp);

            glBindFramebuffer(GL_FRAMEBUFFER, sm.fbo);
            glViewport(0, 0, sm.size, sm.size);

            for (int face = 0; face < 6; ++face) {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                       GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, sm.depthTex, 0);
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                          GL_RENDERBUFFER, sm.rbo);
                glDrawBuffer(GL_COLOR_ATTACHMENT0);

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glm::mat4 viewMat = CS.views[face];
                glm::mat4 projMat = CS.proj;

                if (locView != -1) glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(viewMat));
                if (locProj != -1) glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projMat));

                for (const RenderShapeData &shape : m_renderdata.shapes) {
                    if (locModel != -1) glUniformMatrix4fv(locModel, 1, GL_FALSE, &shape.ctm[0][0]);

                    switch (shape.primitive.type) {
                    case PrimitiveType::PRIMITIVE_CONE:
                        glBindVertexArray(m_coneIds->shape_vao);
                        glDrawArrays(GL_TRIANGLES, 0, m_cone->num_triangles);
                        break;
                    case PrimitiveType::PRIMITIVE_CUBE:
                        glBindVertexArray(m_cubeIds->shape_vao);
                        glDrawArrays(GL_TRIANGLES, 0, m_cube->num_triangles);
                        break;
                    case PrimitiveType::PRIMITIVE_CYLINDER:
                        glBindVertexArray(m_cylinderIds->shape_vao);
                        glDrawArrays(GL_TRIANGLES, 0, m_cylinder->num_triangles);
                        break;
                    case PrimitiveType::PRIMITIVE_SPHERE:
                        glBindVertexArray(m_sphereIds->shape_vao);
                        glDrawArrays(GL_TRIANGLES, 0, m_sphere->num_triangles);
                        break;
                    default:
                        if (m_meshes.count(shape.primitive.meshfile) > 0) {
                            glBindVertexArray(m_meshIds[shape.primitive.meshfile].shape_vao);
                            glDrawArrays(GL_TRIANGLES, 0, m_meshes[shape.primitive.meshfile].num_triangles);
                        }
                        break;
                    }
                }
            }

            glBindVertexArray(0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glUseProgram(0);
        }
    }

    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);  // CRITICAL!
    glDrawBuffer(GL_BACK);   // or GL_COLOR_ATTACHMENT0 if using FBOs
    glReadBuffer(GL_BACK);
    glUseProgram(0);
    // checkGLError("END of renderShadowMaps");

}

// ============================================================================
// BIND SHADOW MAPS TO SHADER
// ============================================================================

void Realtime::bindShadowMapsToShader(GLuint shader) {
    std::cout << "=== Binding shadow maps to shader " << shader << " ===" << std::endl;
    checkGLError("START of bindShadowMapsToShader");

    int baseUnit = 8;
    int count = (int)m_shadowMaps.size();

    std::cout << "Number of shadow maps: " << count << std::endl;

    // Initialize all shadow types to 0 (no shadow)
    for (int i = 0; i < MAX_SHADOW_CASTERS; ++i) {
        std::string stName = "shadowType[" + std::to_string(i) + "]";
        GLint stLoc = glGetUniformLocation(shader, stName.c_str());
        std::cout << "  shadowType[" << i << "] location: " << stLoc << std::endl;
        if (stLoc != -1) {
            glUniform1i(stLoc, 0);
            checkGLError(("After setting shadowType[" + std::to_string(i) + "]").c_str());
        }
    }

    GLint locShadowBias = glGetUniformLocation(shader, "shadowBias");
    GLint locEnablePCF = glGetUniformLocation(shader, "enablePCF");
    GLint locPointFarPlane = glGetUniformLocation(shader, "pointLightFar");

    std::cout << "  shadowBias loc: " << locShadowBias << std::endl;
    std::cout << "  enablePCF loc: " << locEnablePCF << std::endl;
    std::cout << "  pointLightFar loc: " << locPointFarPlane << std::endl;

    if (locShadowBias != -1) {
        glUniform1f(locShadowBias, m_shadowBias);
        checkGLError("After shadowBias");
    }
    if (locEnablePCF != -1) {
        glUniform1i(locEnablePCF, m_enablePCF ? 1 : 0);
        checkGLError("After enablePCF");
    }
    if (locPointFarPlane != -1) {
        glUniform1f(locPointFarPlane, m_pointLightFar);
        checkGLError("After pointFarPlane");
    }

    for (int i = 0; i < count; ++i) {
        const ShadowMap &sm = m_shadowMaps[i];

        std::cout << "Processing shadow map " << i << ":" << std::endl;
        std::cout << "  enabled: " << sm.enabled << ", type: " << (int)sm.type << std::endl;

        int typeInt = 0;
        if (!sm.enabled) typeInt = 0;
        else if (sm.type == ShadowType::Directional) typeInt = 1;
        else if (sm.type == ShadowType::Spot) typeInt = 2;
        else if (sm.type == ShadowType::Point) typeInt = 3;

        std::string stName = "shadowType[" + std::to_string(i) + "]";
        GLint stLoc = glGetUniformLocation(shader, stName.c_str());
        if (stLoc != -1) {
            glUniform1i(stLoc, typeInt);
            checkGLError(("After shadowType[" + std::to_string(i) + "]").c_str());
        }

        std::string lsName = "lightSpace[" + std::to_string(i) + "]";
        GLint lsLoc = glGetUniformLocation(shader, lsName.c_str());
        std::cout << "  lightSpace[" << i << "] loc: " << lsLoc << std::endl;
        if (lsLoc != -1) {
            glUniformMatrix4fv(lsLoc, 1, GL_FALSE, glm::value_ptr(sm.lightSpaceMatrix));
            checkGLError(("After lightSpace[" + std::to_string(i) + "]").c_str());
        }

        if (!sm.enabled) continue;

        if (sm.type == ShadowType::Directional || sm.type == ShadowType::Spot) {
            std::cout << "  Binding 2D shadow map to unit " << (baseUnit + i) << std::endl;

            glActiveTexture(GL_TEXTURE0 + baseUnit + i);
            checkGLError(("After glActiveTexture for shadow " + std::to_string(i)).c_str());

            glBindTexture(GL_TEXTURE_2D, sm.depthTex);
            checkGLError(("After glBindTexture 2D for shadow " + std::to_string(i)).c_str());

            std::string sampName = "shadowMap[" + std::to_string(i) + "]";
            GLint sampLoc = glGetUniformLocation(shader, sampName.c_str());
            std::cout << "  shadowMap[" << i << "] loc: " << sampLoc << std::endl;
            if (sampLoc != -1) {
                glUniform1i(sampLoc, baseUnit + i);
                checkGLError(("After shadowMap sampler " + std::to_string(i)).c_str());
            }
        } else if (sm.type == ShadowType::Point) {
            std::cout << "  Binding cubemap shadow to unit " << (baseUnit + i) << std::endl;

            glActiveTexture(GL_TEXTURE0 + baseUnit + i);
            checkGLError(("After glActiveTexture for cubemap " + std::to_string(i)).c_str());

            glBindTexture(GL_TEXTURE_CUBE_MAP, sm.depthTex);
            checkGLError(("After glBindTexture CUBE_MAP for shadow " + std::to_string(i)).c_str());

            std::string sampName = "shadowCube[" + std::to_string(i) + "]";
            GLint sampLoc = glGetUniformLocation(shader, sampName.c_str());
            std::cout << "  shadowCube[" << i << "] loc: " << sampLoc << std::endl;
            if (sampLoc != -1) {
                glUniform1i(sampLoc, baseUnit + i);
                checkGLError(("After shadowCube sampler " + std::to_string(i)).c_str());
            }
        }
    }

    glActiveTexture(GL_TEXTURE0);
    checkGLError("After resetting to GL_TEXTURE0");

    std::cout << "=== END bindShadowMapsToShader ===" << std::endl;
}

// ================== Camera Paths!
void Realtime::activateCameraPath(CameraPath cameraPath) {
    m_cameraPath = cameraPath;
    m_pathStartTime = std::chrono::high_resolution_clock::now();
}

float Realtime::getPathTime() {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> elapsed = now - m_pathStartTime;;
    return elapsed.count();
}

void Realtime::updateCameraFromPath(PosRot posRot) {
    m_cam.pos = glm::vec4(posRot.pos, 1);
    glm::mat4 trans = {glm::vec4(1.f, 0.f, 0.f, 0.f),
                       glm::vec4(0.f, 1.f, 0.f, 0.f),
                       glm::vec4(0.f, 0.f, 1.f, 0.f),
                       glm::vec4(-glm::vec3(m_cam.pos), 1.f)};
    glm::mat4 inv_trans = {glm::vec4(1.f, 0.f, 0.f, 0.f),
                           glm::vec4(0.f, 1.f, 0.f, 0.f),
                           glm::vec4(0.f, 0.f, 1.f, 0.f),
                           glm::vec4(glm::vec3(m_cam.pos), 1.f)};

    glm::mat4 inv_rot = glm::toMat4(posRot.rot);
    glm::mat4 rot = glm::inverse(inv_rot);

    m_cam.view = rot * trans;
    m_cam.viewInv = inv_trans * inv_rot;
}

// ================== Camera Movement!

void Realtime::keyPressEvent(QKeyEvent *event) {
    m_keyMap[Qt::Key(event->key())] = true;
}

void Realtime::keyReleaseEvent(QKeyEvent *event) {
    m_keyMap[Qt::Key(event->key())] = false;
}

void Realtime::mousePressEvent(QMouseEvent *event) {
    if (event->buttons().testFlag(Qt::LeftButton)) {
        m_mouseDown = true;
        m_prev_mouse_pos = glm::vec2(event->position().x(), event->position().y());
    }
}

void Realtime::mouseReleaseEvent(QMouseEvent *event) {
    if (!event->buttons().testFlag(Qt::LeftButton)) {
        m_mouseDown = false;
    }
}

glm::mat4 Realtime::rotationhelper(glm::vec4 u, float angle) {
    return glm::mat4(
        (cos(angle) + u.x*u.x*(1-cos(angle))), (u.x*u.y*(1-cos(angle))+u.z*sin(angle)), (u.x*u.z*(1-cos(angle))-u.y*sin(angle)), 0,
        (u.x*u.y*(1-cos(angle))-u.z*sin(angle)), (cos(angle) + u.y*u.y*(1-cos(angle))), (u.z*u.y*(1-cos(angle))+u.x*sin(angle)), 0,
        (u.x*u.z*(1-cos(angle))+u.y*sin(angle)), (u.z*u.y*(1-cos(angle))-u.x*sin(angle)), (cos(angle) + u.z*u.z*(1-cos(angle))), 0,
        0, 0, 0, 1
        );
}

void Realtime::mouseMoveEvent(QMouseEvent *event) {
    if (m_mouseDown) {
        int posX = event->position().x();
        int posY = event->position().y();
        int deltaX = posX - m_prev_mouse_pos.x;
        int deltaY = posY - m_prev_mouse_pos.y;

        m_prev_mouse_pos = glm::vec2(posX, posY);


        bool updated = false;
        float pointing;
        glm::mat4 rotation = glm::mat4(1), rot, trans;
        glm::vec3 w, v, u;
        // Use deltaX and deltaY here to rotate
        if (deltaX != 0) {
            glm::vec4 moveX = glm::vec4(0, 1, 0, 0);
            updated = true;
            rotation = rotationhelper(moveX, (deltaX)*(M_PI/1024.f)); // /(float)size().width()
            m_cam.look = rotation * m_cam.look;
            m_cam.up = rotation * m_cam.up;
            // m_cam.look = rotation * m_cam.look;
            // rotate look and up or wtv

            w = -glm::normalize(glm::vec3(m_cam.look));
            v = glm::normalize(glm::vec3(m_cam.up) - (glm::dot(glm::vec3(m_cam.up), w) * w));
            u = glm::cross(v, w);
            rot = glm::mat4(u.x, v.x, w.x, 0, u.y, v.y, w.y, 0, u.z, v.z, w.z, 0, 0, 0, 0, 1);
            trans = glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -m_cam.pos.x, -m_cam.pos.y, -m_cam.pos.z, 1);
            m_cam.view = rot * trans;
            m_cam.viewInv = glm::inverse(m_cam.view);
        }
        if (deltaY != 0) {
            glm::vec4 moveY = glm::normalize(glm::vec4(glm::cross(glm::vec3(m_cam.view * m_cam.look), glm::vec3(m_cam.view * m_cam.up)), 0)); // axis for y distance travelled
            rotation = rotationhelper(moveY, (deltaY)*(M_PI/1024.f)); // /(float)size().height()
            m_cam.look = m_cam.viewInv * rotation * m_cam.view * m_cam.look;
            m_cam.up = m_cam.viewInv * rotation * m_cam.view * m_cam.up;
            updated = true;

            w = -glm::normalize(glm::vec3(m_cam.look));
            v = glm::normalize(glm::vec3(m_cam.up) - (glm::dot(glm::vec3(m_cam.up), w) * w));
            u = glm::cross(v, w);
            rot = glm::mat4(u.x, v.x, w.x, 0, u.y, v.y, w.y, 0, u.z, v.z, w.z, 0, 0, 0, 0, 1);
            trans = glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -m_cam.pos.x, -m_cam.pos.y, -m_cam.pos.z, 1);
            m_cam.view = rot * trans;
            m_cam.viewInv = glm::inverse(m_cam.view);
        }


        if (updated) {
            glUseProgram(m_shader);
            declareCameraUniforms();
            glUseProgram(0);
        }

        update(); // asks for a PaintGL() call to occur
    }
}

void Realtime::timerEvent(QTimerEvent *event) {
    /*
        W: Translates the camera in the direction of the look vector
        S: Translates the camera in the opposite direction of the look vector
        A: Translates the camera in the left direction, perpendicular to the look and up vectors
        D: Translates the camera in the right direction, also perpendicular to the look and up vectors. This movement should be opposite to that of pressing A
        Space: Translates the camera along the world space vector 0,1,0
        Ctrl: Translates the camera along the world space vector 0,-1,0

        m_cam = Camera{viewMatrix, viewInv, glm::vec4{pos.x, pos.y, pos.z, 1}, m_renderdata.cameraData.look, m_renderdata.cameraData.up,
                     m_renderdata.cameraData.heightAngle, m_renderdata.cameraData.aperture, m_renderdata.cameraData.focalLength};

        m_keyMap comes equipped to handle the keys: Qt::Key_W, Qt::Key_A, Qt::Key_S, Qt::Key_D, Qt::Key_Space, and Qt::Key_Control.
    */
    int elapsedms   = m_elapsedTimer.elapsed();
    float deltaTime = elapsedms * 0.001f;
    m_elapsedTimer.restart();

    for (auto &[key, meshval]: m_meshes) {
        if (meshval.hasAnimation) {
            meshval.updateFinalBoneMatrices(deltaTime + meshval.m_meshAnim.m_currentTime);
            meshval.m_meshAnim.m_currentTime += deltaTime;

            if (meshval.m_meshAnim.m_currentTime >= meshval.m_meshAnim.m_animation.m_duration) {
                meshval.m_meshAnim.m_currentTime = 0.0;
            }
        }
    }

    bool updatedoccurred = false;
    // Use deltaTime and m_keyMap here to move around
    if (m_keyMap[Qt::Key_W]) {
        m_cam.pos += (5 * deltaTime * glm::normalize(m_cam.look));
        updatedoccurred = true;
    } else if (m_keyMap[Qt::Key_S]) {
        m_cam.pos -= (5 * deltaTime * glm::normalize(m_cam.look));
        updatedoccurred = true;
    }

    if (m_keyMap[Qt::Key_A]) {
        glm::vec4 move = glm::vec4(glm::normalize(glm::cross(glm::vec3(m_cam.look), glm::vec3(m_cam.up))), 0);
        m_cam.pos -= (5 * deltaTime * move);
        updatedoccurred = true;
    } else if (m_keyMap[Qt::Key_D]) {
        glm::vec4 move = glm::vec4(glm::normalize(glm::cross(glm::vec3(m_cam.look), glm::vec3(m_cam.up))), 0);
        m_cam.pos += (5 * deltaTime * move);
        updatedoccurred = true;
    }

    if (m_keyMap[Qt::Key_Space]) {
        glm::vec4 translate = glm::vec4(0, 1, 0, 0); // m viw
        m_cam.pos += (5 * deltaTime * translate);
        updatedoccurred = true;
    } else if (m_keyMap[Qt::Key_Control]) {
        glm::vec4 translate = glm::vec4(0, 1, 0, 0);
        m_cam.pos -= (5 * deltaTime * translate);
        updatedoccurred = true;
    }

    if (updatedoccurred) {
        glm::vec3 w = -glm::normalize(glm::vec3(m_cam.look));
        glm::vec3 v = glm::normalize(glm::vec3(m_cam.up) - (glm::dot(glm::vec3(m_cam.up), w) * w));
        glm::vec3 u = glm::cross(v, w);

        glm::mat4 rot = glm::mat4(u.x, v.x, w.x, 0, u.y, v.y, w.y, 0, u.z, v.z, w.z, 0, 0, 0, 0, 1);
        glm::mat4 trans = glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -m_cam.pos.x, -m_cam.pos.y, -m_cam.pos.z, 1);
        m_cam.view = rot * trans;
        m_cam.viewInv = glm::inverse(m_cam.view);

        glUseProgram(m_shader);
        declareCameraUniforms();
        glUseProgram(0);
    }
    update(); // asks for a PaintGL() call to occur
}

// DO NOT EDIT
void Realtime::saveViewportImage(std::string filePath) {
    // Make sure we have the right context and everything has been drawn
    makeCurrent();

    // int fixedWidth = 1024;
    // int fixedHeight = 768;

    // CORRECT [REPLACE WITH THIS]
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int fixedWidth = viewport[2];
    int fixedHeight = viewport[3];

    // Create Frame Buffer
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Create a color attachment texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fixedWidth, fixedHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    // Optional: Create a depth buffer if your rendering uses depth testing
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, fixedWidth, fixedHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Error: Framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    // Render to the FBO
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, fixedWidth, fixedHeight);

    // Clear and render your scene here
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    paintGL();

    // Read pixels from framebuffer
    std::vector<unsigned char> pixels(fixedWidth * fixedHeight * 3);
    glReadPixels(0, 0, fixedWidth, fixedHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // Unbind the framebuffer to return to default rendering to the screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Convert to QImage
    QImage image(pixels.data(), fixedWidth, fixedHeight, QImage::Format_RGB888);
    QImage flippedImage = image.mirrored(); // Flip the image vertically

    // Save to file using Qt
    QString qFilePath = QString::fromStdString(filePath);
    if (!flippedImage.save(qFilePath)) {
        std::cerr << "Failed to save image to " << filePath << std::endl;
    }

    // Clean up
    glDeleteTextures(1, &texture);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &fbo);
}
