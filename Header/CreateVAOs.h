#pragma once

#include <GLFW/glfw3.h>

// Drawing functions for shapes (map, personal info rectangle, standing man)
void formMapVAO(unsigned int& VAOmap);

void formStandingManVAO(unsigned int& VAOstandingMan);

void formInformationRectVAO(unsigned int& VAOrect);

// Create a VAO for a pin icon placed in the top-left with an explicit pixel margin.
// marginPx: margin from top and left edges in pixels.
void formTopPinVAO(unsigned int& VAOtopPin, float marginPx = 8.0f);

// Create a VAO for a wider top-left icon (used for standing-man appearance)
void formTopPinWideVAO(unsigned int& VAOtopPinWide, float marginPx = 8.0f);

// Convenience: create all VAOs used by the app (wrapper used by Main.cpp)
void formAllVAOs();