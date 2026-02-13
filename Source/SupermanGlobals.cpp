#include "../Header/SupermanGlobals.h"

float supermanMeters = 0.0f;

float METERS_PER_WORLD_UNIT = 400.0f;


float supermanModelYawOffsetDeg = -90.0f; // adjust if needed to align model forward

glm::vec3 supermanPos(0.0f, 0.0f, 0.0f); 
float supermanYawDeg = 0.0f;            // facing direction in degrees 
const float supermanMoveSpeed = 4.0f;   
const float supermanTurnSpeed = 720.0f; // degrees per second (how fast he turns)


float modelHeightMini = 0.4f;
float modelLoadHeightMini = 0.3f;

float modelHeightBig = 1.5f;
float modelLoadHeightBig = 1.5f;