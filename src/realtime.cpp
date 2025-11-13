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

    // set camera view and proj matrices

    // construct vbos and vaos for every shape?? then use ctm for transforming later

    // Task 4: Set the clear color here
    glClearColor(0, 0, 0, 1.0);

    // Shader setup (DO NOT EDIT)
    m_shader = ShaderLoader::createShaderProgram(":/resources/shaders/default.vert", ":/resources/shaders/default.frag");

    // ================== Vertex Buffer Objects

    // glGenBuffers(1, &m_vbo);
    // glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    // Sphere s;
    // s.updateParams(10, 10);

    // std::vector<GLfloat> triangle = s.generateShape();

    // // Task 9: Pass the triangle vector into your VBO here
    // glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * triangle.size(), triangle.data(), GL_STATIC_DRAW);

    // // ================== Vertex Array Objects

    // // Task 11: Generate a VAO here and store it in m_vao
    // glGenVertexArrays(1, &m_vao);
    // glBindVertexArray(m_vao);

    // // Task 13: Add position and normal attributes to your VAO here
    // glEnableVertexAttribArray(0);
    // glEnableVertexAttribArray(1);
    // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void*>(0));
    // glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));

    // // ================== Returning to Default State

    // // Task 14: Unbind your VBO and VAO here
    // glBindBuffer(GL_ARRAY_BUFFER, 0);
    // glBindVertexArray(0);

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

void Realtime::setupPrimitives(VboVao* shape_ids, const std::vector<GLfloat>& triangles) {
    glBindBuffer(GL_ARRAY_BUFFER, shape_ids->shape_vbo);

    // Task 9: Pass the triangle vector into your VBO here
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * triangles.size(), triangles.data(), GL_STATIC_DRAW);

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
    // set min parameters for the shapes

    // Task 15: Clear the screen here
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Bind the shader
    glUseProgram(m_shader);

    // decl global uniforms and for all 8 lights
    declGeneralUniforms(); // change the uniforms in between draws

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
            vertices = 0;
            break;
        }

        // pass shape.ctm
        declSpecificUniforms(shape);

        // Task 17: Draw your VAO here
        glDrawArrays(GL_TRIANGLES, 0, vertices);

        glBindVertexArray(0);
    }



    // Unbind the shader
    glUseProgram(0);
}

void Realtime::declGeneralUniforms() { // how often do i do this - every vao/vbo drawn
    // --- CAMERA DATA ---
    GLint loc_view = glGetUniformLocation(m_shader, "view");
    glUniformMatrix4fv(loc_view, 1, GL_FALSE, &m_cam.view[0][0]); // general

    GLint loc_proj = glGetUniformLocation(m_shader, "proj");
    glUniformMatrix4fv(loc_proj, 1, GL_FALSE, &m_proj[0][0]); // general

    GLint loc_cam_pos = glGetUniformLocation(m_shader, "camPos");
    glUniform4fv(loc_cam_pos, 1, &m_cam.pos[0]); // general
    // -------------------

    // --- GLOBAL DATA ---
    GLint loc_ka = glGetUniformLocation(m_shader, "ka");
    glUniform1f(loc_ka, m_renderdata.globalData.ka);

    GLint loc_kd = glGetUniformLocation(m_shader, "kd");
    glUniform1f(loc_kd, m_renderdata.globalData.kd);

    GLint loc_ks = glGetUniformLocation(m_shader, "ks");
    glUniform1f(loc_ks, m_renderdata.globalData.ks);
    // -------------------

    // --- LIGHT DATA ---
    // change to add all the light data
    /*
     *     LightType type;

    SceneColor color;
    glm::vec3 function; // Attenuation function
    glm::vec4 dir;      // Not applicable to point lights

    float penumbra; // Only applicable to spot lights, in RADIANS
    float angle;    // Only applicable to spot lights, in RADIANS

    uniform vec3 lightColors[8];
    uniform int lightTypes[8];
    uniform vec3 lightFunctions[8];
    uniform vec4 lightPositions[8];
    uniform vec4 lightDirections[8];
    uniform float lightAngles[8];
    uniform float lightPenumbras[8];

GLint loc = glGetUniformLocation(shaderHandle, "myVectors[" + std::to_string(j) + "]");
glUniform3f(loc, x, y, z);

     */
    std::vector<GLfloat> lightColors(8*3); // scenecolor is 0 to 1 i think
    std::vector<GLfloat> lightFunctions(8*3);
    GLfloat lightPenumbras[8];
    GLfloat lightAngles[8];
    GLint lightTypes[8];
    std::vector<GLfloat> lightPositions(8*4);
    std::vector<GLfloat> lightDirections(8*4);
    // GLint loc_type;
    // const GLchar* tempType;
    for (int i = 0; i < m_renderdata.lights.size(); i++) {
        // tempType = ("lightTypes[" + std::to_string(i) + "]").c_str();
        for (int j = 0; j < 3; j++) lightColors.push_back(m_renderdata.lights[i].color[j]);
        switch (m_renderdata.lights[i].type) {
        case LightType::LIGHT_DIRECTIONAL:
            lightTypes[i] = 1;
            for (int j = 0; j < 4; j++) lightDirections.push_back(m_renderdata.lights[i].dir[j]);
            break;
        case LightType::LIGHT_POINT:
            lightTypes[i] = 0;
            for (int j = 0; j < 3; j++) lightFunctions.push_back(m_renderdata.lights[i].function[j]);
            for (int j = 0; j < 4; j++) lightPositions.push_back(m_renderdata.lights[i].pos[j]);

            break;
        case LightType::LIGHT_SPOT:
            lightTypes[i] = 2;
            lightAngles[i] = m_renderdata.lights[i].angle;
            lightPenumbras[i] = m_renderdata.lights[i].penumbra;
            for (int j = 0; j < 3; j++) lightFunctions.push_back(m_renderdata.lights[i].function[j]);
            for (int j = 0; j < 4; j++) lightDirections.push_back(m_renderdata.lights[i].dir[j]);
            for (int j = 0; j < 4; j++) lightPositions.push_back(m_renderdata.lights[i].pos[j]);

            break;
        default:
            break;
        }
    }
    glUniform1i(glGetUniformLocation(m_shader, "lightsNum"), m_renderdata.lights.size());
    glUniform1iv(glGetUniformLocation(m_shader, "lightTypes"), 8, lightTypes);
    glUniform1fv(glGetUniformLocation(m_shader, "lightPenumbras"), 8, lightPenumbras);
    glUniform1fv(glGetUniformLocation(m_shader, "lightAngles"), 8, lightAngles);
    glUniform3fv(glGetUniformLocation(m_shader, "lightColors"), 8, lightColors.data());
    glUniform3fv(glGetUniformLocation(m_shader, "lightFunctions"), 8, lightFunctions.data());
    glUniform4fv(glGetUniformLocation(m_shader, "lightPositions"), 8, lightPositions.data());
    glUniform4fv(glGetUniformLocation(m_shader, "lightDirections"), 8, lightDirections.data());

    glm::vec4 m_lightPos = glm::vec4(3, 3, 3, 1);
    GLint loc_light_pos = glGetUniformLocation(m_shader, "lightPos");
    glUniform4fv(loc_light_pos, 1, &m_lightPos[0]); // light specific
    // ------------------
}

