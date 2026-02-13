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
float mapTexScale = 1.0f;
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

// Camera defaults (world-space) - first-person style
glm::vec3 cameraPos = glm::vec3(0.0f, 1.60f, 0.0f); // initial player eye pos
// Use yaw/pitch to derive initial front so orientation is consistent
float cameraYaw = 90.0f;
float cameraPitch = -12.0f;
glm::vec3 cameraFront = glm::normalize(glm::vec3(
    cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch)),
    sin(glm::radians(cameraPitch)),
    sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch))
));
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraFov = 45.0f;
float cameraSpeed = 4.0f; // walking-scale movement

// Camera scalar convenience (kept initalized from cameraPos)
float cameraPosX = cameraPos.x;
float cameraPosY = cameraPos.y;
float cameraPosZ = cameraPos.z;

// Saved camera state (initialized to defaults so restore is safe)
glm::vec3 saved_cameraPos = cameraPos;
glm::vec3 saved_cameraFront = cameraFront;
float saved_cameraYaw = cameraYaw;
float saved_cameraPitch = cameraPitch;

// Camera presets moved to globals for easy tuning:
glm::vec3 cameraDefaultPos = glm::vec3(0.0f, 1.60f, 0.0f);     // default first-person
glm::vec3 cameraDebugPos   = glm::vec3(0.0f, 12.0f, 12.0f);    // debug framing
glm::vec3 cameraDebugFront = glm::normalize(glm::vec3(0.0f, -0.7f, -1.0f)); // debug look vector

// Camera heights for modes
float cameraYWalking = 1.60f;    // eye height when walking (first-person)
// Lowered overview height to be closer to plane (was 30.0f)
float cameraYMeasuring = 10.0f;  // reduced so camera sits lower

// how far behind the map center the overview camera sits (world units, positive Z)
float overviewCameraBack = 20.0f;

// allow camera to go slightly beyond plane bounds
float cameraClampMargin = 0.01f; // world units outside the plane allowed

// Measuring/overview camera explicit values (moved into globals)
float measureCamX = 0.0f;
float measureCamY = 18.0f;      // lower -> closer to map
float measureCamZ = -10.0f;     // more negative -> further back from origin (pulls map lower)
float measureCamYaw = 90.0f;    // horizontal rotation (deg)
float measureCamPitch = -120.0f; // tilt down (deg)

// Scene light defaults (tune these for color/intensity/position)
glm::vec3 sceneLightPos       = glm::vec3(5.0f, 12.0f, 5.0f);
glm::vec3 sceneLightColor     = glm::vec3(1.0f, 0.96f, 0.90f); // warm white
float     sceneLightIntensity = 0.65f;   // increased to make scene brighter
// radius controlling how far the point light reaches (ignored in directional mode)
float     sceneLightRadius    = 1.0f;  // large for point light fallback

// Directional mode (when true, light is treated as directional/sun and radius/pos ignored)
bool      sceneLightDirectional = true;
// Direction the directional light comes FROM (pointing toward scene). Use straight down for uniform lighting.
glm::vec3 sceneLightDir        = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));

// Debug toggles for rendering state
bool depthTestEnabled = true;
bool faceCullingEnabled = false;
bool cullBackFaces = true;
bool isCCWWinding = true;

// Model sizing / runtime reload request
float desiredModelHeight   = 1.5f;
float requestModelLoadHeight = 1.5f;
bool  requestReloadModel     = false;