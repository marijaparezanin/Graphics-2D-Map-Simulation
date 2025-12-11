#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "../Header/Callbacks.h"
#include "../Header/Util.h"
#include <cmath> // for sqrtf
#include <vector>
#include <utility>

// Extern global variables from Main.cpp
extern GLFWcursor* cursor;
extern GLFWcursor* cursorPressed;
extern int screenWidth;
extern int screenHeight;

// Extern map pan/zoom state (defined in Main.cpp)
extern float mapOffsetX;
extern float mapOffsetY;
extern float mapTexScale;
extern float panSpeed;

// overview state saved in Main.cpp
extern bool overviewMode;
extern float saved_mapOffsetX;
extern float saved_mapOffsetY;
extern float saved_mapTexScale;

// pin texture state is defined in Main.cpp
extern bool pinShowsStanding;

// measurement storage (defined in Main.cpp)
extern std::vector<std::pair<float,float>> measurementPoints;
extern float measurementDistancePixels;

extern int standingManState;
extern int standingManAnimFrame;
extern float standingManAnimTimer;                                                                  

extern float distancePixels; // 1 px == 1 m

void center_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        glfwSetCursor(window, cursorPressed);

        double xpos_win, ypos_win;
        glfwGetCursorPos(window, &xpos_win, &ypos_win);

        // convert window coords to framebuffer coords (handles HiDPI)
        int fbW = 0, fbH = 0;
        int winW = 0, winH = 0;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        glfwGetWindowSize(window, &winW, &winH);
        if (winW == 0) winW = 1;
        if (winH == 0) winH = 1;
        float scaleX = (float)fbW / (float)winW;
        float scaleY = (float)fbH / (float)winH;
        float xpos = (float)xpos_win * scaleX;
        float ypos = (float)ypos_win * scaleY;

        // Pin hitbox (must match formTopPinVAO: pxW=80, pxH=109, margin=8)
        const float pinPxW = 80.0f;
        const float pinPxH = 109.0f;
        const float pinMarginPx = 8.0f;

        // GLFW cursor origin is top-left of the window (y increases downward)
        bool insidePin =
            xpos >= pinMarginPx &&
            xpos <= pinMarginPx + pinPxW &&
            ypos >= pinMarginPx &&
            ypos <= pinMarginPx + pinPxH;

        if (insidePin) {
            // Toggle overview exactly like pressing R: save/restore view
            if (!overviewMode) {
                saved_mapOffsetX = mapOffsetX;
                saved_mapOffsetY = mapOffsetY;
                saved_mapTexScale = mapTexScale;
                mapTexScale = 1.0f;
                mapOffsetX = 0.0f;
                mapOffsetY = 0.0f;
                overviewMode = true;
            } else {
                mapOffsetX = saved_mapOffsetX;
                mapOffsetY = saved_mapOffsetY;
                mapTexScale = saved_mapTexScale;
                overviewMode = false;
            }

            // set pin visual to reflect overview state
            pinShowsStanding = overviewMode;

            // consume click
            return;
        }

        // If in overview mode, clicks on the map add measurement points
        if (overviewMode) {
            // If click is on an existing measurement point, delete it and update the total.
            const float hitRadius = 12.0f; // pixels, tolerance for clicking a point
            int hitIndex = -1;
            for (size_t i = 0; i < measurementPoints.size(); ++i) {
                float dx = measurementPoints[i].first - xpos;
                float dy = measurementPoints[i].second - ypos;
                float dist2 = dx*dx + dy*dy;
                if (dist2 <= hitRadius * hitRadius) {
                    hitIndex = (int)i;
                    break;
                }
            }

            if (hitIndex >= 0) {
                // erase the clicked point
                measurementPoints.erase(measurementPoints.begin() + hitIndex);

                // recompute total distance as sum of present segments
                measurementDistancePixels = 0.0f;
                for (size_t i = 1; i < measurementPoints.size(); ++i) {
                    float dx = measurementPoints[i].first - measurementPoints[i-1].first;
                    float dy = measurementPoints[i].second - measurementPoints[i-1].second;
                    measurementDistancePixels += sqrtf(dx*dx + dy*dy);
                }

                // consume click
                return;
            }

            // record the clicked pixel in framebuffer coords (not near existing point)
            measurementPoints.emplace_back(xpos, ypos);
            size_t n = measurementPoints.size();
            if (n >= 2) {
                auto &p0 = measurementPoints[n-2];
                auto &p1 = measurementPoints[n-1];
                float dx = p1.first - p0.first;
                float dy = p1.second - p0.second;
                float seg = sqrtf(dx*dx + dy*dy);
                measurementDistancePixels += seg;
            }
            // consume click
            return;
        }

        // (existing behaviour) compute normalized pos if you still need it
        float xposNorm = static_cast<float>((xpos_win / screenWidth) * 2 - 1);
        float yposNorm = static_cast<float>(-((ypos_win / screenHeight) * 2 - 1));

    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        glfwSetCursor(window, cursor);
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // ESC to close
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }

    // R toggles  mode
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        if (!overviewMode) {
            // enter overview: save current view and switch to full texture view
            saved_mapOffsetX = mapOffsetX;
            saved_mapOffsetY = mapOffsetY;
            saved_mapTexScale = mapTexScale;
            mapTexScale = 1.0f;
            mapOffsetX = 0.0f;
            mapOffsetY = 0.0f;
            overviewMode = true;
        } else {
            // exit overview: restore saved view
            mapOffsetX = saved_mapOffsetX;
            mapOffsetY = saved_mapOffsetY;
            mapTexScale = saved_mapTexScale;
            overviewMode = false;
        }
        // keep pin visual in sync with overview state
        pinShowsStanding = overviewMode;
    }
}

