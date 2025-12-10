#ifndef PARTICLES_H
#define PARTICLES_H

#include <unordered_map>
#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QTime>
#include <QTimer>
#include "utils/sceneparser.h"
#include "camera.h"
#include "utils/miscutilities.h"

#endif // PARTICLES_H

class Particle {
public:
    glm::vec3 pos;
    glm::vec3 velocity;
    glm::vec4 color;
    float size;
    float angle;
    float weight;
    float life; // Remaining life of the particle. if < 0 : dead and unused.
    float cameraDistance; // Used for rendering order

    Particle(glm::vec3 eye) {
        for (int i = 0; i < 3; i++) {
            pos[i] = MiscUtilities::randomGen(-2, 2);
            velocity[i] = MiscUtilities::randomGen(-0.3, 0.3);
            //color[i] = MiscUtilities::randomGen(1, 1);
            color[i] = 0;
            //pos[i] = 1;
            //velocity[i] = 0;
            //color[i] = 0;
        }

        color[3] = 1;

        size = MiscUtilities::randomGen(0.03, 0.1);
        angle = 0; // what does this do lol
        weight = MiscUtilities::randomGen(0.5, 1);
        life = MiscUtilities::randomGen(4.0f, 7.0f);
        //size = 3;
        //weight = 1;
        //life = 4;
        cameraDistance = glm::length(pos - eye);
    }
};
