#version 330 core
//in vec3 worldSpacePosition;
in vec4 particleColor;
in vec2 UVCoords;

out vec4 color;

uniform sampler2D sprite;

void main()
{
    //color = (texture(sprite, TexCoords) * ParticleColor);
    vec4 p = vec4(Position, 1.0);
    float d = dot(p, p);
    float alpha = 1;

    if (d >= 1) {
        discard;
    } else if (sqrt(d) >= 0.6) {
        alpha = 1 - ((sqrt(d) - 0.6) / 0.4);
    }

    // smooth circular edge
    //alpha = 1.0 - smoothstep(0.5, 1.0, sqrt(d));

    color = vec4(particleColor.rgb, alpha);
    //color = vec4(alpha);
}
