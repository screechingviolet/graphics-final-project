#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 joints; // changed from ivec
layout(location = 3) in vec4 weights;

out vec3 world_position;
out vec3 world_normal;

uniform mat4 model;
uniform mat4 model_inv_trans;

uniform mat4 view;
uniform mat4 proj;
uniform mat4 finalBoneMatrices[100];
uniform int animating;
uniform int numBones;

void main() {
    // compute the world-space position and normal, then pass them to the fragment shader


    vec4 temp_pos = vec4(position, 1.0);
    vec4 temp_normal = vec4(normal, 0.0);

    // modify based on bones
    if (animating == 1 && numBones > 0 && joints[0] < numBones && joints[1] < numBones && joints[2] < numBones && joints[3] < numBones) {
        if (weights[0] != 0 || weights[1] != 0 || weights[2] != 0 || weights[3] != 0) {
            // normalize?
        temp_pos = weights[0] * (finalBoneMatrices[int(joints[0])] * temp_pos)
                    + weights[1] * (finalBoneMatrices[int(joints[1])] * temp_pos)
                    + weights[2] * (finalBoneMatrices[int(joints[2])] * temp_pos)
                    + weights[3] * (finalBoneMatrices[int(joints[3])] * temp_pos);
        }
        temp_pos[3] = 1.0;
        // do normal next
        temp_normal = (weights[0] * finalBoneMatrices[int(joints[0])]
                + weights[0] * finalBoneMatrices[int(joints[0])]
                + weights[0] * finalBoneMatrices[int(joints[0])]
                + weights[0] * finalBoneMatrices[int(joints[0])]) * temp_normal;
    }


    vec4 new_pos = vec4(model * temp_pos);

    world_position = new_pos.xyz;


    vec4 new_norm = (model_inv_trans * temp_normal);
    new_norm.w = 0;
    new_norm = normalize(new_norm);

    world_normal = new_norm.xyz;

    // set gl_Position to the object space position transformed to clip space
    new_pos = vec4(proj * view * model * temp_pos);
    gl_Position = new_pos;
}
