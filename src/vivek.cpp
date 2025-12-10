#include "realtime.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <iostream>
#include "settings.h"
#include "utils/sceneparser.h"
#include "utils/shaderloader.h"
#include "utils/light.h"
#include "utils/shapebuilder.h"
#include "utils/uniforms.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

static const float MOVE_SPEED = 5.f;
static const float ROTATE_SPEED = 0.005f;

struct LightSpace {
    glm::mat4 view;
    glm::mat4 proj;
};

struct CubeLightSpace {
    glm::mat4 views[6];
    glm::mat4 proj;
};

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
    for (const GPUShape &gpu : m_gpuShapes) {
        if (gpu.vbo) glDeleteBuffers(1, &gpu.vbo);
        if (gpu.vao) glDeleteVertexArrays(1, &gpu.vao);
    }
    m_gpuShapes.clear();

    deletePostResources();
    deleteShadowResources();

    this->doneCurrent();
}

void Realtime::deletePostResources() {
    if (m_fbo) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
    if (m_colorTex) {
        glDeleteTextures(1, &m_colorTex);
        m_colorTex = 0;
    }
    if (m_depthTex) {
        glDeleteTextures(1, &m_depthTex);
        m_depthTex = 0;
    }
    if (m_quadVBO) {
        glDeleteBuffers(1, &m_quadVBO);
        m_quadVBO = 0;
    }
    if (m_quadVAO) {
        glDeleteVertexArrays(1, &m_quadVAO);
        m_quadVAO = 0;
    }
    if (m_postShader) {
        glDeleteProgram(m_postShader);
        m_postShader = 0;
    }
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
    }
    m_shadowMaps.clear();

    if (m_depthShader) {
        glDeleteProgram(m_depthShader);
        m_depthShader = 0;
    }
    if (m_debugDepthShader) {
        glDeleteProgram(m_debugDepthShader);
        m_debugDepthShader = 0;
    }
}

void Realtime::createPostResources(int width, int height) {
    // delete previous if any
    deletePostResources();

    // Create color texture
    glGenTextures(1, &m_colorTex);
    glBindTexture(GL_TEXTURE_2D, m_colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Create depth texture
    glGenTextures(1, &m_depthTex);
    glBindTexture(GL_TEXTURE_2D, m_depthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Create FBO
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTex, 0);

    GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Error creating post-process framebuffer!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Fullscreen quad (pos.xy, uv.xy) - two triangles
    float quad[] = {
        // pos      // uv
        -1.f, -1.f,  0.f, 0.f,
        1.f, -1.f,  1.f, 0.f,
        -1.f,  1.f,  0.f, 1.f,

        -1.f,  1.f,  0.f, 1.f,
        1.f, -1.f,  1.f, 0.f,
        1.f,  1.f,  1.f, 1.f
    };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0); // pos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, (void*)0);
    glEnableVertexAttribArray(1); // uv
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, (void*)(sizeof(float)*2));

    glBindVertexArray(0);

    // Load post shader
    m_postShader = ShaderLoader::createShaderProgram(
        "C:/Users/skylo/graphics/proj5-VivekN147/src/crepuscular.vert",
        "C:/Users/skylo/graphics/proj5-VivekN147/src/crepuscular.frag"
        );
}

