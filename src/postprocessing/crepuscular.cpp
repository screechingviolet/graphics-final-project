#include "crepuscular.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

std::string Crepuscular::frag_shader = ":/resources/shaders/crepuscular.frag";

Crepuscular::Crepuscular(int width, int height,
                         Camera* camera,
                         RenderData* renderData,
                         glm::mat4* projMatrix)
    : PostProcess(frag_shader, width, height)
    , m_camera(camera)
    , m_renderData(renderData)
    , m_projMatrix(projMatrix)
{
    // Create depth texture attachment
    glGenTextures(1, &m_depthTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Attach depth texture to FBO
    glBindFramebuffer(GL_FRAMEBUFFER, getFramebuffer());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_2D, m_depthTexture, 0);

    // Check framebuffer completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Crepuscular FBO not complete: " << status << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Crepuscular::updateCameraAndScene(Camera* camera, RenderData* renderData, glm::mat4* projMatrix) {
    m_camera = camera;
    m_renderData = renderData;
    m_projMatrix = projMatrix;
}

void Crepuscular::paintTexture() {
    glUseProgram(getShader());

    // Bind scene color texture to unit 0
    GLuint sceneTexLoc = glGetUniformLocation(getShader(), "sceneTex");
    glUniform1i(sceneTexLoc, 0);

    // Bind depth texture to unit 2
    GLuint depthTexLoc = glGetUniformLocation(getShader(), "depthTex");
    glUniform1i(depthTexLoc, 2);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);

    // Compute light screen-space position
    glm::vec3 lightWorldPos(0.0f);
    bool foundLight = false;

    // Prefer directional light, else use first light's position
    for (const SceneLightData &L : m_renderData->lights) {
        if (L.type == LightType::LIGHT_DIRECTIONAL) {
            glm::vec3 dir = glm::normalize(glm::vec3(L.dir));
            lightWorldPos = glm::vec3(m_camera->pos) - dir * 1000.0f;
            foundLight = true;
            break;
        }
    }
    if (!foundLight && !m_renderData->lights.empty()) {
        lightWorldPos = m_renderData->lights[0].pos;
        foundLight = true;
    }

    glm::vec2 lightScreenUV(0.5f, 0.5f);
    if (foundLight) {
        glm::vec4 clip = (*m_projMatrix) * m_camera->view * glm::vec4(lightWorldPos, 1.0f);
        if (clip.w != 0.0f) {
            glm::vec3 ndc = glm::vec3(clip) / clip.w;
            lightScreenUV = glm::vec2(ndc.x * 0.5f + 0.5f, ndc.y * 0.5f + 0.5f);
        }
    }

    glUniform2f(glGetUniformLocation(getShader(), "lightScreenPos"),
                lightScreenUV.x, lightScreenUV.y);

    // Set crepuscular ray parameters
    glUniform1f(glGetUniformLocation(getShader(), "exposure"), m_exposure);
    glUniform1f(glGetUniformLocation(getShader(), "decay"), m_decay);
    glUniform1f(glGetUniformLocation(getShader(), "density"), m_density);
    glUniform1f(glGetUniformLocation(getShader(), "weight"), m_weight);
    glUniform1i(glGetUniformLocation(getShader(), "numSamples"), m_numSamples);

    // Viewport size
    glUniform2f(glGetUniformLocation(getShader(), "screenSize"),
                float(m_fbo_width), float(m_fbo_height));

    // Compute facing factor
    glm::vec3 camForward = glm::normalize(glm::vec3(m_camera->look));
    glm::vec3 lightDir = glm::normalize(lightWorldPos - glm::vec3(m_camera->pos));
    float facing = glm::dot(camForward, lightDir);
    if (facing < 0.f) facing = 0.f;
    glUniform1f(glGetUniformLocation(getShader(), "facing"), facing);

    // Call parent's paint method to draw the fullscreen quad
    PostProcess::paintTexture();

    // Cleanup
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
}
