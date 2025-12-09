#ifndef FOG_H
#define FOG_H

#include <QCoreApplication>
#include <QtGui/qimage.h>
#include "postprocess.h"

class Fog : public PostProcess
{
public:
    Fog(float density, int width, int height);
    void paintTexture() override;
    static std::string frag_shader;

private:
    QImage m_LUT_image;
    GLuint m_LUT_texture;
    int m_density;

    GLuint m_fbo_depthTexture;
    GLuint m_fbo_width;
    GLuint m_fbo_height;

};

#endif // FOG_H
