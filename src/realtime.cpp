#include "realtime.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <iostream>
#include "settings.h"
#include "utils/shaderloader.h"


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

    // Students: anything requiring OpenGL calls when the program starts should be done here

    // Task 4: Set the clear color here
    glClearColor(0, 0, 0, 1.0);

    // Shader setup (DO NOT EDIT)
    m_shader = ShaderLoader::createShaderProgram(":/resources/shaders/default.vert", ":/resources/shaders/default.frag");


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

    rebuildMeshes();
}

void Realtime::deleteAllMeshes() {
    for (auto [meshfile, vaovbo]: m_meshIds) {
        glDeleteVertexArrays(1, &vaovbo.shape_vao);
        glDeleteBuffers(1, &vaovbo.shape_vbo);
    }

    m_meshIds.clear();
    m_meshes.clear();
}

void Realtime::rebuildMeshes() {
    deleteAllMeshes();
    std::cout << "rebuilding meshes " << std::endl;

    std::unordered_set<std::string> meshfiles;
    for (RenderShapeData& shape: m_renderdata.shapes) {
        if (shape.primitive.type == PrimitiveType::PRIMITIVE_MESH) {
            std::cout << "Found a mesh" << std::endl;
            meshfiles.insert(shape.primitive.meshfile);
        }
    }
    for (std::string meshfile: meshfiles) {
        // gen a buffer and vao
        glGenBuffers(1, &m_meshIds[meshfile].shape_vbo);
        glGenVertexArrays(1, &m_meshIds[meshfile].shape_vao);
        // create mesh and call update on the new mesh
        // m_meshes[meshfile] = Mesh();
        m_meshes[meshfile].updateMesh(meshfile);
        // setupprimitives
        setupPrimitives(&m_meshIds[meshfile], m_meshes[meshfile].generateShape());
    }
}

void Realtime::setupPrimitives(VboVao* shape_ids, const std::vector<GLfloat>& triangles) {
    glBindBuffer(GL_ARRAY_BUFFER, shape_ids->shape_vbo);

    // Task 9: Pass the triangle vector into your VBO here
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * triangles.size(), triangles.data(), GL_STATIC_DRAW);
    std::cout << triangles[0] << triangles[1] << triangles[2] << std::endl;
    // Task 11: Generate a VAO here and store it in m_vao
    glBindVertexArray(shape_ids->shape_vao);

    // Task 13: Add position and normal attributes to your VAO here
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void*>(0));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));

    // Task 14: Unbind your VBO and VAO here
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Realtime::paintGL() {
    // Students: anything requiring OpenGL calls every frame should be done here
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Bind the shader
    glUseProgram(m_shader);

    GLuint vertices;
    // for each shape: bind vao, decl shape uniforms, draw, unbind, repeat
    for (RenderShapeData &shape: m_renderdata.shapes) {

        // Task 16: Bind your VAO here
        // glBindVertexArray(m_vao);
        switch (shape.primitive.type) {
        case PrimitiveType::PRIMITIVE_CONE:
            vertices = m_cone->num_triangles;
            glBindVertexArray(m_coneIds->shape_vao);
            break;
        case PrimitiveType::PRIMITIVE_CUBE:
            vertices = m_cube->num_triangles;
            glBindVertexArray(m_cubeIds->shape_vao);
            break;
        case PrimitiveType::PRIMITIVE_CYLINDER:
            vertices = m_cylinder->num_triangles;
            glBindVertexArray(m_cylinderIds->shape_vao);
            break;
        case PrimitiveType::PRIMITIVE_SPHERE:
            vertices = m_sphere->num_triangles;
            glBindVertexArray(m_sphereIds->shape_vao);
            break;
        default:
            if (m_meshes.count(shape.primitive.meshfile) == 0) {
                vertices = 0;
            } else {
                vertices = m_meshes[shape.primitive.meshfile].num_triangles;
                glBindVertexArray(m_meshIds[shape.primitive.meshfile].shape_vao);
            }
            break;
        }

        declSpecificUniforms(shape);

        // Task 17: Draw your VAO here
        glDrawArrays(GL_TRIANGLES, 0, vertices);

        glBindVertexArray(0);
    }

    // Unbind the shader
    glUseProgram(0);
}

void Realtime::declareCameraUniforms() {
    // --- CAMERA DATA ---
    GLint loc_view = glGetUniformLocation(m_shader, "view");
    glUniformMatrix4fv(loc_view, 1, GL_FALSE, &m_cam.view[0][0]); // general

    GLint loc_proj = glGetUniformLocation(m_shader, "proj");
    glUniformMatrix4fv(loc_proj, 1, GL_FALSE, &m_proj[0][0]); // general

    GLint loc_cam_pos = glGetUniformLocation(m_shader, "camPos");
    glUniform4fv(loc_cam_pos, 1, &m_cam.pos[0]); // general
}

