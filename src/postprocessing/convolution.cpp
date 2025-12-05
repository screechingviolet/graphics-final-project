#include "convolution.h"

Convolution::Convolution(std::vector<float> kernel, float offset, int width, int height, std::string frag_shader)
    : PostProcess(frag_shader, width, height)
{
    if (kernel.size() != 9) {
        throw std::runtime_error("Only 3x3 kernels are supported!");
    }
    m_kernel = kernel;
    m_offset = offset;

}

void Convolution::paintTexture() {
    float offsets[9][2] = {
        { -m_offset,  m_offset  },  // top-left
        {  0.0f,    m_offset  },  // top-center
        {  m_offset,  m_offset  },  // top-right
        { -m_offset,  0.0f    },  // center-left
        {  0.0f,    0.0f    },  // center-center
        {  m_offset,  0.0f    },  // center - right
        { -m_offset, -m_offset  },  // bottom-left
        {  0.0f,   -m_offset  },  // bottom-center
        {  m_offset, -m_offset  }   // bottom-right
    };
    glUseProgram(getShader());
    glUniform2fv(glGetUniformLocation(getShader(), "offsets"), 9, (float*)offsets);
    glUniform1fv(glGetUniformLocation(getShader(), "kernel"), 9, m_kernel.data());
    glUseProgram(0);
    PostProcess::paintTexture();
}
