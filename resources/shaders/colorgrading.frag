#version 330 core

in vec2 uv;

uniform sampler2D txt;
uniform sampler2D tLUT;
uniform sampler2D depthTexture; // temporary
uniform float slices;

out vec4 fragColor;

void main()
{
    //vec4 depth = texture(depthTexture, uv);
    vec4 origColor = texture(txt, uv);

    //fragColor = depth;

    float xOffset = 1.0 / slices;
    float maxSlice = slices - 1.0;
    float npWidth = 1.0 / (slices * slices);
    float npHeight = 1.0 / slices;
    float npHalfWidth = npWidth * 0.5;
    float npHalfHeight = npHeight * 0.5;

    float x = (origColor.r * (xOffset - (npWidth))) + npHalfWidth;
    float y = (origColor.g * (1.0 - npHeight)) + npHalfHeight;

    float slice = origColor.b * maxSlice;
    float SB = floor(slice);
    float ST = ceil(slice);
    float SM = fract(slice);

    vec3 colB = texture(tLUT, vec2((xOffset * SB) + x, y)).rgb;
    vec3 colT = texture(tLUT, vec2((xOffset * ST) + x, y)).rgb;
    vec3 colF = mix(colB, colT, SM);

    fragColor = vec4(colF, 1.0);
    // fragColor = vec4(origColor.r * 0.5, origColor.g * 0.5, origColor.b * 0.5, 1);

}
