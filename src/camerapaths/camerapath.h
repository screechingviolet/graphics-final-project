#ifndef CAMERAPATH_H
#define CAMERAPATH_H
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <optional>
#include <vector>

struct PosRot {
    glm::vec3 pos;
    glm::quat rot;
};

struct Keyframe {
    PosRot posRot;
    float time;
};

class CameraPath
{
public:
    CameraPath(std::vector<Keyframe> keyframes);
    std::optional<PosRot> get(float t);

private:
    std::pair<PosRot, PosRot> getControlPoints(Keyframe& Kprev, Keyframe& K, Keyframe& Knext, Keyframe& Knextnext);
    PosRot DeCasteljau(PosRot& P1, PosRot& P2, PosRot& P3, PosRot& P4, float t);

    std::vector<Keyframe> m_keyframes;
};

#endif // CAMERAPATH_H
