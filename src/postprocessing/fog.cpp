// #include "fog.h"
// #include <iostream>
// #include <QDebug>
// #include <QDir>
// #include <QImageReader>

// std::string Fog::frag_shader = ":/resources/shaders/fog.frag";

// Fog::Fog(float density, int width, int height)
//     : PostProcess(frag_shader, width, height)
// {
//     std::cout << "start tjhois" << std::endl;
//     m_density = density;

//     // Task 19: Generate and bind an empty texture, set its min/mag filter interpolation, then unbind
//     glGenTextures(1, &m_fbo_depthTexture);
//     glActiveTexture(GL_TEXTURE2);
//     glBindTexture(GL_TEXTURE_2D, m_fbo_depthTexture);
//     glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_fbo_width, m_fbo_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//     glBindTexture(GL_TEXTURE_2D, 0);

//     glBindFramebuffer(GL_FRAMEBUFFER, getFramebuffer());
//     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_fbo_depthTexture, 0);
//     //glDrawBuffer(GL_NONE);
//     //glReadBuffer(GL_NONE);

//     // After setting up FBO, check what's wrong:
//     GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
//     if (status != GL_FRAMEBUFFER_COMPLETE) {
//         std::cerr << "Framebuffer is not complete: ";
//         switch(status) {
//         case GL_FRAMEBUFFER_UNDEFINED:
//             std::cerr << "GL_FRAMEBUFFER_UNDEFINED" << std::endl;
//             break;
//         case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
//             std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT" << std::endl;
//             break;
//         case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
//             std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT" << std::endl;
//             break;
//         case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
//             std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER" << std::endl;
//             break;
//         case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
//             std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER" << std::endl;
//             break;
//         case GL_FRAMEBUFFER_UNSUPPORTED:
//             std::cerr << "GL_FRAMEBUFFER_UNSUPPORTED" << std::endl;
//             break;
//         case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
//             std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE" << std::endl;
//             break;
//         default:
//             std::cerr << "Unknown error " << status << std::endl;
//             break;
//         }
//     }

//     //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_fbo_depthTexture, 0);
//     glBindFramebuffer(GL_FRAMEBUFFER, 0);
// }

// void Fog::paintTexture() {
//     //glEnable(GL_DEPTH_TEST);
//     //glDepthMask(GL_TRUE);
//     glUseProgram(getShader());
//     // set the color grading related uniforms
//     GLuint density_location = glGetUniformLocation(getShader(), "density");
//     glUniform1f(density_location, m_density);
//     GLuint depth_texture_location = glGetUniformLocation(getShader(), "depthTexture");
//     glUniform1i(depth_texture_location, 2);
//     glActiveTexture(GL_TEXTURE2);
//     glBindTexture(GL_TEXTURE_2D, m_fbo_depthTexture);
//     //glUseProgram(0);

//     PostProcess::paintTexture();

//     glActiveTexture(GL_TEXTURE2);
//     glBindTexture(GL_TEXTURE_2D, 0);
// }
