#pragma once

#include <glm/glm.hpp>

void initMeasurement3D();
void shutdownMeasurement3D();
void drawMeasurements3D(const glm::mat4& view, const glm::mat4& projection, float planeScale);