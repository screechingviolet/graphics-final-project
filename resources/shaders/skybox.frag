#version 330 core

out vec4 fragColor;
in vec3 texture_coords;

uniform samplerCube skybox_txt;

void main() {
    // fragColor = vec4(1.0);
    fragColor = texture(skybox_txt, texture_coords);
}
