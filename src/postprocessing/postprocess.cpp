#include "postprocess.h"
#include "utils/shaderloader.h"

std::vector<GLfloat> PostProcess::fullscreen_quad_data =
    { //     POSITIONS    //
        -1.f,  1.f, 0.0f,
        0.f, 1.f,
        -1.f, -1.f, 0.0f,
        0.f, 0.f,
        1.f, -1.f, 0.0f,
        1.f, 0.f,
        1.f,  1.f, 0.0f,
        1.f, 1.f,
        -1.f,  1.f, 0.0f,
        0.f, 1.f,
        1.f, -1.f, 0.0f,
        1.f, 0.f

};

PostProcess::PostProcess(std::string frag_shader, int width, int height, std::string vertex_shader) {
    m_shader = ShaderLoader::createShaderProgram(vertex_shader.c_str(), frag_shader.c_str());
    // m_shader = ShaderLoader::createShaderProgram(":/resources/shaders/texture.vert", ":/resources/shaders/texture.frag");
    m_frag_shader = frag_shader;
    m_fbo_width = width;
    m_fbo_height = height;

    // Generate and bind a VBO and a VAO for a fullscreen quad
    glGenBuffers(1, &m_fullscreen_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_fullscreen_vbo);
    glBufferData(GL_ARRAY_BUFFER, fullscreen_quad_data.size()*sizeof(GLfloat), fullscreen_quad_data.data(), GL_STATIC_DRAW);
    glGenVertexArrays(1, &m_fullscreen_vao);
    glBindVertexArray(m_fullscreen_vao);

    // Task 14: modify the code below to add a second attribute to the vertex attribute array
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), reinterpret_cast<void*>(3*sizeof(GLfloat)));

    // Unbind the fullscreen quad's VBO and VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    makeFBO();
    m_start_time = std::chrono::high_resolution_clock::now();
}

void PostProcess::makeFBO() {
    // Task 19: Generate and bind an empty texture, set its min/mag filter interpolation, then unbind
    glGenTextures(1, &m_fbo_texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_fbo_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_fbo_width, m_fbo_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Task 20: Generate and bind a renderbuffer of the right size, set its format, then unbind
    glGenRenderbuffers(1, &m_fbo_renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_fbo_renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_fbo_width, m_fbo_height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // Task 18: Generate and bind an FBO
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // Task 21: Add our texture as a color attachment, and our renderbuffer as a depth+stencil attachment, to our FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fbo_texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_fbo_renderbuffer);

    // Task 22: Unbind the FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // std::cout << "end" << std::endl;
}

float PostProcess::getTime() {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> elapsed = now - m_start_time;
    return elapsed.count();
}

void PostProcess::updateRes(int width, int height) {
    m_fbo_width = width;
    m_fbo_height = height;
}

GLuint PostProcess::getShader() {
    return m_shader;
}

GLuint PostProcess::getTexture() {
    return m_fbo_texture;
}

GLuint PostProcess::getFramebuffer() {
    return m_fbo;
}

void PostProcess::paintTexture() {
    //glUseProgram(m_shader);
    GLuint txt_location = glGetUniformLocation(m_shader, "txt");
    glUniform1i(txt_location, 0);
    glUniform1f(glGetUniformLocation(getShader(), "time"), getTime());

    glBindVertexArray(m_fullscreen_vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_fbo_texture);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

void PostProcess::destroyFBO() {
    glDeleteTextures(1, &m_fbo_texture);
    glDeleteRenderbuffers(1, &m_fbo_renderbuffer);
    glDeleteFramebuffers(1, &m_fbo);
}

void PostProcess::destroyVertex() {
    glDeleteBuffers(1, &m_fullscreen_vbo);
    glDeleteVertexArrays(1, &m_fullscreen_vao);
}