void Realtime::declGeneralUniforms() { // how often do i do this - every vao/vbo drawn
    // --- GLOBAL DATA ---
    GLint loc_ka = glGetUniformLocation(m_shader, "ka");
    glUniform1f(loc_ka, m_renderdata.globalData.ka);

    GLint loc_kd = glGetUniformLocation(m_shader, "kd");
    glUniform1f(loc_kd, m_renderdata.globalData.kd);

    GLint loc_ks = glGetUniformLocation(m_shader, "ks");
    glUniform1f(loc_ks, m_renderdata.globalData.ks);

    // --- LIGHT DATA ---
    GLfloat lightColors[8*3]; // scenecolor is 0 to 1 i think
    GLfloat lightFunctions[8*3];
    GLfloat lightPenumbras[8];
    GLfloat lightAngles[8];
    GLint lightTypes[8];
    GLfloat lightPositions[8*4];
    GLfloat lightDirections[8*4];
    for (int i = 0; i < m_renderdata.lights.size(); i++) {
        for (int j = 0; j < 3; j++) lightColors[i*3+j] = m_renderdata.lights[i].color[j];
        switch (m_renderdata.lights[i].type) {
        case LightType::LIGHT_DIRECTIONAL:
            lightTypes[i] = 1;
            for (int j = 0; j < 4; j++) lightDirections[i*4+j] = m_renderdata.lights[i].dir[j];
            break;
        case LightType::LIGHT_POINT:
            lightTypes[i] = 0;
            for (int j = 0; j < 3; j++) lightFunctions[i*3+j] = m_renderdata.lights[i].function[j];
            for (int j = 0; j < 4; j++) lightPositions[i*4+j] = m_renderdata.lights[i].pos[j];
            break;
        case LightType::LIGHT_SPOT:
            lightTypes[i] = 2;
            lightAngles[i] = m_renderdata.lights[i].angle;
            lightPenumbras[i] = m_renderdata.lights[i].penumbra;
            for (int j = 0; j < 3; j++) lightFunctions[i*3+j] = m_renderdata.lights[i].function[j];
            for (int j = 0; j < 4; j++) lightDirections[i*4+j] = m_renderdata.lights[i].dir[j];
            for (int j = 0; j < 4; j++) lightPositions[i*4+j] = m_renderdata.lights[i].pos[j];
            break;
        default:
            break;
        }
    }
    glUniform1i(glGetUniformLocation(m_shader, "lightsNum"), m_renderdata.lights.size());
    glUniform3fv(glGetUniformLocation(m_shader, "lightColors"), 8, lightColors);
    glUniform1iv(glGetUniformLocation(m_shader, "lightTypes"), 8, lightTypes);
    glUniform1fv(glGetUniformLocation(m_shader, "lightPenumbras"), 8, lightPenumbras);
    glUniform1fv(glGetUniformLocation(m_shader, "lightAngles"), 8, lightAngles);
    glUniform3fv(glGetUniformLocation(m_shader, "lightFunctions"), 8, lightFunctions);
    glUniform4fv(glGetUniformLocation(m_shader, "lightPositions"), 8, lightPositions);
    glUniform4fv(glGetUniformLocation(m_shader, "lightDirections"), 8, lightDirections);

}

void Realtime::declSpecificUniforms(RenderShapeData& shape) { // is it bad to pass a whole matrix
    // --- SHAPE DATA ---
    GLint loc_shiny = glGetUniformLocation(m_shader, "shininess");
    glUniform1f(loc_shiny, shape.primitive.material.shininess); // material specific

    GLint loc_model = glGetUniformLocation(m_shader, "model");
    glUniformMatrix4fv(loc_model, 1, GL_FALSE, &shape.ctm[0][0]); // shape ctm

    glUniformMatrix4fv(glGetUniformLocation(m_shader, "model_inv_trans"), 1, GL_FALSE, &shape.ctm_inv_trans[0][0]);

    GLfloat shapeColorA[3] = {shape.primitive.material.cAmbient[0], shape.primitive.material.cAmbient[1], shape.primitive.material.cAmbient[2]};
    GLfloat shapeColorD[3] = {shape.primitive.material.cDiffuse[0], shape.primitive.material.cDiffuse[1], shape.primitive.material.cDiffuse[2]};
    GLfloat shapeColorS[3] = {shape.primitive.material.cSpecular[0], shape.primitive.material.cSpecular[1], shape.primitive.material.cSpecular[2]};
    glUniform3fv(glGetUniformLocation(m_shader, "shapeColorA"), 1, shapeColorA);
    glUniform3fv(glGetUniformLocation(m_shader, "shapeColorD"), 1, shapeColorD);
    glUniform3fv(glGetUniformLocation(m_shader, "shapeColorS"), 1, shapeColorS);
}

