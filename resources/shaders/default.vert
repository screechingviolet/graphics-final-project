#version 330 core

// Task 4: declare a vec3 object-space position variable, using
//         the `layout` and `in` keywords.
layout(location = 0) in vec3 objSpacePosition;
layout(location = 1) in vec3 objSpaceNormal;

// Task 5: declare `out` variables for the world-space position and normal,
//         to be passed to the fragment shader
out vec3 worldSpacePosition;
out vec3 worldSpaceNormal;

// Task 6: declare a uniform mat4 to store model matrix
uniform mat4 modelMatrix;

// Task 7: declare uniform mat4's for the view and projection matrix
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main() {
    // Task 8: compute the world-space position and normal, then pass them to
    //         the fragment shader using the variables created in task 5
    worldSpacePosition = vec3(modelMatrix * vec4(objSpacePosition, 1));
    worldSpaceNormal = inverse(transpose(mat3(modelMatrix))) * normalize(objSpaceNormal);

    // Recall that transforming normals requires obtaining the inverse-transpose of the model matrix!
    // In projects 5 and 6, consider the performance implications of performing this here.

    // Task 9: set gl_Position to the object space position transformed to clip space
    gl_Position = projectionMatrix * viewMatrix * vec4(worldSpacePosition, 1);
}
