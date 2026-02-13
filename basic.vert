#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

uniform mat4 uM;
uniform mat4 uV;
uniform mat4 uP;

out vec2 TexCoords;
out vec3 FragPosWorld;
out vec3 NormalWorld;

void main()
{
    // compute world / view / clip positions robustly and always write gl_Position
    vec4 worldPos = uM * vec4(aPos, 1.0);
    vec4 viewPos = uV * worldPos;
    gl_Position = uP * viewPos;

    FragPosWorld = vec3(worldPos);
    // Normal transform: use model (upper-left 3x3) inverse-transpose via mat3(uM)
    NormalWorld = mat3(transpose(inverse(uM))) * aNormal;
    TexCoords = aTexCoords;
}