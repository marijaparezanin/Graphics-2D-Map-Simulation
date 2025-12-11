#version 330 core

in vec2 chTex;
out vec4 outCol;

uniform sampler2D uTex0;
uniform float uOpacity;   


void main()
{
    vec4 texColor = texture(uTex0, chTex);
    outCol = vec4(texColor.rgb, texColor.a * uOpacity);
}