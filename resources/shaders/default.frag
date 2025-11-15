#version 330 core

// Task 5: declare "in" variables for the world-space position and normal,
//         received post-interpolation from the vertex shader
in vec3 world_position;
in vec3 world_normal;

// Task 10: declare an out vec4 for your output color
out vec4 fragColor;

// Task 12: declare relevant uniform(s) here, for ambient lighting
uniform float ka;

// Task 13: declare relevant uniform(s) here, for diffuse lighting
uniform float kd;
// uniform vec4 lightPos;

uniform vec3 lightColors[8];
uniform int lightsNum;
uniform int lightTypes[8];
uniform vec3 lightFunctions[8];
uniform vec4 lightPositions[8];
uniform vec4 lightDirections[8];
uniform float lightAngles[8];
uniform float lightPenumbras[8];

// Task 14: declare relevant uniform(s) here, for specular lighting
uniform float ks;
uniform vec4 camPos;
uniform float shininess;
uniform vec3 shapeColorA;
uniform vec3 shapeColorD;
uniform vec3 shapeColorS;

void main() {
    // Remember that you need to renormalize vectors here if you want them to be normalized

    // Task 10: set your output color to white (i.e. vec4(1.0)). Make sure you get a white circle!
    // fragColor = vec4(1.0);

    // Task 11: set your output color to the absolute value of your world-space normals,
    //          to make sure your normals are correct.
    // fragColor = vec4(abs(world_normal), 1.0);

    fragColor = vec4(0.0);

    /*
// Normalizing directions
    // directionToCamera = glm::normalize(directionToCamera);
    if (glm::dot(normal, directionToCamera) < 0) normal = -normal; // i have added this

*/

    vec4 surfaceToLight, reflectionvec;
    float constantsdiffuse, constantsspecular;
    vec4 norm = normalize(vec4(world_normal, 0.0));
    vec4 directionToCamera = normalize(camPos-vec4(world_position, 1.0));
    float falloff, curr_angle;
    float inner, outer, inten;
    float dist, f_att = 1.;
    vec4 position = vec4(world_position, 1.0);
    // if (dot(norm, directionToCamera) < 0) norm = -norm;


    // Task 12: add ambient component to output color
    // what if pow is to the pow of 0?
    fragColor[0] += ka*shapeColorA[0];
    fragColor[1] += ka*shapeColorA[1];
    fragColor[2] += ka*shapeColorA[2];

    for (int i = 0; i < 8; i++) {
        if (i >= lightsNum) continue;

        // if (length(lightColors[i]) != 0) {
        //     fragColor[2] += 1.0;
        // }

        if (lightTypes[i] == 1) {
            surfaceToLight = normalize(-lightDirections[i]);
        }
        if (lightTypes[i] == 0 || lightTypes[i] == 2) {
            surfaceToLight = normalize(lightPositions[i] - position);
        }

        inten = 1;
        if (lightTypes[i] == 2) {
            // if (lightAngles[i] <= 1 && lightAngles[i] >= 0.1) fragColor[0] = 1.0;
            // if (lightPenumbras[i] <= 1 && lightAngles[i] >= 0.02) fragColor[1] = 1.0;

            inner = lightAngles[i]-lightPenumbras[i];
            outer = lightAngles[i];
            curr_angle = acos(dot(surfaceToLight, normalize(-lightDirections[i])));
            if ((curr_angle - inner)/(outer-inner) == 0) falloff = 0.;
            else falloff = (-2. * pow((curr_angle - inner)/(outer-inner), 3.)) + (3. * pow((curr_angle - inner)/(outer-inner), 2.));

            // if (curr_angle <= inner) fragColor[0] = 1.0; // debug

            if (curr_angle > inner && curr_angle <= outer) {
                inten = max(0.0, 1.0 - falloff);
            } else if (curr_angle > outer) {
                inten = 0.;
            }
        }
        // remove this
        // inten = min(1, inten*2);


        f_att = 1.;
        if (lightTypes[i] != 1) {
            dist = length(lightPositions[i] - position);
            f_att = min(1., 1./(lightFunctions[i].x + dist * lightFunctions[i].y + lightFunctions[i].z * (dist*dist)));
        }

        constantsdiffuse = inten * f_att * max(dot(norm, surfaceToLight), 0.f) * kd;
        fragColor[0] += constantsdiffuse * lightColors[i].r * shapeColorD[0];
        fragColor[1] += constantsdiffuse * lightColors[i].g * shapeColorD[1];
        fragColor[2] += constantsdiffuse * lightColors[i].b * shapeColorD[2];

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

    // Task 13: add diffuse component to output color
    // vec4 surfaceToLight = normalize(lightPos-vec4(world_position, 0.0));
    // vec4 norm = normalize(vec4(world_normal, 0.0));
    // fragColor += kd*clamp(dot(norm, surfaceToLight), 0, 1);

    // Task 14: add specular component to output color
    // vec4 reflection = normalize(reflect(-surfaceToLight, norm));
    // fragColor = fragColor + ks*pow(clamp(dot(reflection, normalize(camPos-vec4(world_position, 1.0))), 0, 1), shininess);

    fragColor.a = 1.0;
}
