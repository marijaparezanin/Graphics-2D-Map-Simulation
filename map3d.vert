#version 330 core

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTex;

out vec2 vTex;
out vec3 vNormal;
out vec3 vWorldPos;

uniform mat4 uM; // model
uniform mat4 uV; // view
uniform mat4 uP; // projection
uniform vec2 uTexOffset; // texture-space pan (0..1)
uniform float uTexScale; // texture-space scale (<1 = zoom in)

uniform int uFlipX; // 1 => flip U (horizontal), 0 => normal

void main()
{
    vec4 worldPos = uM * vec4(inPos, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = mat3(transpose(inverse(uM))) * inNormal;

    vec2 t = inTex * uTexScale + uTexOffset;
    if (uFlipX == 1) {
        vTex = vec2(1.0 - t.x, t.y);
    } else {
        vTex = t;
    }

    gl_Position = uP * uV * worldPos;
}