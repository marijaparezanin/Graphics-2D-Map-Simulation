#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cmath> // for sqrt
#include <string>
#include <vector>
#include <utility>
#include <cfloat>
#include <algorithm>
#include "../Header/stb_easy_font.h"

#include "../Header/Util.h"
#include "../Header/Callbacks.h"
#include "../Header/DrawShapes.h"
#include "../Header/CreateVAOs.h"
#include "../Header/Text.h"
#include "../Header/OverlayDraw.h"
#include "../Header/Globals.h" 
#include "../Header/SupermanGlobals.h" 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../Header/model.hpp"
#include "../Header/Measurement3D.h"

// runtime model switching support
static Model* activeModel = nullptr;
static glm::vec3 activeModelCenter(0.0f);
static float activeModelScale = 1.0f;

// per-model orientation offsets (degrees)
// activeModelYawOffsetDeg is applied around Y (heading). activeModelPitchOffsetDeg around X (tilt).
static float activeModelYawOffsetDeg = 0.0f;
static float activeModelPitchOffsetDeg = 0.0f;

// helper: load model and compute center/scale similar to previous code
static void loadActiveModel(const std::string& filepath, const float desiredHeight = 1.5f)
{
    if (activeModel) {
        delete activeModel;
        activeModel = nullptr;
    }

    activeModel = new Model(filepath);

    // compute bounding box
    glm::vec3 bbMin(FLT_MAX), bbMax(-FLT_MAX);
    for (const Mesh &mesh : activeModel->meshes) {
        for (const Vertex &v : mesh.vertices) {
            bbMin.x = std::min(bbMin.x, v.Position.x);
            bbMin.y = std::min(bbMin.y, v.Position.y);
            bbMin.z = std::min(bbMin.z, v.Position.z);
            bbMax.x = std::max(bbMax.x, v.Position.x);
            bbMax.y = std::max(bbMax.y, v.Position.y);
            bbMax.z = std::max(bbMax.z, v.Position.z);
        }
    }

    activeModelCenter = (bbMin + bbMax) * 0.5f;
    float modelHeight = (bbMax.y - bbMin.y);
    if (modelHeight <= 0.0f) modelHeight = 1.0f;
    activeModelScale = glm::clamp(desiredHeight / modelHeight, 0.001f, 10.0f);

    // load upright.
    activeModelYawOffsetDeg = 0.0f;
    activeModelPitchOffsetDeg = 0.0f;
    activeModelYawOffsetDeg = -90.0f;
}
                                                                                                    
static void updateSupermanMovement(GLFWwindow* window,
    glm::vec3& supermanPos,
    float& supermanYawDeg,
    glm::vec3& prevSupermanPos,
    float& supermanMeters,
    float dt,
    float moveSpeed,
    float turnSpeed)
{
    glm::vec3 inputDir(0.0f);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) inputDir += glm::vec3(0.0f, 0.0f, 1.0f); // forward -> +Z
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) inputDir -= glm::vec3(0.0f, 0.0f, 1.0f); // back    -> -Z
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) inputDir += glm::vec3(1.0f, 0.0f, 0.0f); // right   -> +X
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) inputDir -= glm::vec3(1.0f, 0.0f, 0.0f); // left    -> -X

    if (glm::length(inputDir) > 1e-6f) {
        glm::vec3 moveDir = glm::normalize(glm::vec3(inputDir.x, 0.0f, inputDir.z));

        // desired yaw (world-space) so Superman faces movement direction
        float desiredYaw = glm::degrees(std::atan2(moveDir.z, moveDir.x));

        // shortest angular difference
        float diff = fmodf(desiredYaw - supermanYawDeg + 540.0f, 360.0f) - 180.0f;

        // clamp turn per frame for smooth rotation
        float maxDelta = turnSpeed * dt;
        float appliedDelta = glm::clamp(diff, -maxDelta, maxDelta);
        supermanYawDeg += appliedDelta;

        // Move along moveDir in world-space
        supermanPos += glm::vec3(-moveDir.x, 0.0f, moveDir.z) * moveSpeed * dt;

        // Clamp to plane bounds
        const float planeScaleLocal = 20.0f;
        const float mapHalfLocal = planeScaleLocal * 0.5f;
        supermanPos.x = glm::clamp(supermanPos.x, -mapHalfLocal - cameraClampMargin, mapHalfLocal + cameraClampMargin);
        supermanPos.z = glm::clamp(supermanPos.z, -mapHalfLocal - cameraClampMargin, mapHalfLocal + cameraClampMargin);

        // Accumulate traveled distance (convert world units -> meters)
        float moved = glm::length(supermanPos - prevSupermanPos);
        if (moved > 1e-6f) {
            supermanMeters += moved * METERS_PER_WORLD_UNIT;
            prevSupermanPos = supermanPos;
        }
    }
    // no input: keep yaw/position unchanged
}

