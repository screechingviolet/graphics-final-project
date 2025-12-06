#version 330 core

layout(location = 0) in vec3 position;

out vec3 texture_coords;

uniform mat4 view;
uniform mat4 proj;

void main() {
    texture_coords = position;
    gl_Position = proj * view * vec4(position, 1.0);
}