void Realtime::declSpecificUniforms(RenderShapeData& shape) { // is it bad to pass a whole matrix
    // --- SHAPE DATA ---
    GLint loc_shiny = glGetUniformLocation(m_shader, "shininess");
    glUniform1f(loc_shiny, shape.primitive.material.shininess); // material specific

    GLint loc_model = glGetUniformLocation(m_shader, "model");
    glUniformMatrix4fv(loc_model, 1, GL_FALSE, &shape.ctm[0][0]); // shape ctm
    // ------------------
}

void Realtime::resizeGL(int w, int h) {
    // Tells OpenGL how big the screen is
    glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);

    // Students: anything requiring OpenGL calls when the program starts should be done here
}

void Realtime::sceneChanged() {
    makeCurrent();
    SceneParser::parse(settings.sceneFilePath, m_renderdata);
    rebuildMatrices();
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

    rebuildMatrices();
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
        glm::mat4 rotation = glm::mat4(1);
        // Use deltaX and deltaY here to rotate
        if (deltaX != 0) {
            pointing = ((deltaX > 0) ? 1.f : -1.f);

            std::cout << pointing << std::endl;
            glm::vec4 moveX = m_cam.view * glm::vec4(0, 1, 0, 0);
            updated = true;
            rotation = rotationhelper(moveX, (deltaX/(float)size().width())*(M_PI/2.)) * rotation;
            // m_cam.look = rotation * m_cam.look;
            // rotate look and up or wtv
        }
        if (deltaY != 0) {
            pointing = ((deltaY > 0) ? 1.f : -1.f);
            glm::vec4 moveY = glm::vec4(glm::cross(glm::vec3(m_cam.look), glm::vec3(m_cam.up)), 0); // axis for y distance travelled
            rotation = rotationhelper(moveY, (deltaY/(float)size().height())*(M_PI/4.)) * rotation;

            updated = true;
        }

        if (updated) {
            m_cam.look = rotation * m_cam.look;
            // m_cam.up = rotation * m_cam.up; // only do this for roty maybe?
            glm::vec3 w = -glm::normalize(glm::vec3(m_cam.look));
            glm::vec3 v = glm::normalize(glm::vec3(m_cam.up) - (glm::dot(glm::vec3(m_cam.up), w) * w));
            glm::vec3 u = glm::cross(v, w);

            glm::mat4 rot = glm::mat4(u.x, v.x, w.x, 0, u.y, v.y, w.y, 0, u.z, v.z, w.z, 0, 0, 0, 0, 1);
            glm::mat4 trans = glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -m_cam.pos.x, -m_cam.pos.y, -m_cam.pos.z, 1);
            m_cam.view = rot * trans;
            m_cam.viewInv = glm::inverse(m_cam.view);
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
        m_cam.pos += (5 * deltaTime * m_cam.look);
        updatedoccurred = true;
    } else if (m_keyMap[Qt::Key_S]) {
        m_cam.pos -= (5 * deltaTime * m_cam.look);
        updatedoccurred = true;
    }

    if (m_keyMap[Qt::Key_A]) {
        glm::vec4 move = glm::vec4(glm::cross(glm::vec3(m_cam.look), glm::vec3(m_cam.up)), 0);
        m_cam.pos -= (5 * deltaTime * move);
        updatedoccurred = true;
    } else if (m_keyMap[Qt::Key_D]) {
        glm::vec4 move = glm::vec4(glm::cross(glm::vec3(m_cam.look), glm::vec3(m_cam.up)), 0);
        m_cam.pos += (5 * deltaTime * move);
        updatedoccurred = true;
    }

    if (m_keyMap[Qt::Key_Space]) {
        glm::vec4 translate = m_cam.view * glm::vec4(0, 1, 0, 0);
        m_cam.pos += (5 * deltaTime * translate);
        updatedoccurred = true;
    } else if (m_keyMap[Qt::Key_Control]) {
        glm::vec4 translate = m_cam.view * glm::vec4(0, 1, 0, 0);
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
    }


    update(); // asks for a PaintGL() call to occur
}