// set model lighting and related material uniforms
static void applyModelLighting(Shader& shader, const glm::vec3& modelWorldPos, float modelScale, const glm::vec3& cameraPos, const glm::vec3& frontDir)
{
    float frontDist = glm::max(0.8f, modelScale * 1.2f);
    float verticalOffset = glm::max(0.6f, modelScale * 0.6f);
    glm::vec3 modelLightPos = modelWorldPos + frontDir * frontDist + glm::vec3(0.0f, verticalOffset, 0.0f);

    shader.setVec3("uLightPos", modelLightPos.x, modelLightPos.y, modelLightPos.z);
    shader.setFloat("uLightIntensity", 0.4f);
    shader.setVec3("uLightColor", 1.0f, 1.0f, 1.0f);

    shader.setVec3("uFrontDir", frontDir.x, frontDir.y, frontDir.z);
    shader.setFloat("uFrontIntensity", 0.5f);
    shader.setFloat("uAmbientFactor", 0.55f);

    shader.setFloat("uSpecularStrength", 0.5f);
    shader.setFloat("uShininess", 24.0f);

    shader.setVec3("uViewPos", cameraPos.x, cameraPos.y, cameraPos.z);
}


void formAllVAOs()
{
    formInformationRectVAO(VAOrect);
    formMapVAO(VAOmap);                // now creates a 3D plane 
    formStandingManVAO(VAOstandingMan);
    formTopPinVAO(VAOtopPin);
    formTopPinWideVAO(VAOtopPinWide);
}

//uzeto sa vjezbi
void preprocessTexture(unsigned& texture, const char* filepath) {
    texture = loadImageToTexture(filepath); // Učitavanje teksture
    glBindTexture(GL_TEXTURE_2D, texture); // Vezujemo se za teksturu kako bismo je podesili

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // S - texels x
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // T - texels y

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}


void setupTextures() {
    //glClearColor(0.2f, 0.8f, 0.6f, 1.0f);
    preprocessTexture(personalInformationTexture, "Resources/personal-info.png");
    preprocessTexture(mapTexture, "Resources/novi-sad-map-0.jpg");
    preprocessTexture(pinTexture, "Resources/pin-icon1.png");

    preprocessTexture(standingManTexture, "Resources/icon_standing.png");

    /* DEPRICATED 2D
    //running little guy
    preprocessTexture(standingManTextureRight, "Resources/man_running_right.png");
    preprocessTexture(standingManTextureRightAlt, "Resources/man_running_right_alt.png");
    preprocessTexture(standingManTextureLeft, "Resources/man_running_left.png");
    preprocessTexture(standingManTextureLeftAlt, "Resources/man_running_left_alt.png");

    // up/down frames (add your up/down frame files to Resources)
    preprocessTexture(standingManTextureUp, "Resources/man_walking_up1.png");
    preprocessTexture(standingManTextureUpAlt, "Resources/man_walking_up_alt1.png");
    preprocessTexture(standingManTextureDown, "Resources/man_walking_up1.png");
    preprocessTexture(standingManTextureDownAlt, "Resources/man_walking_up_alt1.png");
    */
}

