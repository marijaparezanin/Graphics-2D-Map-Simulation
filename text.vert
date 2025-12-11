#version 330 core

layout(location = 0) in vec2 inPos;
uniform vec2 uResolution;

void main() {
    // convert pixel coordinates (inPos) to NDC where (0,0) is top-left in pixels
    float ndcX = (inPos.x / uResolution.x) * 2.0 - 1.0;
    float ndcY = 1.0 - (inPos.y / uResolution.y) * 2.0;
    gl_Position = vec4(ndcX, ndcY, 0.0, 1.0);
}