#version 330 core

in vec2 uv;

uniform sampler2D txt;
uniform sampler2D tLUT1;
uniform float slices1;
uniform sampler2D tLUT2;
uniform float slices2;
uniform float alpha;

out vec4 fragColor;

void main()
{
    vec4 origColor = texture(txt, uv);

    float xOffset1 = 1.0 / slices1;
    float maxSlice1 = slices1 - 1.0;
    float npWidth1 = 1.0 / (slices1 * slices1);
    float npHeight1 = 1.0 / slices1;
    float npHalfWidth1 = npWidth1 * 0.5;
    float npHalfHeight1 = npHeight1 * 0.5;

    float x1 = (origColor.r * (xOffset1 - (npWidth1))) + npHalfWidth1;
    float y1 = (origColor.g * (1.0 - npHeight1)) + npHalfHeight1;

    float slice1 = origColor.b * maxSlice1;
    float SB1 = floor(slice1);
    float ST1 = ceil(slice1);
    float SM1 = fract(slice1);

    vec3 colB1 = texture(tLUT1, vec2((xOffset1 * SB1) + x1, y1)).rgb;
    vec3 colT1 = texture(tLUT1, vec2((xOffset1 * ST1) + x1, y1)).rgb;
    vec3 colF1 = mix(colB1, colT1, SM1);

//===================SECOND=LUT=============================

    float xOffset2 = 1.0 / slices2;
    float maxSlice2 = slices2 - 1.0;
    float npWidth2 = 1.0 / (slices2 * slices2);
    float npHeight2 = 1.0 / slices2;
    float npHalfWidth2 = npWidth2 * 0.5;
    float npHalfHeight2 = npHeight2 * 0.5;

    float x2 = (origColor.r * (xOffset2 - (npWidth2))) + npHalfWidth2;
    float y2 = (origColor.g * (1.0 - npHeight2)) + npHalfHeight2;

    float slice2 = origColor.b * maxSlice2;
    float SB2 = floor(slice2);
    float ST2 = ceil(slice2);
    float SM2 = fract(slice2);

    vec3 colB2 = texture(tLUT2, vec2((xOffset2 * SB2) + x2, y2)).rgb;
    vec3 colT2 = texture(tLUT2, vec2((xOffset2 * ST2) + x2, y2)).rgb;
    vec3 colF2 = mix(colB2, colT2, SM2);

    fragColor = vec4(alpha * colF2 + (1 - alpha) * colF1, 1.0);
    // fragColor = vec4(origColor.r * 0.5, origColor.g * 0.5, origColor.b * 0.5, 1);
}