void renderDistance(GLFWwindow* window) {
    // Draw the text overlay
    // disable depth/culling and enable alpha blending for overlay text
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    char buf[128];
    int fbW = 0, fbH = 0;
    glfwGetFramebufferSize(window, &fbW, &fbH);

    if (overviewMode) {
        float effectiveSavedScale = (saved_mapTexScale > 0.0f) ? saved_mapTexScale : 1.0f;
        float walkingEquivalentPixels = measurementDistancePixels / effectiveSavedScale;
        int meters = (int)roundf(walkingEquivalentPixels * METERS_PER_PIXEL);
        snprintf(buf, sizeof(buf), "%dm", meters);
        float textWidthPx = float(stb_easy_font_width(buf)) * TEXT_SCALE;
        float margin = 8.0f;
        drawText(buf, fbW - textWidthPx - margin, margin, 1.0f, 1.0f, 1.0f);
    } else {
        int meters = (int)roundf(supermanMeters);
        snprintf(buf, sizeof(buf), "%dm", meters);
        float textWidthPx = float(stb_easy_font_width(buf)) * TEXT_SCALE;
        float margin = 8.0f;
        drawText(buf, fbW - textWidthPx - margin, margin, 1.0f, 1.0f, 1.0f);
    }
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    screenWidth = mode->width;
    screenHeight = mode->height;
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "3D Map (walking view)", monitor, NULL);
    if (window == NULL) return endProgram("Prozor nije uspeo da se kreirati.");
    glfwMakeContextCurrent(window);

    // Set callbacks (unchanged)
    glfwSetMouseButtonCallback(window, center_callback);
    glfwSetKeyCallback(window, key_callback);
    // register mouse motion callback for look-around
    glfwSetCursorPosCallback(window, mouse_move_callback);
    // register scroll callback to move camera vertically
    glfwSetScrollCallback(window, scroll_callback);


    cursor = loadImageToCursor("Resources/compass-icon-left.png");
    cursorPressed = loadImageToCursor("Resources/compass-icon-right.png");
    glfwSetCursor(window, cursor);
    if (glewInit() != GLEW_OK) return endProgram("GLEW nije uspeo da se inicijalizuje.");

    // Performance / rendering state tweaks
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Transparency for HUD
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                
    // make clear color brighter sky bluergb(123, 194, 252)
    glClearColor(123 / 255.0f, 194.0f / 255.0f, 252.0f / 255.0f, 1.0f);
    setupTextures();

    // create shaders:
    unsigned int rectShader = createShader("rect.vert", "rect.frag");     // existing 2D overlay shader
    unsigned int map3DShader = createShader("map3d.vert", "map3d.frag");  // new 3D map shader

    // initialize measurement 3D (shader + simple meshes)
    initMeasurement3D();

    Shader modelShader("basic.vert", "basic.frag");

    loadActiveModel("Resources\\superman.glb", requestModelLoadHeight);

    // previous position for distance calc
    glm::vec3 prevSupermanPos = supermanPos;

    formAllVAOs();
    initText(); 

    // start centered on map
    mapOffsetX = (1.0f - mapTexScale) * 0.5f;
    mapOffsetY = (1.0f - mapTexScale) * 0.5f;

    bool map3DUseFullTexture = false;

    double prevTime = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        double initFrameTime = glfwGetTime();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        double now = glfwGetTime();
        float dt = float(now - prevTime);
        prevTime = now;

        // Handle any pending model-reload requests (set by key callbacks M/B).
        if (requestReloadModel) {
            loadActiveModel("Resources\\superman.glb", requestModelLoadHeight);
            requestReloadModel = false;
        }
        // Only update movement and distance when not in overview
        if (!overviewMode) {
            updateMapMovement(window, dt);  
        }       

        if (map3DUseFullTexture) {
            mapOffsetX = 0.0f;
            mapOffsetY = 0.0f;
            mapTexScale = 1.0f; // sample the whole texture
        }

        // if in overview mode camera is fixed
        if (!overviewMode) {
            float camMoveSpeed = cameraSpeed * dt;
            float yawRad = glm::radians(cameraYaw);
            glm::vec3 forwardXZ = glm::normalize(glm::vec3(cos(yawRad), 0.0f, sin(yawRad)));
            glm::vec3 right = glm::normalize(glm::cross(forwardXZ, cameraUp));

            // Camera movement 
            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    cameraPos += forwardXZ * camMoveSpeed;
            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  cameraPos -= forwardXZ * camMoveSpeed;
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  cameraPos -= right * camMoveSpeed;
            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) cameraPos += right * camMoveSpeed;

            const float planeScale = 20.0f;
            const float mapHalf = planeScale * 0.5f;
            cameraPos.x = glm::clamp(cameraPos.x, -mapHalf - cameraClampMargin, mapHalf + cameraClampMargin);
            cameraPos.z = glm::clamp(cameraPos.z, -mapHalf - cameraClampMargin, mapHalf + cameraClampMargin);
            cameraPos.y = cameraYWalking;
        } else {
            cameraPos = glm::vec3(measureCamX, measureCamY, measureCamZ);
            cameraFront = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - cameraPos);
            cameraYaw = glm::degrees(atan2(cameraFront.z, cameraFront.x));
            cameraPitch = glm::degrees(asin(glm::clamp(cameraFront.y, -1.0f, 1.0f)));
            measureCamYaw = cameraYaw;
            measureCamPitch = cameraPitch;
        }

        // compute view from camera globals
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        // --- 3D map: enable depth test and render using the 3D shader ---
        glEnable(GL_DEPTH_TEST);
        glUseProgram(map3DShader);

        // model matrix for the plane
        glm::mat4 model = glm::mat4(1.0f);
        const float planeScale = 20.0f;
        model = glm::scale(model, glm::vec3(planeScale, 1.0f, planeScale));

        // compute projection
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)screenWidth / (float)screenHeight, 0.005f, 100.0f);

        // upload matrices
        glUniformMatrix4fv(glGetUniformLocation(map3DShader, "uM"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(map3DShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(map3DShader, "uP"), 1, GL_FALSE, glm::value_ptr(projection));

        // pass texture pan/scale
        glUniform2f(glGetUniformLocation(map3DShader, "uTexOffset"), mapOffsetX, mapOffsetY);
        glUniform1f(glGetUniformLocation(map3DShader, "uTexScale"), mapTexScale);

        // Disable horizontal flip
        int flipX = 0;
        glUniform1i(glGetUniformLocation(map3DShader, "uFlipX"), flipX);

        // Upload scene light uniforms
        GLint locLightPos       = glGetUniformLocation(map3DShader, "uLightPos");
        GLint locLightColor     = glGetUniformLocation(map3DShader, "uLightColor");
        GLint locLightIntensity = glGetUniformLocation(map3DShader, "uLightIntensity");
        GLint locLightRadius    = glGetUniformLocation(map3DShader, "uLightRadius");
        GLint locViewPos        = glGetUniformLocation(map3DShader, "uViewPos");
        GLint locLightDir       = glGetUniformLocation(map3DShader, "uLightDir");
        GLint locLightDirectional = glGetUniformLocation(map3DShader, "uLightDirectional");

        if (locLightPos >= 0)       glUniform3f(locLightPos, sceneLightPos.x, sceneLightPos.y, sceneLightPos.z);
        if (locLightColor >= 0)     glUniform3f(locLightColor, sceneLightColor.r, sceneLightColor.g, sceneLightColor.b);
        if (locLightIntensity >= 0) glUniform1f(locLightIntensity, sceneLightIntensity);
        if (locLightRadius >= 0)    glUniform1f(locLightRadius, sceneLightRadius);
        if (locViewPos >= 0)        glUniform3f(locViewPos, cameraPos.x, cameraPos.y, cameraPos.z);
        if (locLightDir >= 0)      glUniform3f(locLightDir, sceneLightDir.x, sceneLightDir.y, sceneLightDir.z);
        if (locLightDirectional >= 0) glUniform1i(locLightDirectional, sceneLightDirectional ? 1 : 0);

        // draw the 3D map plane
        drawMap3D(map3DShader, VAOmap);

        if (!overviewMode && activeModel) {
            modelShader.use();

            // update movement
            updateSupermanMovement(window, supermanPos, supermanYawDeg, prevSupermanPos, supermanMeters, dt, supermanMoveSpeed, supermanTurnSpeed);

            // Build model matrix from current position & orientation
            const float verticalLift = (desiredModelHeight * 0.5f + 0.05f);

            glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(supermanPos.x, verticalLift, supermanPos.z));

            // Apply per-model pitch (X axis) and yaw (Y axis) offsets
            glm::mat4 Rp = glm::rotate(glm::mat4(1.0f), glm::radians(activeModelPitchOffsetDeg), glm::vec3(1.0f, 0.0f, 0.0f));
            glm::mat4 Ry = glm::rotate(glm::mat4(1.0f), glm::radians(supermanYawDeg + activeModelYawOffsetDeg), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 R = Ry * Rp;

            glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(activeModelScale));
            glm::mat4 C = glm::translate(glm::mat4(1.0f), -activeModelCenter);

            glm::mat4 mModel = T * R * S * C;

            // compute a model-local world position (origin transformed by mModel)
            glm::vec3 modelWorldPos = glm::vec3(mModel * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

            // determine forward direction toward camera (XZ only) for frontal fill light
            glm::vec3 toCamera = glm::normalize(cameraPos - modelWorldPos);
            glm::vec3 frontDir = glm::normalize(glm::vec3(toCamera.x, 0.0f, toCamera.z));
            if (glm::length(frontDir) < 0.001f) frontDir = glm::vec3(0.0f, 0.0f, -1.0f);

            applyModelLighting(modelShader, modelWorldPos, activeModelScale, cameraPos, frontDir);

            modelShader.setMat4("uM", mModel);
            modelShader.setMat4("uV", view);
            modelShader.setMat4("uP", projection);

            modelShader.setVec3("uFrontDir", frontDir.x, frontDir.y, frontDir.z);

            modelShader.setVec3("uViewPos", cameraPos.x, cameraPos.y, cameraPos.z);

            // draw active model
            activeModel->Draw(modelShader);
        }

        // after 3D objects i render 2D (personal info rect, topleft pin) 
        glDisable(GL_DEPTH_TEST);

        drawRect(rectShader, VAOrect);

        // DEPRICATED: 2D
        // drawStandinMan(rectShader, VAOstandingMan);

        drawTopPin(rectShader, VAOtopPin, VAOtopPinWide);

        // measurement overlay now rendered as 3D objects in overview:
        if (overviewMode && !measurementPoints.empty()) {
            const float planeScale = 20.0f;
            drawMeasurements3D(view, projection, planeScale);
        }

        // Render distance (either measurement or walking)
        renderDistance(window);

        glfwSwapBuffers(window);
        glfwPollEvents();      

        // Frame limiter (75 FPS)
        while (glfwGetTime() - initFrameTime < 1 / 75.0) {}
    }

    cleanupText();
    shutdownMeasurement3D();

    if (activeModel) { delete activeModel; activeModel = nullptr; }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
