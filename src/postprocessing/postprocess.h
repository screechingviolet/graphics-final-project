#ifndef POSTPROCESS_H
#define POSTPROCESS_H

#include "GL/glew.h"
#include <string>
#include <vector>

class PostProcess
{
public:
    PostProcess(std::string frag_shader, int width, int height, std::string vertex_shader = ":/resources/shaders/texture.vert");
    void makeFBO();
    void updateRes(int width, int height);
    GLuint getShader();
    GLuint getTexture();
    GLuint getFramebuffer();
    virtual void paintTexture();
    void destroyFBO();
    void destroyVertex();

    static std::vector<GLfloat> fullscreen_quad_data;

    virtual ~PostProcess() {}

private:
    int m_fbo_width;
    int m_fbo_height;
    GLuint m_shader;
    GLuint m_fbo;
    GLuint m_fbo_texture;
    GLuint m_fbo_renderbuffer;
    GLuint m_fullscreen_vbo;
    GLuint m_fullscreen_vao;
    std::string m_frag_shader;

    std::chrono::high_resolution_clock::time_point m_start_time;
    float getTime();



};

#endif // POSTPROCESS_H
