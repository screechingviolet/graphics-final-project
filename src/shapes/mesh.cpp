#include "mesh.h"
#include <iostream>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

void Mesh::updateMesh(std::string meshfile) {
    m_vertexData.clear();

    setVertexData(meshfile.c_str());
    num_triangles = m_vertexData.size()/6;
    if (hasAnimation) num_triangles = m_vertexData.size()/14;
    std::cout << num_triangles << std::endl;
}

// I referenced GPT and the cgltf readme to write this code
void Mesh::setVertexData(const char* meshfile) {
    // std::cout << "hi im here" << std::endl;
    cgltf_options options = {cgltf_file_type_glb, 0};
    cgltf_data* data = NULL;
    cgltf_result result = cgltf_parse_file(&options, meshfile, &data);
    // std::cout << "result about to be determined" << std::endl;
    std::vector<glm::vec3> vertices_temp, normals_temp;
    std::vector<glm::vec4> weights_temp;
    std::vector<glm::ivec4> joints_temp;
    std::vector<int> indices;
    if (result == cgltf_result_success)
    {
        result = cgltf_load_buffers(&options, data, meshfile);
        if (result != cgltf_result_success)
        {
            return;
        }
        // std::cout << "Hi" << std::endl;
        if (data->meshes_count != 1) return;
        if (data->skins_count > 1) return;
        // std::cout << "i have made it to here" << std::endl;

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
                    const glm::ivec4& weight = weights_temp[i];
                    m_vertexData.push_back(joint.x);
                    m_vertexData.push_back(joint.y);
                    m_vertexData.push_back(joint.z);
                    m_vertexData.push_back(joint.w);
                    m_vertexData.push_back(weight.x);
                    m_vertexData.push_back(weight.y);
                    m_vertexData.push_back(weight.z);
                    m_vertexData.push_back(weight.w);
                }
            };

            if (!indices.empty()) {
                // --- indexed mesh: push vertices using the index buffer ---
                for (int idx : indices) {
                    push_vertex(idx);
                }
            } else {
                // --- non-indexed mesh: assume vertices are already in triangle order ---
                for (int i = 0; i < (int)vertices_temp.size(); ++i) {
                    push_vertex(i);
                }
            }
        }

        // fill AnimState from cgltf_skin and data->animation if relevant
        /*
        class Anim {
        public:
            float m_duration; X
            std::vector<Bone> m_allBones;
            vector<int> m_rootId; // is this needed?
        };

        class AnimState {
        public:
            std::vector<glm::mat4> m_finalBoneMatrices; // write a function to compute these
            Anim m_animation;
            float m_currentTime; X
            float m_deltaTime; X
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
glm::mat4 boneTransform;

    // methods here to interpolate
};
        */
        std::cout << "are we animating? " << hasAnimation << "\n";
        if (hasAnimation) {
            m_meshAnim = AnimState({}, Anim(0, {}), 0, 0); // fill in info when possible
            cgltf_node* root = skin->skeleton;
            std::unordered_map<cgltf_node*, int> tempNodeToIdx;
            if (root == nullptr) std::cout << "No skeleton root here" << std::endl;
            else {
                std::cout << root << std::endl;
                // m_meshAnim.m_animation.m_rootId = skin->joints_count;
                tempNodeToIdx[root] = skin->joints_count;

            }

            float buf[16];
            glm::mat4 inv_bind;

            for (int i = 0; i < skin->joints_count; i++) {
                // std::cout << i << std::endl;
                bool res = cgltf_accessor_read_float(skin->inverse_bind_matrices, i, buf, 16);
                if (res == false) std::cout << "ERROR inv bind fetching did not work\n";
                inv_bind = glm::make_mat4(buf);
                m_meshAnim.m_animation.m_allBones.push_back(Bone(i, {}, {}, {}, inv_bind, -1));
                tempNodeToIdx[skin->joints[i]] = i;

                // skin->inverse_bind_matrices[i]

                // find root
                if (skin->joints[i] == root) {
                    std::cout << "Found root\n";
                    // m_meshAnim.m_animation.m_rootId = i; // maybe remove
                }
            }
            if (root != nullptr) {
                m_meshAnim.m_animation.m_allBones.push_back(Bone(skin->joints_count, {}, {}, {}, glm::mat4(1.0), -1));
            }

            // now fill in parents
            for (int i = 0; i < m_meshAnim.m_animation.m_allBones.size(); i++) {
                if (tempNodeToIdx.count(skin->joints[i]->parent) == 0) {
                    std::cout << "found root!" << skin->joints[i] << std::endl;
                } else m_meshAnim.m_animation.m_allBones[i].parent = tempNodeToIdx[skin->joints[i]->parent];
                if (skin->joints[i]->has_matrix) {
                    m_meshAnim.m_animation.m_allBones[i].m_boneTransform = glm::make_mat4(skin->joints[i]->matrix);
                } else {
                    glm::mat4 t, r, s;
                    t = r = s = glm::mat4(1.0);
                    if (skin->joints[i]->has_scale) {
                        s = glm::scale(s, glm::make_vec3(skin->joints[i]->scale));
                    }
                    if (skin->joints[i]->has_rotation) {
                        r = glm::mat4_cast(glm::quat(glm::make_vec4(skin->joints[i]->rotation)));
                    }
                    if (skin->joints[i]->has_translation) {
                        t = glm::translate(t, glm::make_vec3(skin->joints[i]->translation));
                    }
                    m_meshAnim.m_animation.m_allBones[i].m_boneTransform = t * r * s;
                }
            }

            std::cout << "animations" << data->animations_count << std::endl;
            if (data->animations_count == 0) {
                // be sad and leave
            } else {
                float buf[3];
                float buf2[4];
                KeyframeVec3 temp_vec;
                KeyframeQuaternion temp_quat;
                cgltf_animation* main_anim = &data->animations[0];
                int curr;
                cgltf_accessor *time_acc, *transform_acc;
                for (int i = 0; i < main_anim->channels_count; i++) {
                    std::cerr << i << std::endl;
                    curr = tempNodeToIdx[main_anim->channels[i].target_node];
                    switch (main_anim->channels[i].target_path) {
                    case cgltf_animation_path_type_translation:
                        time_acc = main_anim->channels[i].sampler->input;
                        transform_acc = main_anim->channels[i].sampler->output;
                        if (time_acc->count != transform_acc->count) std::cout << "ERROR: channels not playing nice" << std::endl;
                        for (int j = 0; j < time_acc->count; j++) {
                            cgltf_accessor_read_float(time_acc, j, &temp_vec.time, 1);
                            m_meshAnim.m_animation.m_duration = max(m_meshAnim.m_animation.m_duration, temp_vec.time);
                            cgltf_accessor_read_float(transform_acc, j, buf, 3);
                            temp_vec.transform = glm::make_vec3(buf);
                            m_meshAnim.m_animation.m_allBones[curr].m_translate.push_back(temp_vec);
                            std::cerr << "translate" << temp_vec.time << std::endl;
                        }
                        break;
                    case cgltf_animation_path_type_rotation:
                        time_acc = main_anim->channels[i].sampler->input;
                        transform_acc = main_anim->channels[i].sampler->output;
                        if (time_acc->count != transform_acc->count) std::cout << "ERROR: channels not playing nice" << std::endl;
                        for (int j = 0; j < time_acc->count; j++) {
                            cgltf_accessor_read_float(time_acc, j, &temp_quat.time, 1);
                            m_meshAnim.m_animation.m_duration = max(m_meshAnim.m_animation.m_duration, temp_quat.time);
                            cgltf_accessor_read_float(transform_acc, j, buf2, 4);
                            temp_quat.transform = glm::make_quat(buf2);
                            m_meshAnim.m_animation.m_allBones[curr].m_rotate.push_back(temp_quat);
                            std::cerr << "rotate" << temp_quat.time << std::endl;
                        }
                        break;
                    case cgltf_animation_path_type_scale:
                        time_acc = main_anim->channels[i].sampler->input;
                        transform_acc = main_anim->channels[i].sampler->output;
                        if (time_acc->count != transform_acc->count) std::cout << "ERROR: channels not playing nice" << std::endl;
                        for (int j = 0; j < time_acc->count; j++) {
                            cgltf_accessor_read_float(time_acc, j, &temp_vec.time, 1);
                            m_meshAnim.m_animation.m_duration = max(m_meshAnim.m_animation.m_duration, temp_vec.time);
                            cgltf_accessor_read_float(transform_acc, j, buf, 3);
                            temp_vec.transform = glm::make_vec3(buf);
                            m_meshAnim.m_animation.m_allBones[curr].m_scale.push_back(temp_vec);
                            std::cerr << "scale" << temp_vec.time << std::endl;
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

void Mesh::fillVec3FromAccessor(cgltf_accessor* acc, std::vector<glm::vec3>& vertices) {
    cgltf_buffer_view* view = acc->buffer_view;
    cgltf_buffer* buf = view->buffer;

    uint8_t* raw_data = (uint8_t*)buf->data
                        + view->offset
                        + acc->offset;

    int component_count = cgltf_num_components(acc->type);
    int component_size = cgltf_component_size(acc->component_type);
    int stride = acc->stride ? acc->stride : component_count * component_size;

    for (int i = 0; i < acc->count; i++)
    {
        float* v = (float*)(raw_data + i * stride);
        glm::vec3 val = glm::vec3(v[0], v[1], v[2]);
        vertices.push_back(val);
    }
}

void Mesh::fillVec4FromAccessor(cgltf_accessor* acc, std::vector<glm::vec4>& vertices) {
    cgltf_buffer_view* view = acc->buffer_view;
    cgltf_buffer* buf = view->buffer;

    uint8_t* raw_data = (uint8_t*)buf->data
                        + view->offset
                        + acc->offset;

    int component_count = cgltf_num_components(acc->type);
    int component_size = cgltf_component_size(acc->component_type);
    int stride = acc->stride ? acc->stride : component_count * component_size;

    for (int i = 0; i < acc->count; i++)
    {
        float* v = (float*)(raw_data + i * stride);
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

