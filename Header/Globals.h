#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <utility>

// Camera math (GLM)
#include <glm/glm.hpp>

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

// Camera (added to support 3D view)
// camera position and orientation (used by arrow keys and lookAt)
extern glm::vec3 cameraPos;
extern glm::vec3 cameraFront;
extern glm::vec3 cameraUp;
extern float cameraYaw;
extern float cameraPitch;
extern float cameraFov;
extern float cameraSpeed;

// Camera scalar convenience
extern float cameraPosX;
extern float cameraPosY;
extern float cameraPosZ;

// Saved camera state used when toggling overview
extern glm::vec3 saved_cameraPos;
extern glm::vec3 saved_cameraFront;
extern float saved_cameraYaw;
extern float saved_cameraPitch;

// Camera defaults / presets (moved here for easy tuning)
extern glm::vec3 cameraDefaultPos;       // initial first-person position
extern glm::vec3 cameraDebugPos;         // debug framing position
extern glm::vec3 cameraDebugFront;       // debug front vector

// Predefined camera heights (walking vs measuring modes)
extern float cameraYWalking;
extern float cameraYMeasuring;
extern float overviewCameraBack;
// Allow camera to stray outside plane bounds by this margin (world units)
extern float cameraClampMargin;

// Measuring (overview) camera explicit globals (moved here)
extern float measureCamX;
extern float measureCamY;
extern float measureCamZ;
extern float measureCamYaw;
extern float measureCamPitch;

// Scene light (single point or directional)
extern glm::vec3 sceneLightPos;
extern glm::vec3 sceneLightColor; // linear rgb 0..1
extern float     sceneLightIntensity; // multiplier
extern float     sceneLightRadius;    // point light radius (falloff control)

// New: directional mode toggle + direction (useful for uniform lighting)
extern bool      sceneLightDirectional;
extern glm::vec3 sceneLightDir;

// New: debug toggles for rendering state (depth test / face culling / winding)
extern bool depthTestEnabled;
extern bool faceCullingEnabled;
extern bool cullBackFaces;
extern bool isCCWWinding;

// Model sizing / runtime reload request
// - `desiredModelHeight` is used by Main when positioning/lifting the model (vertical lift).
// - `requestModelLoadHeight` is the height passed to `loadActiveModel` when reloading the model mesh (scale computation).
// - `requestReloadModel` when true tells Main to call `loadActiveModel` with `requestModelLoadHeight` at the next frame.
extern float desiredModelHeight;
extern float requestModelLoadHeight;
extern bool  requestReloadModel;