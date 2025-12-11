#pragma once

void drawRect(unsigned int rectShader, unsigned int VAOrect);
void drawMap(unsigned int rectShader, unsigned int VAOmap);
void drawStandinMan(unsigned int rectShader, unsigned int VAOstandingMan);

// Draw top-left pin. You can supply both VAOs: VAOtopPin (normal) and VAOtopPinWide (wider).
// VAOtopPinWide is optional (default 0) — if 0, the normal VAO is used.
void drawTopPin(unsigned int rectShader, unsigned int VAOtopPin, unsigned int VAOtopPinWide = 0);

void setupShader(unsigned int shader,
    int texture = 0,
    float x = 0.0f,
    float y = 0.0f,
    float scale = 1.0f,
    float opacity = 1.0f,
    float texOffsetX = 0.0f,
    float texOffsetY = 0.0f,
    float texScale = 1.0f);

