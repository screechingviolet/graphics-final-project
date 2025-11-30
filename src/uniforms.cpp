#include "realtime.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <iostream>
#include "settings.h"
#include "utils/shaderloader.h"

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
