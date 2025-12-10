#version 330 core
in vec2 uv;

uniform sampler2D txt;
uniform sampler2D depthTexture;
uniform float density;

out vec4 fragColor;

float LinearizeDepth(float z)
{
    float n = 0.1;  // near plane
    float f = 100.0; // far plane
    return (2.0 * n) / (f + n - z * (f - n));
}

void main()
{
    vec4 sceneColor = texture(txt, uv);
    //float depth = LinearizeDepth(texture(depthTexture, uv).r);
    vec4 depth = texture(depthTexture, uv);
    float fogMaxDist = 8.0;
    float fogMinDist = 0.1;
    vec4  fogColor = vec4(0.4, 0.4, 0.4, 1.0);

    // Calculate fog
    //float fogFactor = (fogMaxDist - depth) / (fogMaxDist - fogMinDist);
    //fogFactor = clamp(fogFactor, 0.0, 1.0);

    //fragColor = mix(fogColor, sceneColor, fogFactor);
    fragColor = 100 * vec4(depth.r);
    //fragColor = sceneColor;
}
