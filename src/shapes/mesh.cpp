#include "mesh.h"
#include <iostream>
#include <glm/gtx/string_cast.hpp>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

int Bone::searchInKeyframeQuat(float timestep, std::vector<KeyframeQuaternion> &find) {
    // find max index where timestep >= find[index].time
    // uses binary search
    int l = 0, r = find.size()-1, m;
    int maxless = -1;
    while (l <= r) {
        m = (l+r)/2;
        if (timestep < find[m].time) {
            r = m - 1;
        } else if (timestep >= find[m].time) {
            maxless = std::max(m, maxless);
            l = m + 1;
        }
    }
    return maxless;
}

int Bone::searchInKeyframeVec3(float timestep, std::vector<KeyframeVec3>& find) {
    int l = 0, r = find.size()-1, m;
    int maxless = -1;
    while (l <= r) {
        m = (l+r)/2;
        if (timestep < find[m].time) {
            r = m - 1;
        } else if (timestep >= find[m].time) {
            maxless = std::max(m, maxless);
            l = m + 1;
        }
    }
    return maxless;
}

float Bone::interpolateFactor(float prevTime, float nextTime, float timestep, float duration) {
    float factor = 0.0;
    float timestepdiff, totaldiff;
    if (prevTime > nextTime) {
        totaldiff = nextTime + (duration - prevTime);
    } else totaldiff = nextTime - prevTime;
    if (prevTime > timestep) {
        timestepdiff = timestep + (duration - prevTime);
    } else timestepdiff = timestep - prevTime;
    return (timestepdiff)/(totaldiff);
}

glm::vec3 Bone::interpolateScale(float timestep, float duration) {
    if (m_scale.size() == 0) return glm::vec3(1.0);

    int idx = (searchInKeyframeVec3(timestep, m_scale)+m_scale.size())%m_scale.size();
    int nextIdx = (idx + 1)%m_scale.size(); // plus the value mod the value to get safe index
    float factor = interpolateFactor(m_scale[idx].time, m_scale[nextIdx].time, timestep, duration);
    glm::vec3 scaling = glm::mix(m_scale[idx].transform, m_scale[nextIdx].transform, factor);
    return scaling;
    // return glm::scale(glm::mat4(1.0f), scaling);
}

glm::quat Bone::interpolateRotate(float timestep, float duration) {
    if (m_rotate.size() == 0) return glm::quat(0, 0, 0, 1.0); // is this actually the default? irrelevant now

    int idx = (searchInKeyframeQuat(timestep, m_rotate)+m_rotate.size())%m_rotate.size();
    int nextIdx = (idx + 1)%m_rotate.size(); // plus the value mod the value to get safe index
    float factor = interpolateFactor(m_rotate[idx].time, m_rotate[nextIdx].time, timestep, duration);

    // uses teh wrogn order (Need to load transforms in glm format?)
    // convert transform to glm format, then slerp, then convert back before returning
    glm::quat rotation = glm::slerp(m_rotate[idx].transform, m_rotate[nextIdx].transform, factor);
    return rotation; // normalize?
    // return glm::toMat4(glm::normalize(rotation));
}

glm::vec3 Bone::interpolateTranslate(float timestep, float duration) {
    if (m_translate.size() == 0) return glm::vec3(0.0);

    int idx = (searchInKeyframeVec3(timestep, m_translate)+m_translate.size())%m_translate.size();
    int nextIdx = (idx + 1)%m_translate.size(); // plus the value mod the value to get safe index
    float factor = interpolateFactor(m_translate[idx].time, m_translate[nextIdx].time, timestep, duration);
    glm::vec3 translation = glm::mix(m_translate[idx].transform, m_translate[nextIdx].transform, factor);
    return translation;
    // return glm::translate(glm::mat4(1.0f), translation);
}

void Mesh::updateMesh(std::string meshfile) {
    m_vertexData.clear();

    setVertexData(meshfile.c_str());
    num_triangles = m_vertexData.size()/6;
    if (hasAnimation) num_triangles = m_vertexData.size()/14;
    std::cout << num_triangles << std::endl;
}

glm::mat4 Mesh::getLocalTransformPreprocessing(cgltf_node* node) {
    cgltf_float out[16];
    cgltf_node_transform_local(node, out);
    return glm::make_mat4(out);
}