// DO NOT EDIT
void Realtime::saveViewportImage(std::string filePath) {
    // Make sure we have the right context and everything has been drawn
    makeCurrent();

    int fixedWidth = 1024;
    int fixedHeight = 768;

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

void Realtime::rebuildMatrices() { // does not use disallowed functions (i hope)
    glm::vec3 look = glm::vec3(m_renderdata.cameraData.look);
    glm::vec3 up = glm::vec3(m_renderdata.cameraData.up);
    glm::vec3 pos = glm::vec3(m_renderdata.cameraData.pos);
    glm::vec3 w = -glm::normalize(look);
    glm::vec3 v = glm::normalize(up - (glm::dot(up, w) * w));
    glm::vec3 u = glm::cross(v, w);

    glm::mat4 rot = glm::mat4(u.x, v.x, w.x, 0, u.y, v.y, w.y, 0, u.z, v.z, w.z, 0, 0, 0, 0, 1);
    glm::mat4 trans = glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -pos.x, -pos.y, -pos.z, 1);
    glm::mat4 viewMatrix = rot * trans;

    glm::mat4 viewInv = glm::inverse(m_cam.view);
    // std::cout << pos.x << " " << pos.y << "  " << pos.z << std::endl;
    // std::cout << look.x << " " << look.y << "  " << look.z << std::endl;

    m_cam = Camera{viewMatrix, viewInv, glm::vec4{pos.x, pos.y, pos.z, 1}, m_renderdata.cameraData.look, m_renderdata.cameraData.up,
                 m_renderdata.cameraData.heightAngle, m_renderdata.cameraData.aperture, m_renderdata.cameraData.focalLength};

    near = settings.nearPlane;
    far = settings.farPlane;
    std::cout << near << " near far wherever you are " << far << std::endl;
    float c = -near/far;
    std::cout << "c: " << c << std::endl;
    float widthAngle = m_cam.heightAngle * ((size().width() * m_devicePixelRatio)/(size().height() * m_devicePixelRatio)); // find value

    glm::mat4 unhinging = glm::mat4(1, 0, 0, 0,
                                  0, 1, 0, 0,
                                  0, 0, (1./(1.+c)), -1,
                                  0, 0, ((-c)/(1.+c)), 0);
    glm::mat4 scaling = glm::mat4((1./(far*tan(widthAngle/2.))), 0, 0, 0,
                                    0, (1./(far*tan(m_cam.heightAngle/2.))), 0, 0,
                                    0, 0, (1./far), 0,
                                    0, 0, 0, 1);
    m_proj = glm::mat4(1, 0, 0, 0,
                       0, 1, 0, 0,
                       0, 0, -2, 0,
                       0, 0, -1, 1) * unhinging * scaling;

    // glm::vec4 temp_world_pos = scaling * m_cam.view * glm::mat4(1, 0, 0, 0,
    //                                                            0, 1, 0, 0,
    //                                                            0, 0, 1, 0,
    //                                                            0, 0, 0, 1) * glm::vec4(0, 0, 0, 1);
    // std::cout << temp_world_pos.x << " " << temp_world_pos.y << " " << temp_world_pos.z << " " << temp_world_pos.w << std::endl;

    // temp_world_pos = unhinging * temp_world_pos;
    // std::cout << temp_world_pos.x << " " << temp_world_pos.y << " " << temp_world_pos.z << " " << temp_world_pos.w << std::endl;

    // temp_world_pos = m_proj * m_cam.view * glm::mat4(1, 0, 0, 0,
    //                                                               0, 1, 0, 0,
    //                                                               0, 0, 1, 0,
    //                                                               0, 0, 0, 1) * glm::vec4(0, 0, 0, 1);
    // std::cout << temp_world_pos.x << " " << temp_world_pos.y << " " << temp_world_pos.z << " " << temp_world_pos.w << std::endl;

}


