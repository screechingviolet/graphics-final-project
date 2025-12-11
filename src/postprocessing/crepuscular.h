#ifndef CREPUSCULAR_H
#define CREPUSCULAR_H

#include "postprocess.h"
#include "../camera.h"
#include "../utils/sceneparser.h"
#include <glm/glm.hpp>

class Crepuscular : public PostProcess
{
public:
    Crepuscular(int width, int height,
                Camera* camera,
                RenderData* renderData,
                glm::mat4* projMatrix);
    void paintTexture() override;
    void updateCameraAndScene(Camera* camera, RenderData* renderData, glm::mat4* projMatrix);
    static std::string frag_shader;

private:
    GLuint m_depthTexture;
    Camera* m_camera;
    RenderData* m_renderData;
    glm::mat4* m_projMatrix;

    // Crepuscular ray parameters
    float m_exposure = 0.1f;
    float m_decay = 0.975f;
    float m_density = 0.4f;
    float m_weight = 0.2f;
    int m_numSamples = 200;
};

#endif // CREPUSCULAR_H
