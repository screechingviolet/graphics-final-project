#include "colorgrade.h"
#include <iostream>
#include <QDebug>
#include <QDir>
#include <QImageReader>

std::string Colorgrade::frag_shader = ":/resources/shaders/colorgrading.frag";

Colorgrade::Colorgrade(std::string LUT, int num_slices, int width, int height, bool mirror)
    : PostProcess(frag_shader, width, height)
{
    // initialize the color grading variables (LUTs, etc)
    m_num_slices = num_slices;
    QString LUT_filepath = QString(LUT.c_str());
    m_LUT_image = QImage(LUT_filepath);

    QImageReader reader(LUT.c_str());

    qDebug() << "format:" << reader.format();
    qDebug() << "size:" << reader.size();
    qDebug() << "canRead:" << reader.canRead();

    QImage img = reader.read();
    qDebug() << "img null:" << img.isNull();
    qDebug() << "reader error:" << reader.error() << reader.errorString();
    qDebug() << "QImage format:" << m_LUT_image.format();


    qDebug() << "Images in resource:" << QDir(":/resources/images").entryList();

    if (mirror) {
        m_LUT_image = m_LUT_image.convertToFormat(QImage::Format_RGBA8888).mirrored();
    } else {
        m_LUT_image = m_LUT_image.convertToFormat(QImage::Format_RGBA8888);
    }
    glGenTextures(1, &m_LUT_texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_LUT_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_LUT_image.width(), m_LUT_image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, m_LUT_image.bits());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Task 19: Generate and bind an empty texture, set its min/mag filter interpolation, then unbind
    /*glGenTextures(1, &m_fbo_depthTexture);
    glActiveTexture(GL_TEXTURE20);
    glBindTexture(GL_TEXTURE_2D, m_fbo_depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, getFramebuffer());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_fbo_depthTexture, 0);

    //glDrawBuffer(GL_NONE);
    //glReadBuffer(GL_NONE);

    // After setting up FBO, check what's wrong:
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer is not complete: ";
        switch(status) {
        case GL_FRAMEBUFFER_UNDEFINED:
            std::cerr << "GL_FRAMEBUFFER_UNDEFINED" << std::endl;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT" << std::endl;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT" << std::endl;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER" << std::endl;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER" << std::endl;
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            std::cerr << "GL_FRAMEBUFFER_UNSUPPORTED" << std::endl;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE" << std::endl;
            break;
        default:
            std::cerr << "Unknown error " << status << std::endl;
            break;
        }
    }
*/
    //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_fbo_depthTexture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Colorgrade::paintTexture() {
    glUseProgram(getShader());
    // set the color grading related uniforms
    GLuint slices_location = glGetUniformLocation(getShader(), "slices");
    glUniform1f(slices_location, m_num_slices);
    GLuint LUT_location = glGetUniformLocation(getShader(), "tLUT");
    glUniform1i(LUT_location, 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_LUT_texture);
    //glUseProgram(0);

    //glUseProgram(getShader());
    // set the color grading related uniforms
    //GLuint depth_texture_location = glGetUniformLocation(getShader(), "depthTexture");
    //glUniform1i(depth_texture_location, 20);
    //glActiveTexture(GL_TEXTURE20);
    //glBindTexture(GL_TEXTURE_2D, m_fbo_depthTexture);
    //glUseProgram(0);

    PostProcess::paintTexture();

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
}
