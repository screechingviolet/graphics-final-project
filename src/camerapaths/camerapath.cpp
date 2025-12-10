#include "camerapath.h"
#include <stdexcept>

CameraPath::CameraPath(std::vector<Keyframe> keyframes) {
    m_keyframes = keyframes;
}

std::optional<PosRot> CameraPath::get(float t) {
    Keyframe prev;
    Keyframe curr;
    Keyframe next;
    Keyframe nextnext;

    if (m_keyframes.size() <= 1) {
        throw std::runtime_error("camera path must contain at least 2 keyframes!");
    }
    if (t < m_keyframes[0].time) {
        return std::nullopt;
    }
    if (t >= m_keyframes[m_keyframes.size() - 1].time) {
        return std::nullopt;
    }

    int i = 0;
    while (t >= m_keyframes[i].time) {
        i++;
    }

    // i cannot be 0 or m_keyframes.size() because of the above checks
    // i should point to the end of the segment
    next = m_keyframes[i];
    curr = m_keyframes[i - 1];
    if (i == 1) {
        prev = m_keyframes[i - 1]; // if at the beginning segment, then set prev keyframe to the first keyframe
    } else {
        prev = m_keyframes[i - 2];
    }

    if (i == m_keyframes.size() - 1) {
        nextnext = next;
    }
    else {
        nextnext = m_keyframes[i + 1];
    }

    auto [C1, C2] = getControlPoints(prev, curr, next, nextnext);
    float interpTime = (t - curr.time) / (next.time - curr.time);
    return DeCasteljau(curr.posRot, C1, C2, next.posRot, interpTime);
}


std::pair<PosRot, PosRot> CameraPath::getControlPoints(Keyframe& Kprev, Keyframe& K, Keyframe& Knext, Keyframe& Knextnext) {
    // position
    float PR;
    if (Kprev.time == K.time || Knext.time == K.time) {
        PR = 1;
    } else {
        PR = (Knext.time - K.time) / (K.time - Kprev.time);
    }
    float PRnext;
    if (K.time == Knext.time || Knextnext.time == Knext.time) {
        PRnext = 1;
    } else {
        PRnext = (Knextnext.time - Knext.time) / (Knext.time - K.time);
    }

    glm::vec3 Rpos = glm::mix(Kprev.posRot.pos, K.posRot.pos, 1.f + PR);
    glm::vec3 Tpos = glm::mix(Rpos, Knext.posRot.pos, 0.5f);
    glm::vec3 C1pos = glm::mix(K.posRot.pos, Tpos, 1.f/3.f);

    glm::vec3 RposNext = glm::mix(K.posRot.pos, Knext.posRot.pos, 1.f + PRnext);
    glm::vec3 TposNext = glm::mix(RposNext, Knextnext.posRot.pos, 0.5f);
    glm::vec3 C1posNext = glm::mix(Knext.posRot.pos, TposNext, 1.f/3.f);
    glm::vec3 C2pos = glm::mix(Knext.posRot.pos, C1posNext, -1.f/PRnext);

    // rotation
    glm::quat Rrot = glm::slerp(Kprev.posRot.rot, K.posRot.rot, 1.f + PR);
    glm::quat Trot = glm::slerp(Rrot, Knext.posRot.rot, 0.5f);
    glm::quat C1rot = glm::slerp(K.posRot.rot, Trot, 1.f/3.f);

    glm::quat RrotNext = glm::slerp(K.posRot.rot, Knext.posRot.rot, 1.f + PRnext);
    glm::quat TrotNext = glm::slerp(RrotNext, Knextnext.posRot.rot, 0.5f);
    glm::quat C1rotNext = glm::slerp(Knext.posRot.rot, TrotNext, 1.f/3.f);
    glm::quat C2rot = glm::slerp(Knext.posRot.rot, C1rotNext, -1.f/PRnext);

    return {{C1pos, C1rot}, {C2pos, C2rot}};
}

PosRot CameraPath::DeCasteljau(PosRot& P1, PosRot& P2, PosRot& P3, PosRot& P4, float t) {

    // level 1 interpolation
    glm::vec3 L1pos0 = glm::mix(P1.pos, P2.pos, t);
    glm::vec3 L1pos1 = glm::mix(P2.pos, P3.pos, t);
    glm::vec3 L1pos2 = glm::mix(P3.pos, P4.pos, t);

    glm::quat L1rot0 = glm::slerp(P1.rot, P2.rot, t);
    glm::quat L1rot1 = glm::slerp(P2.rot, P3.rot, t);
    glm::quat L1rot2 = glm::slerp(P3.rot, P4.rot, t);

    // level 2 interpolation
    glm::vec3 L2pos0 = glm::mix(L1pos0, L1pos1, t);
    glm::vec3 L2pos1 = glm::mix(L1pos1, L1pos2, t);

    glm::quat L2rot0 = glm::slerp(L1rot0, L1rot1, t);
    glm::quat L2rot1 = glm::slerp(L1rot1, L1rot2, t);

    // level 3 interpolation
    glm::vec3 L3pos = glm::mix(L2pos0, L2pos1, t);
    glm::quat L3rot = glm::slerp(L2rot0, L2rot1, t);

    return {L3pos, L3rot};

}