void Realtime::resizeGL(int w, int h) {
    // Tells OpenGL how big the screen is
    glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);

    // Students: anything requiring OpenGL calls when the program starts should be done here
}

void Realtime::sceneChanged() {
    makeCurrent();
    SceneParser::parse(settings.sceneFilePath, m_renderdata);
    rebuildCamera();
    rebuildMatrices();
    rebuildMeshes();

    glUseProgram(m_shader);
    declareCameraUniforms();
    declGeneralUniforms();
    glUseProgram(0);
    update(); // asks for a PaintGL() call to occur
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
        setupPrimitives(m_coneIds, m_cone->generateShape());
    }

    if (m_cylinderIds->shape_vao != 0 && m_cylinderIds->shape_vbo != 0) {
        m_cylinder->updateParams(settings.shapeParameter1, settings.shapeParameter2);
        setupPrimitives(m_cylinderIds, m_cylinder->generateShape());
    }

    if (m_shader != 0 && (settings.nearPlane != near || settings.farPlane != far)) {
        rebuildMatrices();
        glUseProgram(m_shader);
        declareCameraUniforms(); // do i need to anymore
        glUseProgram(0);
    }
    update(); // asks for a PaintGL() call to occur
    // if nearPlane or farPlane changed update camera settings
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
            // m_cam.look = rotation * m_cam.look;
            // m_cam.up = rotation * m_cam.up; // only do this for roty maybe?
            // glm::vec3 w = -glm::normalize(glm::vec3(m_cam.look));
            // glm::vec3 v = glm::normalize(glm::vec3(m_cam.up) - (glm::dot(glm::vec3(m_cam.up), w) * w));
            // glm::vec3 u = glm::cross(v, w);

            // glm::mat4 rot = glm::mat4(u.x, v.x, w.x, 0, u.y, v.y, w.y, 0, u.z, v.z, w.z, 0, 0, 0, 0, 1);
            // glm::mat4 trans = glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -m_cam.pos.x, -m_cam.pos.y, -m_cam.pos.z, 1);
            // m_cam.view = rot * trans;
            // m_cam.viewInv = glm::inverse(m_cam.view);

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

void Realtime::rebuildCamera() {
    glm::vec3 look = glm::vec3(m_renderdata.cameraData.look);

    glm::vec3 up = glm::vec3(m_renderdata.cameraData.up);
    glm::vec3 pos = glm::vec3(m_renderdata.cameraData.pos);
    // std::cout << m_renderdata.cameraData.look[3] << " hi ???  " << m_renderdata.cameraData.up[3] << " " << m_renderdata.cameraData.pos[3] << std::endl;
    glm::vec3 w = -glm::normalize(look);
    glm::vec3 v = glm::normalize(up - (glm::dot(up, w) * w));
    glm::vec3 u = glm::cross(v, w);

    glm::mat4 rot = glm::mat4(u.x, v.x, w.x, 0,
                              u.y, v.y, w.y, 0,
                              u.z, v.z, w.z, 0,
                              0, 0, 0, 1.f);
    glm::mat4 trans = glm::mat4(1.f, 0, 0, 0,
                                0, 1.f, 0, 0,
                                0, 0, 1.f, 0,
                                -pos.x, -pos.y, -pos.z, 1.f);
    glm::mat4 viewMatrix = rot * trans;

    glm::mat4 viewInv = glm::inverse(m_cam.view);

    m_cam = Camera{viewMatrix, viewInv, glm::vec4{pos.x, pos.y, pos.z, 1}, m_renderdata.cameraData.look, m_renderdata.cameraData.up,
                   m_renderdata.cameraData.heightAngle, m_renderdata.cameraData.aperture, m_renderdata.cameraData.focalLength};

}

void Realtime::rebuildMatrices() {

    near = settings.nearPlane;
    far = settings.farPlane;
    float c = -near/far;
    float widthAngle = m_cam.heightAngle * (size().width()/(float)size().height());// ((size().width() * m_devicePixelRatio)/(size().height() * m_devicePixelRatio)); // find value

    glm::mat4 unhinging = glm::mat4(1.f, 0, 0, 0,
                                  0, 1.f, 0, 0,
                                  0, 0, (1.f/(1.f+c)), -1.f,
                                  0, 0, ((-c)/(1.f+c)), 0);
    glm::mat4 scaling = glm::mat4((1.f/(far*tan(widthAngle/2.f))), 0, 0, 0,
                                    0, (1.f/(far*tan(m_cam.heightAngle/2.f))), 0, 0,
                                    0, 0, (1.f/far), 0,
                                    0, 0, 0, 1.f);
    m_proj = glm::mat4(1.f, 0, 0, 0,
                       0, 1.f, 0, 0,
                       0, 0, -2.f, 0,
                       0, 0, -1.f, 1.f) * unhinging * scaling;

}


