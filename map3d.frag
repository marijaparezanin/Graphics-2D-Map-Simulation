#version 330 core

in vec2 vTex;
in vec3 vNormal;
in vec3 vWorldPos;

out vec4 FragColor;

uniform sampler2D uMapTex;

// Lighting uniforms
uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform float uLightIntensity;
uniform float uLightRadius; // radius for point light falloff
uniform vec3 uViewPos;

// New directional support
uniform int  uLightDirectional; // 1 => directional, 0 => point light
uniform vec3 uLightDir;         // direction FROM which light comes (should be normalized)

void main()
{
    vec3 albedo = texture(uMapTex, vTex).rgb;

    vec3 N = normalize(vNormal);
    vec3 L;
    float att = 1.0;

    if (uLightDirectional == 1) {
        // directional (sun) - constant intensity across the scene
        // use given light direction (coming FROM), so vector to light = -dir
        L = normalize(-uLightDir);

        // directional gets full, no distance attenuation
        att = 1.0;
    } else {
        // point light with smooth radius falloff
        vec3 toLight = (uLightPos - vWorldPos);
        float dist = length(toLight);
        L = normalize(toLight);
        float r = max(uLightRadius, 0.0001);
        att = 1.0 / (1.0 + (dist * dist) / (r * r));
    }

    vec3 V = normalize(uViewPos - vWorldPos);
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);

    // Raised ambient for uniform base brightness
    vec3 ambient = 0.50 * albedo; // increased ambient

    // Diffuse full-strength (for a flat plane pointing up and a straight-down light this will be ~=1 everywhere)
    vec3 diffuse = NdotL * albedo * uLightColor;

    // moderate specular so highlights don't blow out
    float specularStrength = 0.25;
    float shininess = 32.0;
    vec3 specular = specularStrength * pow(NdotH, shininess) * uLightColor;

    vec3 color = ambient + (diffuse + specular) * uLightIntensity * att;

    // clamp to avoid excessive values
    color = clamp(color, 0.0, 1.0);

    FragColor = vec4(color, 1.0);
}