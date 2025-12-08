#version 330 core
in vec2 uv;

uniform sampler2D txt;
uniform sampler2D depthTexture;
uniform float density;

out vec4 fragColor;

void main()
{
    vec4 sceneColor = texture(txt, uv);
    float depth = texture(depthTexture, uv).r;
    float fogMaxDist = 8.0;
    float fogMinDist = 0.1;
    vec4  fogColor = vec4(0.4, 0.4, 0.4, 1.0);

    // Calculate fog
    float fogFactor = (fogMaxDist - depth) / (fogMaxDist - fogMinDist);
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    fragColor = mix(fogColor, sceneColor, fogFactor);
    fragColor = vec4(depth);
}
