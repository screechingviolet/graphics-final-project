#ifndef COLORGRADE_H
#define COLORGRADE_H

#include <QCoreApplication>
#include <QtGui/qimage.h>
#include "postprocess.h"

class Colorgrade : public PostProcess
{
public:
    Colorgrade(std::string LUT, int num_slices, int width, int height, bool mirror = false);
    void paintTexture() override;
    static std::string frag_shader;

private:
    QImage m_LUT_image;
    GLuint m_LUT_texture;
    int m_num_slices;
};

#endif // COLORGRADE_H
