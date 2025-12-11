#version 330 core
in vec4 chCol;

out vec4 outCol;

uniform float uB;
uniform float uA;


void main()
{
    outCol = vec4(chCol.rg, chCol.b + uB, chCol.a + uA);
} 