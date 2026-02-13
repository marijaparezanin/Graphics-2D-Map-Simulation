#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "../Header/Callbacks.h"
#include "../Header/Util.h"
#include "../Header/Globals.h"
#include "../Header/SupermanGlobals.h"
#include <cmath> // for sqrtf
#include <vector>
#include <utility>
#include <iostream>

// GLM helpers
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Internal state for mouse-look
static bool g_mouseCaptured = false;
static bool g_firstMouse = true;
static double g_lastX = 0.0;
static double g_lastY = 0.0;
static const float g_mouseSensitivity = 0.06f; // lower slower

static void enableMouseCapture(GLFWwindow* window)
{
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    g_mouseCaptured = true;
    g_firstMouse = true;
}

static void disableMouseCapture(GLFWwindow* window)
{
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    g_mouseCaptured = false;
    if (cursor) glfwSetCursor(window, cursor);
}

void center_callback(GLFWwindow* window, int button, int action, int mods) {
    // Right-click toggles mouse-capture / look-around on press (toggle behavior)
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        if (!g_mouseCaptured) enableMouseCapture(window);
        else disableMouseCapture(window);
        return; // consume right-click for look toggle
    }

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
                // save map view
                saved_mapOffsetX = mapOffsetX;
                saved_mapOffsetY = mapOffsetY;
                saved_mapTexScale = mapTexScale;

                // save camera state
                saved_cameraPos = cameraPos;
                saved_cameraFront = cameraFront;
                saved_cameraYaw = cameraYaw;
                saved_cameraPitch = cameraPitch;

                // disable mouse-look so user cannot rotate the overview camera
                disableMouseCapture(window);

                // enter overview: move camera to center, full-map, using globals for measuring camera
                mapTexScale = 1.0f;
                mapOffsetX = 0.0f;
                mapOffsetY = 0.0f;
                overviewMode = true;

                // Use explicit measuring camera globals for position
                cameraPos = glm::vec3(measureCamX, measureCamY, measureCamZ);
                // Point at map center (origin) to ensure the map is visible
                cameraFront = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - cameraPos);
                // update yaw/pitch and keep measuring globals in sync
                cameraYaw = glm::degrees(atan2(cameraFront.z, cameraFront.x));
                cameraPitch = glm::degrees(asin(glm::clamp(cameraFront.y, -1.0f, 1.0f)));
                measureCamYaw = cameraYaw;
                measureCamPitch = cameraPitch;
            } else {
                // restore map view
                mapOffsetX = saved_mapOffsetX;
                mapOffsetY = saved_mapOffsetY;
                mapTexScale = saved_mapTexScale;
                overviewMode = false;

                // restore camera state
                cameraPos = saved_cameraPos;
                cameraFront = saved_cameraFront;
                cameraYaw = saved_cameraYaw;
                cameraPitch = saved_cameraPitch;
            }

            // set pin visual to reflect overview state
            pinShowsStanding = overviewMode;

            // consume click
            return;
        }

        // If in overview mode, clicks on the map add measurement points
        if (overviewMode) {
            if (fbW > 0 && fbH > 0) {
                // compute normalized device coords (NDC)
                float ndcX = (xpos / float(fbW)) * 2.0f - 1.0f;
                float ndcY = 1.0f - (ypos / float(fbH)) * 2.0f; // convert top-left to bottom-left origin

                // Recreate projection and view matrices (must match main's)
                glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                    (float)screenWidth / (float)screenHeight, 0.005f, 100.0f);
                glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

                // inverse of proj * view to unproject clip-space coords
                glm::mat4 invPV = glm::inverse(projection * view);

                glm::vec4 clipNear(ndcX, ndcY, -1.0f, 1.0f);
                glm::vec4 clipFar (ndcX, ndcY,  1.0f, 1.0f);

                glm::vec4 worldNear4 = invPV * clipNear;
                glm::vec4 worldFar4  = invPV * clipFar;
                if (worldNear4.w == 0.0f || worldFar4.w == 0.0f) {
                    // fallback: ignore click if invalid
                    return;
                }
                glm::vec3 worldNear = glm::vec3(worldNear4) / worldNear4.w;
                glm::vec3 worldFar  = glm::vec3(worldFar4)  / worldFar4.w;
                glm::vec3 rayDir = glm::normalize(worldFar - worldNear);
                glm::vec3 rayOrigin = worldNear;

                // intersect with XZ plane at y = 0
                if (fabs(rayDir.y) < 1e-6f) {
                    // nearly parallel -> ignore (no intersection)
                    return;
                }
                float t = - (rayOrigin.y) / rayDir.y;
                if (t < 0.0f) {
                    // intersection is behind the ray origin -> ignore
                    return;
                }
                glm::vec3 hit = rayOrigin + rayDir * t;

                // planeScale must match the one used in Main when rendering the plane
                const float planeScale = 20.0f;
                const float mapHalf = planeScale * 0.5f;

                // Accept only hits inside the plane bounds
                if (hit.x < -mapHalf || hit.x > mapHalf || hit.z < -mapHalf || hit.z > mapHalf) {
                    // click falls outside the map plane projection -> ignore
                    return;
                }
            }

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
                measurementPoints.erase(measurementPoints.begin() + hitIndex);

                // recompute total distance as sum of present segments
                measurementDistancePixels = 0.0f;
                for (size_t i = 1; i < measurementPoints.size(); ++i) {
                    float dx = measurementPoints[i].first - measurementPoints[i-1].first;
                    float dy = measurementPoints[i].second - measurementPoints[i-1].second;
                    measurementDistancePixels += sqrtf(dx*dx + dy*dy);
                }
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
            return;
        }
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

    // Toggle mouse-capture with 'C'
    if (key == GLFW_KEY_C && action == GLFW_PRESS) {
        if (!g_mouseCaptured) enableMouseCapture(window);
        else disableMouseCapture(window);
        return;
    }

    // R toggles overview mode (on key press)
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        if (!overviewMode) {
            // enter overview: save current view and switch to full texture view
            saved_mapOffsetX = mapOffsetX;
            saved_mapOffsetY = mapOffsetY;
            saved_mapTexScale = mapTexScale;

            // save camera state
            saved_cameraPos = cameraPos;
            saved_cameraFront = cameraFront;
            saved_cameraYaw = cameraYaw;
            saved_cameraPitch = cameraPitch;

            // disable mouse-look while in overview
            disableMouseCapture(window);

            mapTexScale = 1.0f;
            mapOffsetX = 0.0f;
            mapOffsetY = 0.0f;
            overviewMode = true;

            // put overview camera behind the map center so it looks toward the plane
            cameraPos = glm::vec3(0.0f, cameraYMeasuring, -overviewCameraBack);
            cameraYaw = -90.0f;
            cameraPitch = -80.0f;
            {
                float yr = glm::radians(cameraYaw);
                float pr = glm::radians(cameraPitch);
                glm::vec3 f;
                f.x = cos(yr) * cos(pr);
                f.y = sin(pr);
                f.z = sin(yr) * cos(pr);
                cameraFront = glm::normalize(f);
            }
        } else {
            // exit overview: restore saved view
            mapOffsetX = saved_mapOffsetX;
            mapOffsetY = saved_mapOffsetY;
            mapTexScale = saved_mapTexScale;
            overviewMode = false;

            // restore camera state
            cameraPos = saved_cameraPos;
            cameraFront = saved_cameraFront;
            cameraYaw = saved_cameraYaw;
            cameraPitch = saved_cameraPitch;
        }
        // keep pin visual in sync with overview state
        pinShowsStanding = overviewMode;
        return;
    }

    // F-keys: toggle depth/culling/winding (act only on key press)
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_F1:
            depthTestEnabled = !depthTestEnabled;
            if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
            else glDisable(GL_DEPTH_TEST);
            std::cout << (depthTestEnabled ? "DEPTH TEST ENABLED" : "DEPTH TEST DISABLED") << std::endl;
            break;

        case GLFW_KEY_F2:
            faceCullingEnabled = !faceCullingEnabled;
            if (faceCullingEnabled) glEnable(GL_CULL_FACE);
            else glDisable(GL_CULL_FACE);
            std::cout << (faceCullingEnabled ? "FACE CULLING ENABLED" : "FACE CULLING DISABLED") << std::endl;
            break;

        case GLFW_KEY_F3:
            cullBackFaces = !cullBackFaces;
            glCullFace(cullBackFaces ? GL_BACK : GL_FRONT);
            std::cout << (cullBackFaces ? "CULLING BACK" : "CULLING FRONT") << std::endl;
            break;

        case GLFW_KEY_F4:
            isCCWWinding = !isCCWWinding;
            glFrontFace(isCCWWinding ? GL_CCW : GL_CW);
            std::cout << (isCCWWinding ? "CCW WINDING" : "CW WINDING") << std::endl;
            break;

        // M = make model small: set desiredModelHeight (used for lift) and request a reload
        case GLFW_KEY_M:
            // User request: desiredHeight should become 0.4f while loadActiveModel should be called with 0.3f
            desiredModelHeight = modelHeightMini;
            requestModelLoadHeight = modelLoadHeightMini;
            requestReloadModel = true;
            std::cout << "REQUEST: SUPERMAN MINI - reload scheduled" << std::endl;
            break;

        // B = make model big again (restore defaults)
        case GLFW_KEY_B:
            desiredModelHeight = modelHeightBig;
            requestModelLoadHeight = modelLoadHeightBig;
            requestReloadModel = true;
            std::cout << "REQUEST: SUPERMAN BIG - reload scheduled" << std::endl;
            break;

        default:
            break;
        }
    }
}

