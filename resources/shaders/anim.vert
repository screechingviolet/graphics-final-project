#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in ivec4 joints;
layout(location = 3) in vec4 weights;

out vec3 world_position;
out vec3 world_normal;

uniform mat4 model;
uniform mat4 model_inv_trans;

uniform mat4 view;
uniform mat4 proj;

void main() {
    // compute the world-space position and normal, then pass them to the fragment shader
    vec4 temp_pos = vec4(position, 1.0);
    vec4 new_pos = vec4(model * temp_pos);
    world_position = new_pos.xyz;
    vec4 new_norm = (model_inv_trans * vec4(normal, 0.0));
    new_norm.w = 0;
    new_norm = normalize(new_norm);

    world_normal = new_norm.xyz;

    // set gl_Position to the object space position transformed to clip space
    new_pos = vec4(proj * view * model * temp_pos);
    gl_Position = new_pos;
}
