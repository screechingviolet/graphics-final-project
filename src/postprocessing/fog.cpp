#include "fog.h"
#include <iostream>
#include <QDebug>
#include <QDir>
#include <QImageReader>

std::string Fog::frag_shader = ":/resources/shaders/fog.frag";

Fog::Fog(int density, int width, int height)
    : PostProcess(frag_shader, width, height)
{
    // initialize the color grading variables (LUTs, etc)
    m_density = density;

    // Task 19: Generate and bind an empty texture, set its min/mag filter interpolation, then unbind
    glGenTextures(1, &m_fbo_depthTexture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_fbo_depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, m_fbo_width, m_fbo_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Attach depth texture instead of renderbuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_fbo_depthTexture, 0);

    // (Optional but recommended if not using stencil)
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Fog::paintTexture() {
    glUseProgram(getShader());
    // set the color grading related uniforms
    GLuint density_location = glGetUniformLocation(getShader(), "density");
    glUniform1f(density_location, m_density);
    GLuint LUT_location = glGetUniformLocation(getShader(), "tLUT");
    glUniform1i(LUT_location, 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_LUT_texture);
    glUseProgram(0);

    PostProcess::paintTexture();

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
}
