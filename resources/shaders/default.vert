#version 330 core

// Task 4: declare a vec3 object-space position variable, using
//         the `layout` and `in` keywords.
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

// Task 5: declare `out` variables for the world-space position and normal,
//         to be passed to the fragment shader
out vec3 world_position;
out vec3 world_normal;

// Task 6: declare a uniform mat4 to store model matrix
uniform mat4 model;

// Task 7: declare uniform mat4's for the view and projection matrix
uniform mat4 view;
uniform mat4 proj;

void main() {
    // Task 8: compute the world-space position and normal, then pass them to
    //         the fragment shader using the variables created in task 5
    vec4 temp_pos = vec4(position, 1.0);
    vec4 new_pos = vec4(model * temp_pos);
    world_position = new_pos.xyz;
    world_normal = transpose(inverse(mat3(model))) * normalize(normal);

    // Recall that transforming normals requires obtaining the inverse-transpose of the model matrix!
    // In projects 5 and 6, consider the performance implications of performing this here.

    // Task 9: set gl_Position to the object space position transformed to clip space
    new_pos = vec4(proj * view * model * temp_pos);
    gl_Position = new_pos;
}
