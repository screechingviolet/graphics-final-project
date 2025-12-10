#include "realtime.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QString>
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

void Realtime::declareSkyboxUniforms(int tex1, int tex2, float interp_factor) {
    glUniform1i(glGetUniformLocation(m_skybox_shader, "skybox_txt"), tex1); // decide based on interpolation
    glUniform1i(glGetUniformLocation(m_skybox_shader, "skybox_txt2"), tex2); // decide based on interpolation
    glUniform1f(glGetUniformLocation(m_skybox_shader, "interp"), interp_factor);

    glm::mat4 view_temp = glm::mat4(glm::mat3(m_cam.view));
    glUniformMatrix4fv(glGetUniformLocation(m_skybox_shader, "view"), 1, GL_FALSE, &view_temp[0][0]);

    glUniformMatrix4fv(glGetUniformLocation(m_skybox_shader, "proj"), 1, GL_FALSE, &m_proj[0][0]);
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

    glUniform1f(glGetUniformLocation(m_shader, "blend"), shape.primitive.material.blend);

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


void Realtime::initializeTextures(std::string filepath) {
    m_textures.resize(1);
    // std::cout << "base filepath " << filepath << std::endl;
    std::filesystem::path basepath = std::filesystem::path(filepath).parent_path(); // .parent_path();
    std::filesystem::path fileRelativePath("textures/Texture_1.png");
    //std::filesystem::path fileRelativePath("textures/water.png");

    // Prepare filepath
    QString tex_filepath = QString((basepath/fileRelativePath).string().c_str());
    // std::cout << "FILEASPTH" << (basepath / fileRelativePath).string() << std::endl;

    std::vector<QImage> images;
    std::vector<QString> filenames;

    // Task 1: Obtain image from filepath
    filenames.push_back("textures/Texture_1.png");
    filenames.push_back("textures/turbulentNoise.png");

    // Obtain texture images for all shapes
    for (RenderShapeData &shape: m_renderdata.shapes) {
        if (shape.primitive.material.textureMap.isUsed) {
            QString primitiveTextureFile = QString(shape.primitive.material.textureMap.filename.c_str());
            // std::cout << "adding file " << primitiveTextureFile.toStdString() << std::endl;

            if (std::find(filenames.begin(), filenames.end(), primitiveTextureFile) == filenames.end()) {
                filenames.push_back(primitiveTextureFile);
                // std::cout << "mana  push back" << std::endl;
            }
        }
    }

    for (QString &filename: filenames) {
        // std::cout << "FILENLAME:" << filename.toStdString() << std::endl;
    }

    for (int i = 0; i < filenames.size(); i++) {
        // std::cout << filenames[i].toStdString() << std::endl;
        std::filesystem::path relativePath(filenames[i].toStdString());
        tex_filepath = QString((basepath/relativePath).string().c_str());
        QImage image = QImage(tex_filepath);
        m_texIndexLUT[filenames[i].toStdString()] = i;

        if (image.isNull()) {
            std::cout << "No such image: iteration " << i << std::endl;
        }

        // Task 2: Format image to fit OpenGL
        image = image.convertToFormat(QImage::Format_RGBA8888).mirrored();

        // Task 3: Generate texture
        glGenTextures(1, &m_textures[i]);

        // Task 9: Set the active texture slot to texture slot 20 + i
        glActiveTexture(GL_TEXTURE20 + i);

        // Task 4: Bind texture
        glBindTexture(GL_TEXTURE_2D, m_textures[i]);

        // Task 5: Load image into texture
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.bits());

        // Task 6: Set min and mag filters' interpolation mode to linear
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Task 7: Unbind texture
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // SKYBOX
    m_skybox.resize(4);
    for (int i = 0; i < 4; i++) {
        glGenTextures(1, &m_skybox[i]);
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_skybox[i]);

        const std::vector<std::string> skyboxes = {"dawn_sky", "blue_sky", "purple_sky", "cloudy_sky"};

        std::vector<std::filesystem::path> skyboxpaths = {
            "textures/" + skyboxes[i] + "/right.png",
            "textures/" + skyboxes[i] + "/left.png",
            "textures/" + skyboxes[i] + "/up.png",
            "textures/" + skyboxes[i] + "/down.png",
            "textures/" + skyboxes[i] + "/front.png",
            "textures/" + skyboxes[i] + "/back.png"
        };

        // use 0 to 3

        QImage image;

        for (int j = 0; j < 6; j++) {
            image = QImage((basepath / skyboxpaths[j]).string().c_str());
            // std::cout << (basepath / skyboxpaths[j]).string() << std::endl;
            if (image.isNull()) std::cout << "image is null :(" << std::endl;
            image = image.convertToFormat(QImage::Format_RGBA8888); // .mirrored();
            //if (image.isNull()) std::cout << "Failed to fetch skybox\n";
            //else std::cout << "Fetched skybox\n";
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.bits());

        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_2D, 0);
    }
    // END SKYBOX

    // glUseProgram(m_skybox_shader);
    // declareSkyboxUniforms();
    // glUseProgram(0);

    // Task 10: Set the texture.frag uniform for our texture
    glUseProgram(m_shader);

    for (int i = 0; i < filenames.size(); i++) {
        std::string uniformName = "txt[" + std::to_string(i) + "]";
        GLint loc = glGetUniformLocation(m_shader, uniformName.c_str());
        glUniform1i(loc, 20 + i);
    }

    glUseProgram(0);
}



