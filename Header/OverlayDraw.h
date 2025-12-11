#pragma once
#include <vector>

// Draw a polyline given pixel-space points: pts = [x0,y0, x1,y1, ...]
// Color components in 0..1, alpha in 0..1.
void drawPolylinePixels(const std::vector<float>& pts, float r, float g, float b, float a);

// Draw points given pixel-space points: pts = [x0,y0, x1,y1, ...]
// pxSize is the point size in pixels.
void drawPointsPixels(const std::vector<float>& pts, float r, float g, float b, float a, float pxSize);