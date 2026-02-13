#version 330 core

in vec2 TexCoords;
in vec3 FragPosWorld;
in vec3 NormalWorld;

uniform vec3 uLightPos;        // existing model-centered bright light (primary)
uniform vec3 uViewPos;
uniform vec3 uLightColor;
uniform float uLightIntensity;

uniform bool uUseTex;
uniform vec3 uMatColor;

uniform float uSpecularStrength;
uniform float uShininess;
uniform sampler2D uDiffMap1; // loader binds diffuse textures to uDiffMap1, uDiffMap2...

// New controls for softer, frontal fill lighting and raised ambient
uniform vec3  uFrontDir;       // normalized direction pointing FROM the model toward the frontal light (world-space)
uniform float uFrontIntensity; // strength of frontal fill light
uniform float uAmbientFactor;  // overall ambient multiplier (0..1)

out vec4 FragColor;

void main()
{
    vec3 color = uMatColor;
    if (uUseTex) {
        color = texture(uDiffMap1, TexCoords).rgb;
    }

    vec3 norm = normalize(NormalWorld);

    // Primary (existing) light contribution
    vec3 primaryDir = normalize(uLightPos - FragPosWorld);
    float diffPrimary = max(dot(norm, primaryDir), 0.0);
    vec3 diffusePrimary = diffPrimary * color * uLightColor * uLightIntensity;

    // Frontal fill light (directional) - softens shadows and brightens front of the model
    vec3 fillDir = normalize(uFrontDir);
    float diffFront = max(dot(norm, fillDir), 0.0);
    vec3 diffuseFront = diffFront * color * uLightColor * uFrontIntensity;

    // Ambient / hemispheric approximation (simple)
    vec3 ambient = color * uAmbientFactor;

    // Specular: combine contributions from both lights (Blinn-Phong)
    vec3 viewDir = normalize(uViewPos - FragPosWorld);

    vec3 halfPrimary = normalize(primaryDir + viewDir);
    float specPrimary = pow(max(dot(norm, halfPrimary), 0.0), uShininess) * uSpecularStrength * uLightIntensity;

    vec3 halfFront = normalize(fillDir + viewDir);
    float specFront = pow(max(dot(norm, halfFront), 0.0), uShininess) * uSpecularStrength * uFrontIntensity;

    vec3 specular = (specPrimary + specFront) * uLightColor * 0.5; // scale down combined spec a bit

    vec3 final = ambient + diffusePrimary + diffuseFront + specular;

    // tone clamp / gamma hint
    final = clamp(final, 0.0, 1.0);

    FragColor = vec4(final, 1.0);
}