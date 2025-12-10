#include "seasoncolorgrade.h"

std::string SeasonColorgrade::frag_shader = ":/resources/shaders/seasoncolorgrading.frag";

SeasonColorgrade::SeasonColorgrade(std::array<std::string, n_LUTs> LUTs, std::array<int, n_LUTs> num_slices, int width, int height)
    : PostProcess(frag_shader, width, height)
{

    m_num_slices = num_slices;
    for (int i = 0; i < n_LUTs; i++) {
        std::string LUT = LUTs[i];
        int num_slice = num_slices[i];
        GLuint texture_slot = TXTSLOTSTART + i;

        QString LUT_filepath = QString(LUT.c_str());
        m_LUT_images[i] = QImage(LUT_filepath);
        m_LUT_images[i] = m_LUT_images[i].convertToFormat(QImage::Format_RGBA8888);

        glGenTextures(1, &m_LUT_textures[i]);
        glActiveTexture(texture_slot);
        glBindTexture(GL_TEXTURE_2D, m_LUT_textures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_LUT_images[i].width(), m_LUT_images[i].height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, m_LUT_images[i].bits());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void SeasonColorgrade::paintTexture() {
    // represents float values for winter, spring, summer, fall
    // eps to guarantee that m_season is > first val and < last val
    float eps = 0.0001;
    float season_floats[4] = {0.f - eps, 1.f/3.f, 2.f/3.f, 1.f + eps};

    int i = 0;
    while (m_season > season_floats[i]) i++;
    int szn1 = i - 1; // i > 0 since m_season > season_floats[0]
    int szn2 = i;
    float alpha = (m_season - season_floats[szn1]) / (season_floats[szn2] - season_floats[szn1]);

    glUseProgram(getShader());

    glUniform1f(glGetUniformLocation(getShader(), "alpha"), alpha);

    glUniform1f(glGetUniformLocation(getShader(), "slices1"), m_num_slices[szn1]);
    glUniform1f(glGetUniformLocation(getShader(), "slices2"), m_num_slices[szn2]);
    glUniform1i(glGetUniformLocation(getShader(), "tLUT1"), SLOTSTART + szn1);
    glUniform1i(glGetUniformLocation(getShader(), "tLUT2"), SLOTSTART + szn2);

    glActiveTexture(TXTSLOTSTART + szn1);
    glBindTexture(GL_TEXTURE_2D, m_LUT_textures[szn1]);

    glActiveTexture(TXTSLOTSTART + szn2);
    glBindTexture(GL_TEXTURE_2D, m_LUT_textures[szn2]);

    glUseProgram(0);
    PostProcess::paintTexture();

    glActiveTexture(TXTSLOTSTART + szn1);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(TXTSLOTSTART + szn2);
    glBindTexture(GL_TEXTURE_2D, 0);




}
