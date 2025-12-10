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
uniform bool usingTexture;
uniform int txtIndex;

// Shadow uniforms
uniform int shadowType[8];
uniform mat4 lightSpace[8];
uniform sampler2D shadowMap[8];
uniform samplerCube shadowCube[8];
uniform float shadowBias;
uniform int enablePCF;
uniform float pointLightFar;

// Shadow calculation for 2D maps (directional/spot)
float calcShadow2D(int idx, vec4 worldPos4, vec3 normal, vec3 lightDir) {
    vec4 proj = lightSpace[idx] * worldPos4;
    if (proj.w == 0.0) return 1.0;
    proj /= proj.w;

    vec3 projCoords = proj.xyz * 0.5 + 0.5;

    // Outside frustum -> lit
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z < 0.0 || projCoords.z > 1.0) {
        return 1.0;
    }

    float currentDepth = projCoords.z;
    float bias = max(shadowBias * (1.0 - dot(normal, lightDir)), shadowBias * 0.5);

    if (enablePCF == 0) {
        float closestDepth = texture(shadowMap[idx], projCoords.xy).r;
        return (currentDepth - bias <= closestDepth) ? 1.0 : 0.0;
    }

    // PCF 3x3
    float shadow = 0.0;
    ivec2 texSize = textureSize(shadowMap[idx], 0);
    vec2 texelSize = 1.0 / vec2(texSize);

    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            float closestDepth = texture(shadowMap[idx], projCoords.xy + offset).r;
            if (currentDepth - bias <= closestDepth) shadow += 1.0;
        }
    }
    shadow /= 9.0;
    return shadow;
}

// Shadow calculation for point lights (cubemap)
float calcShadowPoint(int idx, vec3 fragPos, vec3 lightPos) {
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);

    float closestDepth = texture(shadowCube[idx], fragToLight).r;
    float storedDepth = closestDepth * pointLightFar;

    float bias = shadowBias * 0.5;
    return (currentDepth - bias <= storedDepth) ? 1.0 : 0.0;
}

// PCF for point lights
float calcShadowPointPCF(int idx, vec3 fragPos, vec3 lightPos) {
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    float bias = shadowBias * 0.5;

    vec3 dirs[5] = vec3[](
        fragToLight,
        normalize(fragToLight + vec3(0.1, 0.0, 0.0)),
        normalize(fragToLight + vec3(-0.1, 0.0, 0.0)),
        normalize(fragToLight + vec3(0.0, 0.1, 0.0)),
        normalize(fragToLight + vec3(0.0, -0.1, 0.0))
    );

    float visible = 0.0;
    for (int i = 0; i < 5; ++i) {
        float closestDepth = texture(shadowCube[idx], dirs[i]).r;
        float storedDepth = closestDepth * pointLightFar;
        if (currentDepth - bias <= storedDepth) visible += 1.0;
    }
    return visible / 5.0;
}

void main() {
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

        // Calculate shadow factor
        float shadowFactor = 1.0;
        if (shadowType[i] == 1 || shadowType[i] == 2) {
            // 2D shadow map (directional or spot)
            shadowFactor = calcShadow2D(i, position, world_normal, vec3(surfaceToLight));
        } else if (shadowType[i] == 3) {
            // Point light cubemap
            if (enablePCF == 0) {
                shadowFactor = calcShadowPoint(i, world_position, vec3(lightPositions[i]));
            } else {
                shadowFactor = calcShadowPointPCF(i, world_position, vec3(lightPositions[i]));
            }
        }

        constantsdiffuse = inten * f_att * max(dot(norm, surfaceToLight), 0.f);
        if (usingTexture) {
            temp_tex = texture(txt[txtIndex], uv_coord);
            fragColor[0] += lightColors[i].r * constantsdiffuse * (blend*(temp_tex[0]) + (1-blend)*(kd * shapeColorD[0])) * shadowFactor;
            fragColor[1] += lightColors[i].g * constantsdiffuse * (blend*(temp_tex[1]) + (1-blend)*(kd * shapeColorD[1])) * shadowFactor;
            fragColor[2] += lightColors[i].b * constantsdiffuse * (blend*(temp_tex[2]) + (1-blend)*(kd * shapeColorD[2])) * shadowFactor;
        } else {
            fragColor[0] += kd * constantsdiffuse * lightColors[i].r * shapeColorD[0] * shadowFactor;
            fragColor[1] += kd * constantsdiffuse * lightColors[i].g * shapeColorD[1] * shadowFactor;
            fragColor[2] += kd * constantsdiffuse * lightColors[i].b * shapeColorD[2] * shadowFactor;
        }

        reflectionvec = -normalize(surfaceToLight - (2. * (dot(surfaceToLight, norm)) * (norm)));
        float dotprod = dot(directionToCamera, reflectionvec);
        if (dotprod == 0) constantsspecular = 0.;
        else if (shininess == 0) constantsspecular = inten * f_att * ks * 1;
        else constantsspecular = inten * f_att * ks * pow(max(dotprod, 0.0f), max(0.0, shininess));

        fragColor[0] += constantsspecular * lightColors[i][0] * shapeColorS[0] * shadowFactor;
        fragColor[1] += constantsspecular * lightColors[i][1] * shapeColorS[1] * shadowFactor;
        fragColor[2] += constantsspecular * lightColors[i][2] * shapeColorS[2] * shadowFactor;
    }

    fragColor.r = min(max(fragColor.r, 0.0), 1.0);
    fragColor.g = min(max(fragColor.g, 0.0), 1.0);
    fragColor.b = min(max(fragColor.b, 0.0), 1.0);
    fragColor.a = 1.0;
}