// I referenced GPT and the cgltf readme to write this function
void Mesh::setVertexData(const char* meshfile) {
    cgltf_options options = {cgltf_file_type_glb, 0};
    cgltf_data* data = NULL;
    cgltf_result result = cgltf_parse_file(&options, meshfile, &data);
    std::vector<glm::vec3> vertices_temp, normals_temp;
    std::vector<glm::vec4> weights_temp;
    std::vector<glm::ivec4> joints_temp;
    std::vector<int> indices;
    if (result == cgltf_result_success)
    {
        result = cgltf_load_buffers(&options, data, meshfile);
        if (result != cgltf_result_success) {
            return;
        }
        if (data->meshes_count != 1) return;
        if (data->skins_count > 1) return;

        cgltf_mesh* mesh = &data->meshes[0];

        cgltf_skin* skin = nullptr;
        if (data->skins_count > 0) {
            skin = &data->skins[0];
        }
        cgltf_primitive* prim;
        cgltf_accessor *pos_acc = nullptr, *norm_acc = nullptr, *joints_acc = nullptr, *weights_acc = nullptr;
        for (int i = 0; i < mesh->primitives_count; i++) {
            prim = &mesh->primitives[i];
            vertices_temp.clear();
            normals_temp.clear();
            indices.clear();

            for (int j = 0; j < prim->attributes_count; j++) {
                std::cout << "ATTR: " << prim->attributes[j].name << "\n";
                switch (prim->attributes[j].type) {
                case cgltf_attribute_type_position:
                    pos_acc = prim->attributes[j].data;
                    break;
                case cgltf_attribute_type_normal:
                    norm_acc = prim->attributes[j].data;
                    break;
                case cgltf_attribute_type_joints:
                    joints_acc = prim->attributes[j].data;
                    break;
                case cgltf_attribute_type_weights:
                    weights_acc = prim->attributes[j].data;
                    break;
                default:
                    break;
                }
            }

            fillVec3FromAccessor(pos_acc, vertices_temp);
            fillVec3FromAccessor(norm_acc, normals_temp);
            if (joints_acc != nullptr && weights_acc != nullptr && skin != nullptr) {
                hasAnimation = true;
                std::cout << "Found animation skeleton\n";
                fillVec4FromAccessor(weights_acc, weights_temp);
                filliVec4FromAccessor(joints_acc, joints_temp);
            }

            // fetch here for primitive
            cgltf_accessor* idx = prim->indices;

            int count  = idx->count;
            // int stride = cgltf_component_size(idx->component_type);

            uint8_t* raw = (uint8_t*)idx->buffer_view->buffer->data +
                           idx->buffer_view->offset +
                           idx->offset;

            indices.reserve(count);

            for (int i = 0; i < count; i++) {
                uint32_t index = 0;  // output index is always 32-bit for your engine
                switch (idx->component_type) {
                case cgltf_component_type_r_8u:     // UNSIGNED_BYTE
                    index = ((uint8_t*)raw)[i];
                    break;
                case cgltf_component_type_r_16u:    // UNSIGNED_SHORT
                    index = ((uint16_t*)raw)[i];
                    break;
                case cgltf_component_type_r_32u:    // UNSIGNED_INT
                    index = ((uint32_t*)raw)[i];
                    break;
                default:
                    // glTF ONLY allows 8/16/32-bit unsigned ints for indices.
                    // If we reach here, something is wrong.
                    fprintf(stderr, "Unsupported index component type.\n");
                    break;
                }
                indices.push_back(index);
            }

            m_vertexData.clear();
            m_vertexData.reserve((indices.empty() ? vertices_temp.size() : indices.size()) * 6);

            auto push_vertex = [&](int i) {
                const glm::vec3& p = vertices_temp[i];
                const glm::vec3& n = normals_temp[i];
                m_vertexData.push_back(p.x);
                m_vertexData.push_back(p.y);
                m_vertexData.push_back(p.z);
                m_vertexData.push_back(n.x);
                m_vertexData.push_back(n.y);
                m_vertexData.push_back(n.z);
                if (hasAnimation) {
                    const glm::vec4& joint = joints_temp[i];
                    const glm::vec4& weight = weights_temp[i];
                    m_vertexData.push_back(joint.x);
                    m_vertexData.push_back(joint.y);
                    m_vertexData.push_back(joint.z);
                    m_vertexData.push_back(joint.w);
                    m_vertexData.push_back(weight.x);
                    m_vertexData.push_back(weight.y);
                    m_vertexData.push_back(weight.z);
                    m_vertexData.push_back(weight.w);
                    // std::cout << weight.x << " " << weight.y << " " << weight.z << " " << weight.w << std::endl;
                }
            };

            if (!indices.empty()) {
                std::cout << "indexed mesh\n";
                // --- indexed mesh: push vertices using the index buffer ---
                for (int idx : indices) {
                    push_vertex(idx);
                }
            } else {
                std::cout << "non-indexed mesh\n";
                // --- non-indexed mesh: assume vertices are already in triangle order ---
                for (int i = 0; i < (int)vertices_temp.size(); ++i) {
                    push_vertex(i);
                }
            }
        }

        if (hasAnimation) {
            m_meshAnim = AnimState({}, {}, Anim(0, {}), 0, 0);
            cgltf_node* root = skin->skeleton;
            std::unordered_map<cgltf_node*, int> tempNodeToIdx;
            if (root == nullptr) std::cout << "No skeleton root here" << std::endl;
            else {
                tempNodeToIdx[root] = skin->joints_count;
            }

            float buf[16];
            glm::mat4 inv_bind;

            for (int i = 0; i < skin->joints_count; i++) {
                // temp commented out
                bool res = cgltf_accessor_read_float(skin->inverse_bind_matrices, i, buf, cgltf_num_components(skin->inverse_bind_matrices->type));
                if (res == false) std::cout << "ERROR inv bind fetching did not work\n";

                inv_bind = glm::make_mat4(buf);
                m_meshAnim.m_animation.m_allBones.push_back(Bone(i, std::vector<KeyframeVec3>{}, std::vector<KeyframeQuaternion>{}, std::vector<KeyframeVec3>{}, inv_bind, -1));
                tempNodeToIdx[skin->joints[i]] = i;
            }
            // if (root != nullptr) {
                // COME BACK
                // m_meshAnim.m_animation.m_allBones.push_back(Bone(skin->joints_count, std::vector<KeyframeVec3>{}, std::vector<KeyframeQuaternion>{}, std::vector<KeyframeVec3>{}, glm::mat4(1.0), -1));
            // }

            // now fill in parents
            for (int i = 0; i < m_meshAnim.m_animation.m_allBones.size(); i++) {
                if (tempNodeToIdx.count(skin->joints[i]->parent) == 0) {
                    std::cout << "found root!" << skin->joints[i] << " " << skin->joints[i]->parent << std::endl;
                    // there is some kind of parent node that has its own transform
                } else m_meshAnim.m_animation.m_allBones[i].parent = tempNodeToIdx[skin->joints[i]->parent];
                // turn this into a function that takes a cgltf_node* and returns its bonetransform
                // run it for each skin->joints[i]
                // then if the parent of hte joint is not in the joints map, iterate up until no more parent and multiply the matrix as you go

                m_meshAnim.m_animation.m_allBones[i].m_boneTransform = getLocalTransformPreprocessing(skin->joints[i]);
                cgltf_node* curr = skin->joints[i];
                while (curr->parent != nullptr && tempNodeToIdx.count(curr->parent) == 0) {
                    std::cout << "multiplying\n";
                    curr = curr->parent;
                    m_meshAnim.m_animation.m_allBones[i].m_constParentTransform = getLocalTransformPreprocessing(curr) * m_meshAnim.m_animation.m_allBones[i].m_constParentTransform;
                }
                // std::cout << "are equal: " << glm::to_string(m_meshAnim.m_animation.m_allBones[i].m_constParentTransform * m_meshAnim.m_animation.m_allBones[i].m_boneTransform) << "\n" <<  glm::to_string(glm::inverse(m_meshAnim.m_animation.m_allBones[i].m_toBoneSpace)) << '\n';
            }

            std::cout << "animations" << data->animations_count << std::endl;
            if (data->animations_count == 0) {
                // be sad and leave
                // hasAnimation = false;
            } else {
                float buf[3];
                float buf2[4];
                KeyframeVec3 temp_vec;
                KeyframeQuaternion temp_quat;
                cgltf_animation* main_anim = &data->animations[0];
                int curr;
                cgltf_accessor *time_acc, *transform_acc;
                for (int i = 0; i < main_anim->channels_count; i++) {
                    // std::cerr << i << std::endl;
                    curr = tempNodeToIdx[main_anim->channels[i].target_node];
                    std::cerr << main_anim->channels[i].sampler->interpolation << " type\n";
                    switch (main_anim->channels[i].target_path) {
                    case cgltf_animation_path_type_translation:
                        time_acc = main_anim->channels[i].sampler->input;
                        transform_acc = main_anim->channels[i].sampler->output;

                        if (time_acc->count != transform_acc->count) std::cout << "ERROR: channels not playing nice" << std::endl;
                        for (int j = 0; j < time_acc->count; j++) {
                            cgltf_accessor_read_float(time_acc, j, &temp_vec.time, 1);
                            m_meshAnim.m_animation.m_duration = std::max(m_meshAnim.m_animation.m_duration, temp_vec.time);
                            cgltf_accessor_read_float(transform_acc, j, buf, 3);
                            temp_vec.transform = glm::make_vec3(buf);
                            m_meshAnim.m_animation.m_allBones[curr].m_translate.push_back(temp_vec);
                        }
                        break;
                    case cgltf_animation_path_type_rotation:
                        time_acc = main_anim->channels[i].sampler->input;
                        transform_acc = main_anim->channels[i].sampler->output;
                        if (time_acc->count != transform_acc->count) std::cout << "ERROR: channels not playing nice" << std::endl;
                        for (int j = 0; j < time_acc->count; j++) {
                            cgltf_accessor_read_float(time_acc, j, &temp_quat.time, 1);
                            m_meshAnim.m_animation.m_duration = std::max(m_meshAnim.m_animation.m_duration, temp_quat.time);
                            cgltf_accessor_read_float(transform_acc, j, buf2, 4);
                            temp_quat.transform = glm::quat(buf2[3], buf2[0], buf2[1], buf2[2]);
                            m_meshAnim.m_animation.m_allBones[curr].m_rotate.push_back(temp_quat);
                        }
                        break;
                    case cgltf_animation_path_type_scale:
                        time_acc = main_anim->channels[i].sampler->input;
                        transform_acc = main_anim->channels[i].sampler->output;
                        if (time_acc->count != transform_acc->count) std::cout << "ERROR: channels not playing nice" << std::endl;
                        for (int j = 0; j < time_acc->count; j++) {
                            cgltf_accessor_read_float(time_acc, j, &temp_vec.time, 1);
                            m_meshAnim.m_animation.m_duration = std::max(m_meshAnim.m_animation.m_duration, temp_vec.time);
                            cgltf_accessor_read_float(transform_acc, j, buf, 3);
                            temp_vec.transform = glm::make_vec3(buf);
                            m_meshAnim.m_animation.m_allBones[curr].m_scale.push_back(temp_vec);
                        }
                        break;
                    default:
                        break;
                    }
                }

                m_meshAnim.m_deltaTime = 0.0416; // 1/24, approximately
            }
        }


        cgltf_free(data);
    }
}

