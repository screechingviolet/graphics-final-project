#version 330 core

in vec2 uv;
out vec4 fragColor;

uniform sampler2D sceneTex;
uniform sampler2D depthTex;
uniform vec2 lightScreenPos;      // Light position in screen space [0,1]
uniform float exposure;           // Ray intensity
uniform float decay;              // Ray decay factor
uniform float density;            // Ray density
uniform float weight;             // Ray weight
uniform int numSamples;           // Number of samples along ray
uniform vec2 screenSize;          // Screen dimensions
uniform float facing;             // How much camera faces light (0-1)

void main() {
    // Sample the scene color
    vec4 sceneColor = texture(sceneTex, uv);

    // Early exit if camera is facing away from light
    if (facing < 0.1) {
        fragColor = sceneColor;
        return;
    }

    // Calculate vector from current pixel to light position
    vec2 deltaTexCoord = uv - lightScreenPos;

    // Divide by number of samples and scale by density
    deltaTexCoord *= 1.0 / float(numSamples) * density;

    // Initialize illumination decay
    float illuminationDecay = 1.0;

    // Start sampling position at current fragment
    vec2 samplePos = uv;

    // Accumulate color along ray
    vec3 rayColor = vec3(0.0);

    for (int i = 0; i < numSamples; i++) {
        // Step towards light
        samplePos -= deltaTexCoord;

        // Only sample if within texture bounds
        if (samplePos.x < 0.0 || samplePos.x > 1.0 ||
            samplePos.y < 0.0 || samplePos.y > 1.0) {
            break;
        }

        // Sample scene at this position
        vec4 sampleColor = texture(sceneTex, samplePos);

        // Sample depth to check if this pixel is "sky" or bright
        float depth = texture(depthTex, samplePos).r;

        // Enhance effect for pixels that are far away (sky)
        // or very bright
        float brightness = dot(sampleColor.rgb, vec3(0.299, 0.587, 0.114));
        float depthFactor = (depth > 0.9999) ? 2.0 : 1.0;
        float brightnessFactor = smoothstep(0.5, 1.0, brightness);

        // Apply illumination decay and weights
        sampleColor.rgb *= illuminationDecay * weight * depthFactor * (1.0 + brightnessFactor);

        // Accumulate
        rayColor += sampleColor.rgb;

        // Update decay
        illuminationDecay *= decay;
    }

    // Apply exposure and facing factor
    rayColor *= exposure * facing;

    // Blend rays with original scene
    fragColor = vec4(sceneColor.rgb + rayColor, sceneColor.a);
}