// This moves the texture sub-rectangle (mapOffsetX/Y) so the standing man stays visually centered.
// main calls updateMovement for each frame.
void updateMovement(GLFWwindow* window, float dt) {
    // Use a direction vector so combinations (e.g., A+W) work and normalize for constant speed
    float dirX = 0.0f, dirY = 0.0f;

    //WASD
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) dirY += 1.0f; // pan up
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) dirY -= 1.0f; // pan down
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) dirX -= 1.0f; // pan left
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) dirX += 1.0f; // pan right

    // Arrow keys
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    dirY += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  dirY -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  dirX -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) dirX += 1.0f;

    bool movingRightish = dirX > 0.0f; 
    bool movingLeftish  = dirX < 0.0f; 
    bool movingUpish    = dirY > 0.0f;
    bool movingDownish  = dirY < 0.0f;

    if (dirX != 0.0f || dirY != 0.0f) {
        // normalize so diagonal speed equals cardinal speed
        float len = sqrtf(dirX * dirX + dirY * dirY);
        if (len != 0.0f) {
            dirX /= len;
            dirY /= len;
        }

        // panStep tuned by panSpeed and scaled by mapTexScale for finer control when zoomed in
        float panStep = panSpeed * dt * mapTexScale;

        // update offsets (you may invert signs to taste)
        mapOffsetX += dirX * panStep;
        mapOffsetY += dirY * panStep;

        // compute movement in texture units and convert to screen pixels
        float dxTex = dirX * panStep;
        float dyTex = dirY * panStep;
        float movedPxX = fabsf(dxTex) * (float)screenWidth / mapTexScale;
        float movedPxY = fabsf(dyTex) * (float)screenHeight / mapTexScale;
        float movedPx = sqrtf(movedPxX * movedPxX + movedPxY * movedPxY);
        distancePixels += movedPx;

        // clamp so viewport stays inside texture
        if (mapOffsetX < 0.0f) mapOffsetX = 0.0f;
        if (mapOffsetY < 0.0f) mapOffsetY = 0.0f;
        if (mapOffsetX > 1.0f - mapTexScale) mapOffsetX = 1.0f - mapTexScale;
        if (mapOffsetY > 1.0f - mapTexScale) mapOffsetY = 1.0f - mapTexScale;
    }

    // update facing state and animation:
    // - movingRightish: use right-facing animation (state=1)
    // - movingLeftish:  use left-facing animation (state=2)
    // - movingUpish:    use up-facing animation (state=3)
    // - movingDownish:  use down-facing animation (state=4)
    // - no movement:    idle (state=0)
    if (movingRightish) {
        standingManState = 1; // rightish
        // advance animation timer; toggle frame every 0.5 second
        standingManAnimTimer += dt;
        if (standingManAnimTimer >= 0.5f) {
            standingManAnimTimer -= 0.5f;
            standingManAnimFrame ^= 1; // toggle 0/1
        }
    } else if (movingLeftish) {
        standingManState = 2; // leftish
        standingManAnimTimer += dt;
        if (standingManAnimTimer >= 0.5f) {
            standingManAnimTimer -= 0.5f;
            standingManAnimFrame ^= 1;
        }
    } else if (movingUpish) {
        standingManState = 3; // up
        standingManAnimTimer += dt;
        if (standingManAnimTimer >= 0.5f) {
            standingManAnimTimer -= 0.5f;
            standingManAnimFrame ^= 1;
        }
    } else if (movingDownish) {
        standingManState = 4; // down
        standingManAnimTimer += dt;
        if (standingManAnimTimer >= 0.5f) {
            standingManAnimTimer -= 0.5f;
            standingManAnimFrame ^= 1;
        }
    } else {
        // no movement -> idle/default
        standingManState = 0;
        standingManAnimTimer = 0.0f;
        standingManAnimFrame = 0;
    }
}

