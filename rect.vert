#version 330 core

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inTex;
out vec2 chTex;

// ux moves it horizontal
// uy moves it vertical
// us scales geometry

uniform float uX;
uniform float uY;
uniform float uS; // geometric scale
uniform vec2 uTexOffset; // texture-space pan (0..1)
uniform float uTexScale; // texture-space scale (<1 = zoom in)

void main()
{
    // transform quad geometry (keeps quad fullscreen or positioned)
    gl_Position = vec4((inPos.x * uS) + uX, (inPos.y * uS) + uY, 0.0, 1.0);

    // Compute texture coordinates by sampling a sub-rectangle of the texture
    // inTex in [0,1] -> map to [uTexOffset, uTexOffset + uTexScale]
    chTex = inTex * uTexScale + uTexOffset;
}