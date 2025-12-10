#version 330 core

in vec3 world_position;
in vec3 world_normal;
in vec2 uv_coord;

out vec4 fragColor;

uniform float ka;
uniform float kd;
uniform vec3 lightColors[8];
uniform int lightsNum;
uniform int lightTypes[8];
uniform vec3 lightFunctions[8];
uniform vec4 lightPositions[8];
uniform vec4 lightDirections[8];
uniform float lightAngles[8];
uniform float lightPenumbras[8];

uniform float ks;
uniform vec4 camPos;
uniform float shininess;
uniform float blend;
uniform vec3 shapeColorA;
uniform vec3 shapeColorD;
uniform vec3 shapeColorS;

uniform sampler2D txt[8];
uniform sampler2D noiseMap;
uniform bool usingTexture;
uniform int txtIndex;
uniform bool isScrolling;
uniform float time;

void main() {
    // texture(txt, uv_coord);
    fragColor = vec4(0.0);
    vec4 surfaceToLight, reflectionvec;
    float constantsdiffuse, constantsspecular;
    vec4 norm = normalize(vec4(world_normal, 0.0));
    vec4 directionToCamera = normalize(camPos-vec4(world_position, 1.0));
    float falloff, curr_angle;
    float inner, outer, inten;
    float dist, f_att = 1.;
    vec4 position = vec4(world_position, 1.0);
    fragColor[0] += ka*shapeColorA[0];
    fragColor[1] += ka*shapeColorA[1];
    fragColor[2] += ka*shapeColorA[2];
    vec4 temp_tex;

    for (int i = 0; i < 8; i++) {
        if (i >= lightsNum) continue;

        if (lightTypes[i] == 1) {
            surfaceToLight = normalize(-lightDirections[i]);
        }
        if (lightTypes[i] == 0 || lightTypes[i] == 2) {
            surfaceToLight = normalize(lightPositions[i] - position);
        }

        inten = 1;
        if (lightTypes[i] == 2) {
            inner = lightAngles[i]-lightPenumbras[i];
            outer = lightAngles[i];
            curr_angle = acos(dot(surfaceToLight, normalize(-lightDirections[i])));
            if ((curr_angle - inner)/(outer-inner) == 0) falloff = 0.;
            else falloff = (-2. * pow((curr_angle - inner)/(outer-inner), 3.)) + (3. * pow((curr_angle - inner)/(outer-inner), 2.));

            if (curr_angle > inner && curr_angle <= outer) {
                inten = max(0.0, 1.0 - falloff);
            } else if (curr_angle > outer) {
                inten = 0.;
            }
        }

        f_att = 1.;
        if (lightTypes[i] != 1) {
            dist = length(lightPositions[i] - position);
            f_att = min(1., 1./(lightFunctions[i].x + dist * lightFunctions[i].y + lightFunctions[i].z * (dist*dist)));
        }

        constantsdiffuse = inten * f_att * max(dot(norm, surfaceToLight), 0.f);
        if (usingTexture) {
            temp_tex = texture(txt[txtIndex], uv_coord);

            if (isScrolling) {
                vec2 uv = uv_coord;
                vec2 realuv = uv_coord;
                uv.x += sin(time * 0.1 + uv.y * 1.0) * 0.01;
                uv.y += cos(time * 0.1 + uv.y * 1.0) * 0.03; // scrolling

                vec4 displacement_tex = texture(noiseMap, 0.2 * uv);
                realuv.x += displacement_tex[0];
                realuv.y += displacement_tex[1];
                temp_tex = texture(txt[txtIndex], realuv);

            } else {
                temp_tex = texture(txt[txtIndex], uv_coord);
            }

            fragColor[0] += lightColors[i].r * constantsdiffuse * (blend*(temp_tex[0]) + (1-blend)*(kd * shapeColorD[0]));
            fragColor[1] += lightColors[i].g * constantsdiffuse * (blend*(temp_tex[1]) + (1-blend)*(kd * shapeColorD[1]));
            fragColor[2] += lightColors[i].b * constantsdiffuse * (blend*(temp_tex[2]) + (1-blend)*(kd * shapeColorD[2]));

        } else {
            fragColor[0] += kd * constantsdiffuse * lightColors[i].r * shapeColorD[0];
            fragColor[1] += kd * constantsdiffuse * lightColors[i].g * shapeColorD[1];
            fragColor[2] += kd * constantsdiffuse * lightColors[i].b * shapeColorD[2];
        }

        reflectionvec = -normalize(surfaceToLight - (2. * (dot(surfaceToLight, norm)) * (norm)));
        float dotprod = dot(directionToCamera, reflectionvec);
        if (dotprod == 0) constantsspecular = 0.;
        else if (shininess == 0) constantsspecular = inten * f_att * ks * 1;
        else constantsspecular = inten * f_att * ks * pow(max(dotprod, 0.0f), max(0.0, shininess));
        fragColor[0] += constantsspecular * lightColors[i][0] * shapeColorS[0];
        fragColor[1] += constantsspecular * lightColors[i][1] * shapeColorS[1];
        fragColor[2] += constantsspecular * lightColors[i][2] * shapeColorS[2];
    }

    fragColor.r = min(max(fragColor.r, 0.0), 1.0);
    fragColor.g = min(max(fragColor.g, 0.0), 1.0);
    fragColor.b = min(max(fragColor.b, 0.0), 1.0);

    fragColor.a = 1.0;

/*
    if (usingTexture) {
        if (isScrolling) {
            vec2 uv = uv_coord;
            vec2 realuv = uv_coord;
            uv.x += sin(time * 0.2 + uv.y * 1.0) * 0.03;
            uv.y += cos(time * 0.2 + uv.y * 1.0) * 0.05; // scrolling

            vec4 displacement_tex = texture(noiseMap, 0.2 * uv);
            realuv.x += displacement_tex[0];
            realuv.y += displacement_tex[1];
            temp_tex = texture(txt[txtIndex], realuv);
            fragColor = temp_tex;

        } else {
            temp_tex = texture(txt[txtIndex], uv_coord);
            fragColor = temp_tex;
        }

    }*/
    //fragColor = vec4(10*uv_coord, 0, 1);
}