void updateMapMovement(GLFWwindow* window, float dt) {
    // Use a direction vector so combinations (e.g., A+W) work and normalize for constant speed
    float dirX = 0.0f, dirY = 0.0f;

    //WASD
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) dirY += 1.0f; // pan up
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) dirY -= 1.0f; // pan down
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) dirX -= 1.0f; // pan left
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) dirX += 1.0f; // pan right

    // Arrow keys (also supported)
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    dirY += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  dirY -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  dirX -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) dirX += 1.0f;

    bool movingRightish = dirX > 0.0f; // true when input has rightward component
    bool movingLeftish  = dirX < 0.0f; // true when input has leftward component
    bool movingUpish    = dirY > 0.0f;
    bool movingDownish  = dirY < 0.0f;

    if (dirX != 0.0f || dirY != 0.0f) {
        // normalize so diagonal speed equals cardinal speed
        float len = sqrtf(dirX * dirX + dirY * dirY);
        if (len != 0.0f) {
            dirX /= len;
            dirY /= len;
        }

        // panStep tuned by panSpeed and scaled by mapTexScale for finer control when zoomed in
        float panStep = panSpeed * dt * mapTexScale;

        // update offsets (you may invert signs to taste)
        mapOffsetX += dirX * panStep;
        mapOffsetY += dirY * panStep;

        // compute movement in texture units and convert to screen pixels
        float dxTex = dirX * panStep;
        float dyTex = dirY * panStep;
        float movedPxX = fabsf(dxTex) * (float)screenWidth / mapTexScale;
        float movedPxY = fabsf(dyTex) * (float)screenHeight / mapTexScale;
        float movedPx = sqrtf(movedPxX * movedPxX + movedPxY * movedPxY);
        distancePixels += movedPx;

        // clamp so viewport stays inside texture
        if (mapOffsetX < 0.0f) mapOffsetX = 0.0f;
        if (mapOffsetY < 0.0f) mapOffsetY = 0.0f;
        if (mapOffsetX > 1.0f - mapTexScale) mapOffsetX = 1.0f - mapTexScale;
        if (mapOffsetY > 1.0f - mapTexScale) mapOffsetY = 1.0f - mapTexScale;
    }

    // update facing/animation similar to updateMovement
    if (movingRightish) {
        standingManState = 1;
        standingManAnimTimer += dt;
        if (standingManAnimTimer >= 0.5f) {
            standingManAnimTimer -= 0.5f;
            standingManAnimFrame ^= 1;
        }
    } else if (movingLeftish) {
        standingManState = 2;
        standingManAnimTimer += dt;
        if (standingManAnimTimer >= 0.5f) {
            standingManAnimTimer -= 0.5f;
            standingManAnimFrame ^= 1;
        }
    } else if (movingUpish) {
        standingManState = 3;
        standingManAnimTimer += dt;
        if (standingManAnimTimer >= 0.5f) {
            standingManAnimTimer -= 0.5f;
            standingManAnimFrame ^= 1;
        }
    } else if (movingDownish) {
        standingManState = 4;
        standingManAnimTimer += dt;
        if (standingManAnimTimer >= 0.5f) {
            standingManAnimTimer -= 0.5f;
            standingManAnimFrame ^= 1;
        }
    } else {
        standingManState = 0;
        standingManAnimTimer = 0.0f;
        standingManAnimFrame = 0;
    }
}