#version 330 core
layout (location = 2) in vec3 billboard; // <individual vertices>
layout (location = 3) in vec4 posAndSize; // <vec3 position, scalar size>
layout (location = 4) in vec4 color; // <rgba>
layout (location = 5) in vec2 uv;

//out vec3 worldSpacePosition;
out vec4 particleColor;
out vec2 UVCoords;

//uniform mat4 projection;
//uniform vec2 offset;
//uniform vec4 color;

// Task 6: declare a uniform mat4 to store model matrix
uniform mat4 modelMatrix;

// Task 7: declare uniform mat4's for the view and projection matrix
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main()
{
    float size = posAndSize[3];
    vec3 pos = vec3(posAndSize);
    vec3 position = pos + size * billboard;
    particleColor = color;
    //UVCoords = posAndSize.xy + vec2(0.5);
    UVCoords = uv;

    vec3 camRight = vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
    vec3 camUp = vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);

    vec3 billboardPos = pos + camRight * billboard[0] * size + camUp * billboard[1] * size;

    // Task 8: compute the world-space position and normal, then pass them to
    //         the fragment shader using the variables created in task 5
    vec3 worldSpacePosition = vec3(modelMatrix * vec4(billboardPos, 1));

    // Task 9: set gl_Position to the object space position transformed to clip space
    //gl_Position = projectionMatrix * viewMatrix * vec4(worldSpacePosition, 1);
    gl_Position = projectionMatrix * viewMatrix * vec4(worldSpacePosition, 1);
}