// Mouse movement callback for look-around
void mouse_move_callback(GLFWwindow* window, double xpos, double ypos) {
    // only affect camera when cursor is captured/disabled
    if (!g_mouseCaptured) {
        g_firstMouse = true; // ensure re-init when next capture happens
        return;
    }

    if (g_firstMouse) {
        g_lastX = xpos;
        g_lastY = ypos;
        g_firstMouse = false;
    }

    float xoffset = float(xpos - g_lastX);
    float yoffset = float(g_lastY - ypos); // reversed: y ranges top->bottom
    g_lastX = xpos;
    g_lastY = ypos;

    xoffset *= g_mouseSensitivity;
    yoffset *= g_mouseSensitivity;

    cameraYaw += xoffset;
    cameraPitch += yoffset;

    // constrain pitch
    if (cameraPitch > 89.0f) cameraPitch = 89.0f;
    if (cameraPitch < -89.0f) cameraPitch = -89.0f;

    // update camera front vector from yaw/pitch (spherical)
    float yawRad = glm::radians(cameraYaw);
    float pitchRad = glm::radians(cameraPitch);

    glm::vec3 front;
    front.x = cos(yawRad) * cos(pitchRad);
    front.y = sin(pitchRad);
    front.z = sin(yawRad) * cos(pitchRad);
    cameraFront = glm::normalize(front);
}