glm::mat4 Mesh::getMatrixFromTRS(glm::vec3 t, glm::quat r, glm::vec3 s) {
    // THIS CODE IS TAKEN FROM cgltf_node_transform_local
    float lm[16];

    float tx = t[0];
    float ty = t[1];
    float tz = t[2];

    float qx = r.x;
    float qy = r.y;
    float qz = r.z;
    float qw = r.w;

    float sx = s[0];
    float sy = s[1];
    float sz = s[2];

    lm[0] = (1 - 2 * qy*qy - 2 * qz*qz) * sx;
    lm[1] = (2 * qx*qy + 2 * qz*qw) * sx;
    lm[2] = (2 * qx*qz - 2 * qy*qw) * sx;
    lm[3] = 0.f;

    lm[4] = (2 * qx*qy - 2 * qz*qw) * sy;
    lm[5] = (1 - 2 * qx*qx - 2 * qz*qz) * sy;
    lm[6] = (2 * qy*qz + 2 * qx*qw) * sy;
    lm[7] = 0.f;

    lm[8] = (2 * qx*qz + 2 * qy*qw) * sz;
    lm[9] = (2 * qy*qz - 2 * qx*qw) * sz;
    lm[10] = (1 - 2 * qx*qx - 2 * qy*qy) * sz;
    lm[11] = 0.f;

    lm[12] = tx;
    lm[13] = ty;
    lm[14] = tz;
    lm[15] = 1.f;

    return glm::make_mat4(lm);
}

