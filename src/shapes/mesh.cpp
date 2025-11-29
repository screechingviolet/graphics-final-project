#include "mesh.h"
#include <iostream>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

void Mesh::updateMesh(std::string meshfile) {
    m_vertexData.clear();

    setVertexData(meshfile.c_str());
    num_triangles = m_vertexData.size()/6;
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
        // std::cout << "i have made it to here" << std::endl;

        cgltf_mesh* mesh = &data->meshes[0];
        cgltf_primitive* prim;
        cgltf_accessor *pos_acc, *norm_acc;
        for (int i = 0; i < mesh->primitives_count; i++) {
            prim = &mesh->primitives[i];
            vertices_temp.clear();
            normals_temp.clear();
            indices.clear();

            for (int j = 0; j < prim->attributes_count; j++) {
                switch (prim->attributes[j].type) {
                case cgltf_attribute_type_position:
                    pos_acc = prim->attributes[i].data;
                    break;
                case cgltf_attribute_type_normal:
                    norm_acc = prim->attributes[i].data;
                    break;
                default:
                    break;
                }

            }

            fillVecFromAccessor(pos_acc, vertices_temp);
            fillVecFromAccessor(norm_acc, normals_temp);

            // fetch here for primitive
            cgltf_accessor* idx = prim->indices;

            int count  = idx->count;
            // int stride = cgltf_component_size(idx->component_type);

            uint8_t* raw = (uint8_t*)idx->buffer_view->buffer->data +
                           idx->buffer_view->offset +
                           idx->offset;

            indices.reserve(count);

            for (int i = 0; i < count; i++)
            {
                uint32_t index = 0;  // output index is always 32-bit for your engine

                switch (idx->component_type)
                {
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
            m_vertexData.reserve(
                (indices.empty() ? vertices_temp.size() : indices.size()) * 6
                );

            auto push_vertex = [&](int i)
            {
                const glm::vec3& p = vertices_temp[i];
                const glm::vec3& n = normals_temp[i];

                m_vertexData.push_back(p.x);
                m_vertexData.push_back(p.y);
                m_vertexData.push_back(p.z);

                m_vertexData.push_back(n.x);
                m_vertexData.push_back(n.y);
                m_vertexData.push_back(n.z);
            };

            if (!indices.empty())
            {
                // --- indexed mesh: push vertices using the index buffer ---
                for (int idx : indices)
                {
                    push_vertex(idx);
                }
            }
            else
            {
                // --- non-indexed mesh: assume vertices are already in triangle order ---
                for (int i = 0; i < (int)vertices_temp.size(); ++i)
                {
                    push_vertex(i);
                }
            }



        }

        cgltf_free(data);
    }
}

void Mesh::fillVecFromAccessor(cgltf_accessor* acc, std::vector<glm::vec3>& vertices) {
    // std::cerr << "hiiii";
    cgltf_buffer_view* view = acc->buffer_view;
    // std::cerr << "hiiii";
    cgltf_buffer* buf = view->buffer;
    // std::cerr << buf->data;

    uint8_t* raw_data = (uint8_t*)buf->data
                        + view->offset
                        + acc->offset;

    // std::cerr << "I madef it here dsGkjsdJGH";

    int component_count = cgltf_num_components(acc->type);  // POSITION = 3
    int component_size = cgltf_component_size(acc->component_type); // float = 4
    int stride = acc->stride ? acc->stride : component_count * component_size;

    for (int i = 0; i < acc->count; i++)
    {
        // std::cerr << "I madef it here";
        float* v = (float*)(raw_data + i * stride);
        // std::cerr << v;
        glm::vec3 val = glm::vec3(v[0], v[1], v[2]);
        // std::cerr << "I madef it here";
        vertices.push_back(val);
    }
}

