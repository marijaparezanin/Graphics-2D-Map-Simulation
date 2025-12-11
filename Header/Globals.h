#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <utility>

// Window size (framebuffer/window pixels)
extern int screenWidth;
extern int screenHeight;

extern GLFWcursor* cursor;
extern GLFWcursor* cursorPressed;

// VAOs
extern unsigned int VAOrect;
extern unsigned int VAOmap;
extern unsigned int VAOstandingMan;
extern unsigned int VAOtopPin;
extern unsigned int VAOtopPinWide;

// textures
extern unsigned personalInformationTexture;
extern unsigned pinTexture;
extern unsigned mapTexture;
extern unsigned standingManTexture;
extern unsigned standingManTextureRight;
extern unsigned standingManTextureRightAlt;
extern unsigned standingManTextureLeft;
extern unsigned standingManTextureLeftAlt;
extern unsigned standingManTextureUp;
extern unsigned standingManTextureUpAlt;
extern unsigned standingManTextureDown;
extern unsigned standingManTextureDownAlt;

// visuals / state
extern bool pinShowsStanding;
extern float lowerOpacity;
extern float fullOpacity;

// standing man state
extern int standingManState;
extern int standingManAnimFrame;
extern float standingManAnimTimer;

// map pan/zoom state
extern float mapOffsetX;
extern float mapOffsetY;
extern float mapTexScale;
extern float panSpeed;

// walking distance (pixels)
extern float distancePixels;

// Text renderer constants (declare extern so single definition in Globals.cpp)
extern float TEXT_SCALE;
extern float METERS_PER_PIXEL;

// measurement tool (framebuffer pixels)
extern std::vector<std::pair<float,float>> measurementPoints;
extern float measurementDistancePixels;

// Overview toggle and saved view state
extern bool overviewMode;
extern float saved_mapOffsetX;
extern float saved_mapOffsetY;
extern float saved_mapTexScale;