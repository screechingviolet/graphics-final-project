#pragma once

#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>


#include "cgltf.h"

struct KeyframeVec3 {
    float time;
    glm::vec3 transform;
};

struct KeyframeQuaternion {
    float time;
    glm::quat transform;
};

class Bone {
public:
    int m_index;
    std::vector<KeyframeVec3> m_translate;
    std::vector<KeyframeQuaternion> m_rotate;
    std::vector<KeyframeVec3> m_scale;
    glm::mat4 m_toBoneSpace; // from mesh space to bone space, so relevant transforms can be applied
    // but the final boneMatrix will be applied to a vertex's object space position
    int parent;
    glm::mat4 m_boneTransform = glm::mat4(1.0);
    glm::mat4 m_constParentTransform = glm::mat4(1.0);

    // methods here to interpolate
    glm::vec3 interpolateScale(float timestep, float duration);
    glm::quat interpolateRotate(float timestep, float duration);
    glm::vec3 interpolateTranslate(float timestep, float duration);
    int searchInKeyframeQuat(float timestep, std::vector<KeyframeQuaternion>& find);
    int searchInKeyframeVec3(float timestep, std::vector<KeyframeVec3>& find);
    float interpolateFactor(float prevTime, float nextTime, float timestep, float duration);

    Bone(int index,
         const std::vector<KeyframeVec3>& translate,
         const std::vector<KeyframeQuaternion>& rotate,
         const std::vector<KeyframeVec3>& scale,
         const glm::mat4& toBoneSpace,
         int parentIdx)
        : m_index(index),
        m_translate(translate),
        m_rotate(rotate),
        m_scale(scale),
        m_toBoneSpace(toBoneSpace),
        parent(parentIdx),
        m_boneTransform(1.0f),
        m_constParentTransform(1.0f)
    {}
};

class Anim {
public:
    Anim()
        : m_duration(0), m_allBones()
    {}
    Anim(float duration, std::vector<Bone> allBones)
        : m_duration(duration), m_allBones(allBones)
    {}
    float m_duration;
    // int m_ticksPerSec;
    std::vector<Bone> m_allBones;
    // int m_rootId;
    // std::unordered_map<int, Bone> m_idxToBone;
};

class AnimState {
public:
    std::vector<glm::mat4> m_finalBoneMatrices;
    std::vector<bool> m_visited;
    Anim m_animation;
    float m_currentTime;
    float m_deltaTime;
    AnimState()
        : m_finalBoneMatrices(),
        m_visited(),
        m_animation(),
        m_currentTime(0.0f),
        m_deltaTime(0.0f)
    {}

    AnimState(std::vector<glm::mat4> finalBones,
              std::vector<bool> visited,
              Anim animation,
              float currentTime,
              float deltaTime)
        : m_finalBoneMatrices(std::move(finalBones)),
        m_visited(std::move(visited)),
        m_animation(std::move(animation)),
        m_currentTime(currentTime),
        m_deltaTime(deltaTime)
    {}
};

class Mesh
{
public:
    void updateMesh(std::string meshfile);
    std::vector<float> generateShape() { return m_vertexData; }
    int num_triangles = 0;
    bool hasAnimation = false;
    AnimState m_meshAnim;
    void updateFinalBoneMatrices(float timestep);


private:
    std::vector<float> m_vertexData;
    void setVertexData(const char* meshfile);
    void fillVec3FromAccessor(cgltf_accessor* acc, std::vector<glm::vec3>& vertices);
    void fillVec4FromAccessor(cgltf_accessor* acc, std::vector<glm::vec4>& vertices);
    void filliVec4FromAccessor(cgltf_accessor* acc, std::vector<glm::ivec4>& vertices);
    glm::mat4 transformForBone(int bone, float timestep);
    glm::mat4 getLocalTransformPreprocessing(cgltf_node* node);
    glm::mat4 getMatrixFromTRS(glm::vec3 t, glm::quat r, glm::vec3 s);
};
