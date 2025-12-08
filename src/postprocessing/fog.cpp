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

    glGenTextures(1, &m_LUT_texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_LUT_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_LUT_image.width(), m_LUT_image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, m_LUT_image.bits());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

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