glm::mat4 Mesh::transformForBone(int bone, float timestep) {
    // need to better understand the process
    // return m_meshAnim.m_finalBoneMatrices[bone] = glm::inverse(m_meshAnim.m_animation.m_allBones[bone].m_toBoneSpace); // temp
    if (m_meshAnim.m_visited[bone]) return m_meshAnim.m_finalBoneMatrices[bone];
    else m_meshAnim.m_visited[bone] = true;
    Bone* relevant = &m_meshAnim.m_animation.m_allBones[bone];
    glm::mat4 localTransform;
    if (relevant->m_rotate.size() == 0 && relevant->m_scale.size() == 0 && relevant->m_translate.size() == 0) {
        localTransform = relevant->m_boneTransform;
    } else {
        localTransform = getMatrixFromTRS(relevant->interpolateTranslate(timestep, m_meshAnim.m_animation.m_duration),
                                        relevant->interpolateRotate(timestep, m_meshAnim.m_animation.m_duration),
                                        relevant->interpolateScale(timestep, m_meshAnim.m_animation.m_duration));
    }
    glm::mat4 worldBone;

    if (relevant->parent != -1) worldBone = transformForBone(relevant->parent, timestep) * localTransform;
    else {
        worldBone = relevant->m_constParentTransform * localTransform;
    }
    m_meshAnim.m_finalBoneMatrices[bone] = worldBone;

    return worldBone;

}

