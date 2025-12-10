#version 330 core

out vec4 fragColor;
in vec3 texture_coords;

uniform samplerCube skybox_txt;
uniform samplerCube skybox_txt2;
uniform float interp;

void main() {
    // fragColor = vec4(1.0);
    fragColor = interp * texture(skybox_txt2, texture_coords) + (1-interp) * texture(skybox_txt, texture_coords);
}