// Scroll callback: adjust camera vertical eye height
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    const float scrollSpeed = 0.5f; // meters per scroll tick, tune to taste
    const float minY = 0.01f;       // minimum eye height above plane (meters)
    const float maxY = cameraYMeasuring; // use overview height as upper bound

    // Adjust walking-eye-height target
    cameraYWalking += static_cast<float>(yoffset) * scrollSpeed;

    // Clamp the target
    if (cameraYWalking < minY) cameraYWalking = minY;
    if (cameraYWalking > maxY) cameraYWalking = maxY;

    if (!overviewMode) {
        cameraPos.y = cameraYWalking;
    }
}


//      DEPRICATED :2D
// This moves the texture sub-rectangle (mapOffsetX/Y) so the standing man stays visually centered.
void updateMovement(GLFWwindow* window, float dt) {
    // Use a direction vector so combinations (e.g., A+W) work and normalize for constant speed
    float dirX = 0.0f, dirY = 0.0f;

    // WASD drives player/camera movement now (no map panning)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) dirY += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) dirY -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) dirX -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) dirX += 1.0f;

    bool movingRightish = dirX > 0.0f; // true when input has rightward component
    bool movingLeftish  = dirX < 0.0f; // true when input has leftward component
    bool movingUpish    = dirY > 0.0f;
    bool movingDownish  = dirY < 0.0f;

    // We no longer modify mapOffset/mapTexScale/distancePixels here.
    // Keep only animation / facing state so the standing-man sprite (if shown) animates.
    if (dirX != 0.0f || dirY != 0.0f) {
        // normalize so diagonal speed equals cardinal speed (used for animation direction)
        float len = sqrtf(dirX * dirX + dirY * dirY);
        if (len != 0.0f) {
            dirX /= len;
            dirY /= len;
        }
    }

    // update facing/animation only
    if (movingRightish) {
        standingManState = 1; // rightish
        standingManAnimTimer += dt;
        if (standingManAnimTimer >= 0.5f) {
            standingManAnimTimer -= 0.5f;
            standingManAnimFrame ^= 1;
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
    // Forward to updateMovement which now only handles animation state
    updateMovement(window, dt);
}