#version 330 core

// Task 5: declare "in" variables for the world-space position and normal,
//         received post-interpolation from the vertex shader
in vec3 world_position;
in vec3 world_normal;

// Task 10: declare an out vec4 for your output color
out vec4 fragColor;

// Task 12: declare relevant uniform(s) here, for ambient lighting
uniform float ka;

// Task 13: declare relevant uniform(s) here, for diffuse lighting
uniform float kd;
uniform vec4 lightPos;

uniform vec3 lightColors[8];
uniform int lightsNum;
uniform int lightTypes[8];
uniform vec3 lightFunctions[8];
uniform vec4 lightPositions[8];
uniform vec4 lightDirections[8];
uniform float lightAngles[8];
uniform float lightPenumbras[8];

// Task 14: declare relevant uniform(s) here, for specular lighting
uniform float ks;
uniform vec4 camPos;
uniform float shininess;

void main() {
    // Remember that you need to renormalize vectors here if you want them to be normalized

    // Task 10: set your output color to white (i.e. vec4(1.0)). Make sure you get a white circle!
    // fragColor = vec4(1.0);

    // Task 11: set your output color to the absolute value of your world-space normals,
    //          to make sure your normals are correct.
    // fragColor = vec4(abs(world_normal), 1.0);

    fragColor = vec4(0.0);

    // Task 12: add ambient component to output color
    fragColor += ka;

    // Task 13: add diffuse component to output color
    vec4 surfaceToLight = normalize(lightPos-vec4(world_position, 0.0));
    vec4 norm = normalize(vec4(world_normal, 0.0));
    fragColor += kd*clamp(dot(norm, surfaceToLight), 0, 1);

    // Task 14: add specular component to output color
    vec4 reflection = normalize(reflect(-surfaceToLight, norm));
    fragColor = fragColor + ks*pow(clamp(dot(reflection, normalize(camPos-vec4(world_position, 1.0))), 0, 1), shininess);

    fragColor.a = 1.0;
}
