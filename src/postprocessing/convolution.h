#ifndef CONVOLUTION_H
#define CONVOLUTION_H

#include <vector>
#include "postprocessing/postprocess.h"
class Convolution : public PostProcess
{
public:
    Convolution(std::vector<float> kernel, float offset, int width, int height, std::string frag_shader = ":/resources/shaders/convolution.frag");
    void paintTexture() override;
    static std::string frag_shader;
    static constexpr int KERNEL_SIZE = 9;

private:
    std::vector<float> m_kernel;
    float m_offset;
};

#endif // CONVOLUTION_H
