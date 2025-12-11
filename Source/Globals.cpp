#include "../Header/Globals.h"

// Window size
int screenWidth = 800;
int screenHeight = 800;

GLFWcursor* cursor = nullptr;
GLFWcursor* cursorPressed = nullptr;

// VAOs
unsigned int VAOrect = 0;
unsigned int VAOmap = 0;
unsigned int VAOstandingMan = 0;
unsigned int VAOtopPin = 0;
unsigned int VAOtopPinWide = 0;

// textures
unsigned personalInformationTexture = 0;
unsigned pinTexture = 0;
unsigned mapTexture = 0;
unsigned standingManTexture = 0;
unsigned standingManTextureRight = 0;
unsigned standingManTextureRightAlt = 0;
unsigned standingManTextureLeft = 0;
unsigned standingManTextureLeftAlt = 0;
// up/down frames
unsigned standingManTextureUp = 0;
unsigned standingManTextureUpAlt = 0;
unsigned standingManTextureDown = 0;
unsigned standingManTextureDownAlt = 0;

// visuals / state
bool pinShowsStanding = false;
float lowerOpacity = 0.6f;
float fullOpacity = 1.0f;

// standing man state
int standingManState = 0;
int standingManAnimFrame = 0;
float standingManAnimTimer = 0.0f;

// map pan/zoom state
float mapOffsetX = 0.0f;
float mapOffsetY = 0.0f;
float mapTexScale = 0.15f;
float panSpeed = 0.15f;

// walking distance (pixels)
float distancePixels = 0.0f;

// Text renderer constants
float TEXT_SCALE = 6.0f;
float METERS_PER_PIXEL = 0.5f;

// measurement tool (framebuffer pixels)
std::vector<std::pair<float,float>> measurementPoints;
float measurementDistancePixels = 0.0f;

// Overview toggle and saved view state
bool overviewMode = false;
float saved_mapOffsetX = 0.0f;
float saved_mapOffsetY = 0.0f;
float saved_mapTexScale = 0.15f;