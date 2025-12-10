#version 330 core

// Task 5: declare "in" variables for the world-space position and normal,
//         received post-interpolation from the vertex shader
in vec3 worldSpacePosition;
in vec3 worldSpaceNormal;

// Task 10: declare an out vec4 for your output color
out vec4 fragColor;

// Task 12: declare relevant uniform(s) here, for ambient lighting
uniform float m_ka;

// Task 13: declare relevant uniform(s) here, for diffuse lighting
uniform float m_kd;
uniform vec4 m_lightPos;

// Task 14: declare relevant uniform(s) here, for specular lighting
uniform float m_ks;
uniform float m_shininess;
uniform vec3 m_camPos;

// Phong
uniform vec4 cAmbient;
uniform vec4 cDiffuse;
uniform vec4 cSpecular;

struct SceneLightData {
    int type;

    vec4 color;
    vec3 function; // Attenuation function

    vec4 pos; // Position with CTM applied (Not applicable to directional lights)
    vec4 dir; // Direction with CTM applied (Not applicable to point lights)

    float penumbra; // Only applicable to spot lights, in RADIANS
    float angle;    // Only applicable to spot lights, in RADIANS
};

uniform int numLights;
uniform SceneLightData lights[8];

float getAttenuation(SceneLightData light, vec3 lightVector) {
    float c1 = light.function[0];
    float c2 = light.function[1];
    float c3 = light.function[2];
    float distance = length(lightVector);
    float fAttenuation = min(1.0, 1.0 / (c1 + (distance * c2) + (distance * distance * c3)));

    return fAttenuation;
}


void main() {
    // Remember that you need to renormalize vectors here if you want them to be normalized
    // Task 10: set your output color to white (i.e. vec4(1.0)). Make sure you get a white circle!

    // Task 11: set your output color to the absolute value of your world-space normals,
    //          to make sure your normals are correct.
    //fragColor = vec4(abs(worldSpaceNormal), 1.0);

    // Task 12: add ambient component to output color
    fragColor = vec4(0.0);
    fragColor = vec4(m_ka * vec3(cAmbient), 1.0);
    bool theBugger = true;
    //fragColor = vec4(0.0);

    for (int i = 0; i < numLights; i++) {
        vec3 lightVector;
        float fAttenuation = 1.0;
        float falloff = 0.0;

        switch (lights[i].type) {
            case 0:  // POINT
                lightVector = vec3(lights[i].pos) - worldSpacePosition;
                fAttenuation = getAttenuation(lights[i], lightVector);

                break;
            case 1:  // DIRECTIONAL
                lightVector = -vec3(lights[i].dir);

                break;
            case 2:  // SPOT
                lightVector = vec3(lights[i].pos[0], lights[i].pos[1], lights[i].pos[2]) - worldSpacePosition;
                fAttenuation = getAttenuation(lights[i], lightVector);
                float foundAngle = acos(dot(normalize(-lightVector), normalize(vec3(lights[i].dir))));
                float outer = lights[i].angle;
                float inner = outer - (lights[i].penumbra);

                if ((foundAngle <= lights[i].angle) && (foundAngle >= inner)) {
                    float subterm = (foundAngle - inner) / (outer - inner);
                    falloff = -2.0f * pow(subterm, 3) + 3.0f * pow(subterm, 2);
                } else if (foundAngle > lights[i].angle) {
                    falloff = 1.0f;
                } else {
                    falloff = 0.0f;
                }

                break;
        }

        vec3 L = normalize(lightVector);
        vec3 N = normalize(worldSpaceNormal);
        vec3 V = normalize(m_camPos - worldSpacePosition);
        vec3 R = reflect(-L, N);

        if (dot(N, L) >= 0) {
            theBugger = false;
        }

        float diffuseLightContribution = clamp(m_kd * max(dot(N, L), 0), 0, 1);
        float specularLightContribution = clamp(m_ks * pow(max(dot(R, V), 0), m_shininess), 0, 1);

        vec3 diffuseColor = clamp(diffuseLightContribution * vec3(cDiffuse), 0, 1);
        vec3 specularColor = clamp(specularLightContribution * vec3(cSpecular), 0, 1);

        // Process each color channel independently
        //float diffuseComponent = m_kd * (clamp(dot(normalize(worldSpaceNormal), normalize(lightDir)), 0, 1));
        vec3 lightContribution = clamp(diffuseColor + specularColor, 0, 1);

        float I = 1.0 - falloff;
        fragColor += vec4(fAttenuation * I * vec3(lights[i].color) * lightContribution, 1);

        //fragColor += vec4(lightContribution, 1);
    }

    fragColor = clamp(fragColor, 0, 1);
    fragColor = vec4(1);
    //fragColor = vec4(m_ka * vec3(cAmbient), 1.0);

    if (theBugger) {
        //fragColor = vec4(0);
    }
    //fragColor = vec4(abs(worldSpaceNormal), 1.0);
}