void Mesh::updateFinalBoneMatrices(float timestep) {
    // run this at every deltaTime
    // go through nodes, traverse to parents recursively to get final transform matrix
    // create a visited array
    m_meshAnim.m_finalBoneMatrices.clear();
    m_meshAnim.m_visited.clear(); // time complexity of clear()?
    for (int i = 0; i < m_meshAnim.m_animation.m_allBones.size(); i++) {
        m_meshAnim.m_finalBoneMatrices.push_back(glm::mat4(1.0));
        m_meshAnim.m_visited.push_back(false);
    }
    for (int i = 0; i < m_meshAnim.m_visited.size(); i++) {
        transformForBone(i, timestep);
    }
    for (int i = 0; i < m_meshAnim.m_visited.size(); i++) {
        m_meshAnim.m_finalBoneMatrices[i] = m_meshAnim.m_finalBoneMatrices[i] * m_meshAnim.m_animation.m_allBones[i].m_toBoneSpace;
    }
    // multiply all by inv bind matrices

}

void Mesh::fillVec3FromAccessor(cgltf_accessor* acc, std::vector<glm::vec3>& vertices) {
    for (int i = 0; i < acc->count; i++) {
        cgltf_float v[3];
        bool res = cgltf_accessor_read_float(acc, i, v, 3);
        if (res == false) std::cout << "oops did not work\n";
        glm::vec3 val = glm::vec3(v[0], v[1], v[2]);
        vertices.push_back(val);
    }
}

void Mesh::fillVec4FromAccessor(cgltf_accessor* acc, std::vector<glm::vec4>& vertices) {
    for (int i = 0; i < acc->count; i++) {
        cgltf_float v[4];
        bool res = cgltf_accessor_read_float(acc, i, v, 4);
        if (res == false) std::cout << "filling vec4 failed\n";
        glm::vec4 val = glm::vec4(v[0], v[1], v[2], v[3]);
        vertices.push_back(val);
    }
}

void Mesh::filliVec4FromAccessor(cgltf_accessor* acc, std::vector<glm::ivec4>& vertices) {
    for (int i = 0; i < acc->count; i++) {
        cgltf_uint v[4];
        bool res = cgltf_accessor_read_uint(acc, i, v, 4);
        if (res == false) std::cout << "oops did not work\n";
        glm::ivec4 val = glm::ivec4(v[0], v[1], v[2], v[3]);
        vertices.push_back(val);
    }
}

