#ifndef SEASONCOLORGRADE_H
#define SEASONCOLORGRADE_H

#include "postprocessing/postprocess.h"
#include <QtGui/qimage.h>

constexpr int n_LUTs = 2;
constexpr GLuint TXTSLOTSTART = GL_TEXTURE16;
constexpr int SLOTSTART = 16;

class SeasonColorgrade : public PostProcess
{
public:
    SeasonColorgrade(std::array<std::string, n_LUTs> LUTs, std::array<int, n_LUTs> num_slices, int width, int height);
    void paintTexture() override;
    void setSeason(float season) { m_season = std::max(0.f, std::min(1.f, season)); }
    static std::string frag_shader;

private:
    std::array<QImage, n_LUTs> m_LUT_images;
    std::array<GLuint, n_LUTs> m_LUT_textures;
    std::array<int, n_LUTs> m_num_slices;

    float m_season = 0.f;
};

#endif // SEASONCOLORGRADE_H