void Realtime::createShadowResources() {
    // delete any previous shadow resources
    deleteShadowResources();

    // Limit number of shadow-casting lights to available lights in scene
    int count = std::min<int>((int)m_renderData.lights.size(), MAX_SHADOW_CASTERS);
    m_shadowMaps.resize(count);

    for (int i = 0; i < count; ++i) {
        const SceneLightData &L = m_renderData.lights[i];
        ShadowMap sm;
        sm.lightIndex = i;

        if (L.type == LightType::LIGHT_DIRECTIONAL) {
            sm.type = ShadowType::Directional;
            sm.enabled = true;
            sm.size = DEFAULT_SHADOW_SIZE;
            // create 2D depth texture
            glGenTextures(1, &sm.depthTex);
            glBindTexture(GL_TEXTURE_2D, sm.depthTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, sm.size, sm.size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

            glGenFramebuffers(1, &sm.fbo);

        } else if (L.type == LightType::LIGHT_SPOT) {
            sm.type = ShadowType::Spot;
            sm.enabled = true;
            sm.size = DEFAULT_SHADOW_SIZE;
            // create 2D depth texture
            glGenTextures(1, &sm.depthTex);
            glBindTexture(GL_TEXTURE_2D, sm.depthTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, sm.size, sm.size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

            glGenFramebuffers(1, &sm.fbo);

        } else if (L.type == LightType::LIGHT_POINT) {
            sm.type = ShadowType::Point;
            sm.enabled = true;
            // cubemap size typically smaller
            sm.size = DEFAULT_SHADOW_SIZE / 2;
            glGenTextures(1, &sm.depthTex);
            glBindTexture(GL_TEXTURE_CUBE_MAP, sm.depthTex);
            for (unsigned int face = 0; face < 6; ++face) {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_DEPTH_COMPONENT24, sm.size, sm.size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
            }
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

            glGenFramebuffers(1, &sm.fbo);
        } else {
            sm.type = ShadowType::None;
            sm.enabled = false;
        }

        // store
        m_shadowMaps[i] = sm;
    }

    // Load depth shader (2D depth)
    m_depthShader = ShaderLoader::createShaderProgram(
        "C:/Users/skylo/graphics/proj5-VivekN147/src/shadow_depth.vert",
        "C:/Users/skylo/graphics/proj5-VivekN147/src/shadow_depth.frag"
        );

    // Optional: debug shader for visualizing depth (not required)
    // m_debugDepthShader = ShaderLoader::createShaderProgram(...);
}

void Realtime::initializeGL() {
    m_devicePixelRatio = this->devicePixelRatio();

    m_timer = startTimer(1000/60);
    m_elapsedTimer.start();

    m_defaultFBO = 2;
    std::cerr << m_defaultFBO << std::endl;

    // Initializing GL.
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

    // Students: anything requiring OpenGL calls when the program starts should be done here
    m_shader = ShaderLoader::createShaderProgram("C:/Users/skylo/graphics/proj5-VivekN147/resources/shaders/default.vert",
                                                 "C:/Users/skylo/graphics/proj5-VivekN147/resources/shaders/default.frag");

    // Create post resources for initial window size
    int w = int(size().width() * m_devicePixelRatio);
    int h = int(size().height() * m_devicePixelRatio);
    createPostResources(w, h);

    // Create shadow resources for initial scene (if lights present)
    if (!m_renderData.lights.empty()) {
        createShadowResources();
    }
}

// --- Use Camera's projection math for directional, spot, and cube faces ---
// Note: we create a temporary Camera and SceneCameraData and call Camera::setFromSceneData()
// and Camera::updateProjection(...) so the projection math comes from camera.cpp.

static LightSpace makeOrthoForDirectional(const SceneLightData &L)
{
    LightSpace ls;

    // Directional light view
    glm::vec3 dir = glm::normalize(glm::vec3(L.dir));
    glm::vec3 pos = -dir * 20.0f;   // push back from origin
    glm::vec3 up  = glm::vec3(0,1,0);

    ls.view = glm::lookAt(pos, glm::vec3(0), up);

    float orthoSize = 20.0f;
    ls.proj = glm::ortho(
        -orthoSize, orthoSize,
        -orthoSize, orthoSize,
        1.0f, 100.0f
        );

    return ls;
}

static LightSpace makePerspectiveForSpot(const SceneLightData &L)
{
    LightSpace ls;

    glm::vec3 pos = glm::vec3(L.pos);
    glm::vec3 dir = glm::normalize(glm::vec3(L.dir));
    glm::vec3 up  = glm::vec3(0,1,0);

    ls.view = glm::lookAt(pos, pos + dir, up);

    float fovy = L.angle * 2.0f;            // full cone angle
    float aspect = 1.0f;
    float nearP = 0.1f;
    float farP  = 100.0f;

    ls.proj = glm::perspective(fovy, aspect, nearP, farP);

    return ls;
}


static CubeLightSpace makeCubeFaceMatrices(const SceneLightData &L)
{
    CubeLightSpace cs;

    glm::vec3 pos = glm::vec3(L.pos);

    // GLM look directions for cube map
    glm::vec3 dirs[6] = {
        { 1, 0, 0}, {-1, 0, 0},
        { 0, 1, 0}, { 0,-1, 0},
        { 0, 0, 1}, { 0, 0,-1}
    };

    glm::vec3 ups[6] = {
        {0,-1, 0}, {0,-1, 0},
        {0, 0, 1}, {0, 0,-1},
        {0,-1, 0}, {0,-1, 0}
    };

    // 90° projection
    cs.proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);

    for (int i = 0; i < 6; ++i) {
        cs.views[i] = glm::lookAt(pos, pos + dirs[i], ups[i]);
    }

    return cs;
}

void Realtime::renderShadowMaps() {
    if (m_shadowMaps.empty()) return;
    if (!m_depthShader) return;

    // Save old viewport
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    glUseProgram(m_depthShader);

    // For each configured shadow map, render depth-only pass
    for (size_t si = 0; si < m_shadowMaps.size(); ++si) {
        ShadowMap &sm = m_shadowMaps[si];
        if (!sm.enabled || sm.type == ShadowType::None) continue;

        const SceneLightData &L = m_renderData.lights[sm.lightIndex];

        // ======== DIRECTIONAL + SPOT LIGHT SHADOW MAPS (2D depth) ========
        if (sm.type == ShadowType::Directional || sm.type == ShadowType::Spot) {

            glBindFramebuffer(GL_FRAMEBUFFER, sm.fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                   GL_TEXTURE_2D, sm.depthTex, 0);

            GLenum drawBuffers[1] = { GL_NONE };
            glDrawBuffers(0, drawBuffers); // depth only

            glViewport(0, 0, sm.size, sm.size);
            glClear(GL_DEPTH_BUFFER_BIT);

            // Compute light-space matrix using new struct-returning helpers
            LightSpace LS;
            if (sm.type == ShadowType::Directional) {
                LS = makeOrthoForDirectional(L);
            } else {
                LS = makePerspectiveForSpot(L);
            }

            glm::mat4 lightSpace = LS.proj * LS.view;
            sm.lightSpaceMatrix = lightSpace;

            // Upload uniforms
            GLint locModel      = glGetUniformLocation(m_depthShader, "model");
            GLint locLightSpace = glGetUniformLocation(m_depthShader, "lightSpace");

            if (locLightSpace != -1)
                glUniformMatrix4fv(locLightSpace, 1, GL_FALSE, glm::value_ptr(lightSpace));

            // Render each shadow-casting shape
            for (const GPUShape &shape : m_gpuShapes) {
                if (!shape.castsShadow) continue;

                if (locModel != -1)
                    glUniformMatrix4fv(locModel, 1, GL_FALSE, &shape.ctm[0][0]);

                glBindVertexArray(shape.vao);
                glDrawArrays(GL_TRIANGLES, 0, shape.count);
            }

            glBindVertexArray(0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // ======== POINT LIGHT SHADOW MAPS (CUBE MAP) ========
        else if (sm.type == ShadowType::Point) {

            glBindFramebuffer(GL_FRAMEBUFFER, sm.fbo);
            glViewport(0, 0, sm.size, sm.size);

            float nearp = 1.0f;
            float farp  = m_pointLightFar;

            CubeLightSpace CS = makeCubeFaceMatrices(L);

            GLint locModel      = glGetUniformLocation(m_depthShader, "model");
            GLint locLightSpace = glGetUniformLocation(m_depthShader, "lightSpace");

            // Render six faces of the cube map
            for (int face = 0; face < 6; ++face) {

                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                       GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                                       sm.depthTex, 0);

                glClear(GL_DEPTH_BUFFER_BIT);

                // Correct matrix for this face
                glm::mat4 faceMat = CS.proj * CS.views[face];

                if (locLightSpace != -1)
                    glUniformMatrix4fv(locLightSpace, 1, GL_FALSE, glm::value_ptr(faceMat));

                // Render all shapes
                for (const GPUShape &shape : m_gpuShapes) {
                    if (!shape.castsShadow) continue;

                    if (locModel != -1)
                        glUniformMatrix4fv(locModel, 1, GL_FALSE, &shape.ctm[0][0]);

                    glBindVertexArray(shape.vao);
                    glDrawArrays(GL_TRIANGLES, 0, shape.count);
                }
            }

            glBindVertexArray(0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }

    // Restore original viewport
    glViewport(prevViewport[0], prevViewport[1],
               prevViewport[2], prevViewport[3]);

    glUseProgram(0);
}

void Realtime::bindShadowMapsToShader(GLuint shader) {
    // Bind shadow maps to texture units and upload their uniforms:
    // We will use texture units starting at 8 to avoid clobbering previously used units.
    int baseUnit = 8;
    int count = (int)m_shadowMaps.size();

    // set bias and pcf toggles
    GLint locShadowBias = glGetUniformLocation(shader, "shadowBias");
    GLint locEnablePCF = glGetUniformLocation(shader, "enablePCF");
    GLint locPointFarPlane = glGetUniformLocation(shader, "pointLightFar");
    if (locShadowBias != -1) glUniform1f(locShadowBias, m_shadowBias);
    if (locEnablePCF != -1) glUniform1i(locEnablePCF, m_enablePCF ? 1 : 0);
    if (locPointFarPlane != -1) glUniform1f(locPointFarPlane, m_pointLightFar);

    for (int i = 0; i < count; ++i) {
        const ShadowMap &sm = m_shadowMaps[i];
        int typeInt = 0;
        if (!sm.enabled) typeInt = 0;
        else if (sm.type == ShadowType::Directional) typeInt = 1;
        else if (sm.type == ShadowType::Spot) typeInt = 2;
        else if (sm.type == ShadowType::Point) typeInt = 3;

        // safer: set by explicit name lookup
        std::string stName = "shadowType[" + std::to_string(i) + "]";
        GLint stLoc = glGetUniformLocation(shader, stName.c_str());
        if (stLoc != -1) glUniform1i(stLoc, typeInt);

        // lightSpace matrix
        std::string lsName = "lightSpace[" + std::to_string(i) + "]";
        GLint lsLoc = glGetUniformLocation(shader, lsName.c_str());
        if (lsLoc != -1) {
            glUniformMatrix4fv(lsLoc, 1, GL_FALSE, glm::value_ptr(sm.lightSpaceMatrix));
        }

        if (!sm.enabled) continue;

        if (sm.type == ShadowType::Directional || sm.type == ShadowType::Spot) {
            // bind 2D shadow texture
            glActiveTexture(GL_TEXTURE0 + baseUnit + i);
            glBindTexture(GL_TEXTURE_2D, sm.depthTex);
            std::string sampName = "shadowMap[" + std::to_string(i) + "]";
            GLint sampLoc = glGetUniformLocation(shader, sampName.c_str());
            if (sampLoc != -1) glUniform1i(sampLoc, baseUnit + i);
        } else if (sm.type == ShadowType::Point) {
            // bind cubemap
            glActiveTexture(GL_TEXTURE0 + baseUnit + i);
            glBindTexture(GL_TEXTURE_CUBE_MAP, sm.depthTex);
            std::string sampName = "shadowCube[" + std::to_string(i) + "]";
            GLint sampLoc = glGetUniformLocation(shader, sampName.c_str());
            if (sampLoc != -1) glUniform1i(sampLoc, baseUnit + i);
        }
    }

    // return active texture to 0
    glActiveTexture(GL_TEXTURE0);
}

void Realtime::paintGL() {
    // Students: anything requiring OpenGL calls every frame should be done here

    m_camera.updateView();
    float aspect = float(width()) / float(height());
    m_camera.updateProjection(aspect, settings.nearPlane, settings.farPlane);

    // // Ensure shadow resources exist for current lights
    // if ((int)m_renderData.lights.size() > 0 && (int)m_shadowMaps.size() != (int)m_renderData.lights.size()) {
    //     // recreate shadow resources when scene changes
    //     createShadowResources();
    // }

    // // 0) Render shadow maps
    // renderShadowMaps();

    // 1) Render scene into FBO (color + depth)
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, int(width()*m_devicePixelRatio), int(height()*m_devicePixelRatio));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_shader);

    uploadLightsAndCamera(m_shader, m_renderData, m_camera.getPos());

    // // bind shadow maps + set shadow uniforms
    // bindShadowMapsToShader(m_shader);

    for (const GPUShape &shape : m_gpuShapes) {
        sendUniforms(m_shader, shape, m_camera, m_renderData);
        glBindVertexArray(shape.vao);
        glDrawArrays(GL_TRIANGLES, 0, shape.count);
    }
    glBindVertexArray(0);

    // 2) Post-process pass: crepuscular + blend to default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
    glViewport(0, 0, int(width()*m_devicePixelRatio), int(height()*m_devicePixelRatio));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_postShader);

    // Bind color texture to unit 0 and depth texture to unit 1
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_colorTex);
    glUniform1i(glGetUniformLocation(m_postShader, "sceneTex"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_depthTex);
    glUniform1i(glGetUniformLocation(m_postShader, "depthTex"), 1);

    // Compute light screen-space position
    // Prefer a directional light, else use first light's position.
    glm::vec3 lightWorldPos(0.0f);
    bool foundLight = false;
    for (const SceneLightData &L : m_renderData.lights) {
        if (L.type == LightType::LIGHT_DIRECTIONAL) {
            glm::vec3 dir = glm::normalize(glm::vec3(L.dir));
            lightWorldPos = m_camera.getPos() - dir * 1000.0f; // far away point along direction
            foundLight = true;
            break;
        }
    }
    if (!foundLight && !m_renderData.lights.empty()) {
        lightWorldPos = m_renderData.lights[0].pos;
        foundLight = true;
    }

    glm::vec2 lightScreenUV(0.5f, 0.5f);
    if (foundLight) {
        glm::mat4 view = m_camera.getView();
        glm::mat4 proj = m_camera.getProj();
        glm::vec4 clip = proj * view * glm::vec4(lightWorldPos, 1.0f);
        if (clip.w != 0.0f) {
            glm::vec3 ndc = glm::vec3(clip) / clip.w;
            lightScreenUV = glm::vec2(ndc.x * 0.5f + 0.5f, ndc.y * 0.5f + 0.5f);
        }
    }

    glUniform2f(glGetUniformLocation(m_postShader, "lightScreenPos"), lightScreenUV.x, lightScreenUV.y);

    // parameters for effect (tweakable)
    glUniform1f(glGetUniformLocation(m_postShader, "exposure"), 0.6f);
    glUniform1f(glGetUniformLocation(m_postShader, "decay"), 0.95f);
    glUniform1f(glGetUniformLocation(m_postShader, "density"), 0.8f);
    glUniform1f(glGetUniformLocation(m_postShader, "weight"), 0.7f);
    glUniform1i(glGetUniformLocation(m_postShader, "numSamples"), 100);

    // viewport size (for uv step computation in shader)
    glUniform2f(glGetUniformLocation(m_postShader, "screenSize"), float(width()), float(height()));

    // Update facing uniform every frame
    glm::vec3 camForward = m_camera.getLook();
    glm::vec3 lightDir   = glm::normalize(lightWorldPos - m_camera.getPos());
    float facing = glm::dot(camForward, lightDir);
    if (facing < 0.f) facing = 0.f;
    glUniform1f(glGetUniformLocation(m_postShader, "facing"), facing);

    // draw quad
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    // done
}
