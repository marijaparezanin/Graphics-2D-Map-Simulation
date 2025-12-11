#pragma once

// Simple in-canvas text rendering using stb_easy_font.
// initText() must be called after an OpenGL context is ready.
// drawText draws at pixel coordinates where (0,0) is top-left.
void initText();
void drawText(const char* text, float xPx, float yPx, float r, float g, float b);
void cleanupText();
