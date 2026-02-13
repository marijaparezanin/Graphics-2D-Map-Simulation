#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <cmath>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../Header/Measurement3D.h"
#include "../Header/Globals.h"
#include "../Header/Util.h" // for createShader()

// Simple low-poly sphere + cone generator for pins and line rendering
static unsigned measurementProg = 0;
static unsigned sphereVAO = 0, sphereVBO = 0, sphereEBO = 0, sphereCount = 0;
static unsigned coneVAO = 0, coneVBO = 0, coneEBO = 0, coneCount = 0;
static unsigned lineVAO = 0, lineVBO = 0;

// helper to create shader program (uses existing project helper)
static unsigned createMeasurementShader() {
    return createShader("measurement3d.vert", "measurement3d.frag");
}

static void buildSphere(int stacks = 8, int slices = 16) {
    std::vector<float> verts;
    std::vector<unsigned> idx;
    for (int i = 0; i <= stacks; ++i) {
        float V = float(i) / float(stacks);
        float phi = V * glm::pi<float>();
        for (int j = 0; j <= slices; ++j) {
            float U = float(j) / float(slices);
            float theta = U * (glm::pi<float>() * 2.0f);
            float x = std::cos(theta) * std::sin(phi);
            float y = std::cos(phi);
            float z = std::sin(theta) * std::sin(phi);
            verts.push_back(x); verts.push_back(y); verts.push_back(z);
            verts.push_back(x); verts.push_back(y); verts.push_back(z);
        }
    }
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            int first = i * (slices + 1) + j;
            int second = first + slices + 1;
            idx.push_back(first);
            idx.push_back(second);
            idx.push_back(first + 1);
            idx.push_back(second);
            idx.push_back(second + 1);
            idx.push_back(first + 1);
        }
    }

    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned), idx.data(), GL_STATIC_DRAW);

    // pos(3) + normal(3)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
    sphereCount = (unsigned)idx.size();
}

// Build a cone where TIP is at y=0 (point at bottom) and BASE ring is at y=1 (wider at top).
static void buildCone(int slices = 24) {
    std::vector<float> verts;
    std::vector<unsigned> idx;

    // TIP vertex at y = 0
    verts.push_back(0.0f); verts.push_back(0.0f); verts.push_back(0.0f); // pos
    verts.push_back(0.0f); verts.push_back(-1.0f); verts.push_back(0.0f); // normal (approx)

    // ring vertices at y = 1 (base)
    for (int i = 0; i <= slices; ++i) {
        float theta = (float)i / float(slices) * glm::two_pi<float>();
        float x = std::cos(theta);
        float z = std::sin(theta);
        verts.push_back(x); verts.push_back(1.0f); verts.push_back(z);
        glm::vec3 n = glm::normalize(glm::vec3(x, 0.5f, z));
        verts.push_back(n.x); verts.push_back(n.y); verts.push_back(n.z);
    }

    unsigned tipIndex = 0;
    unsigned ringStart = 1;

    // side triangles (connect tip -> ring)
    for (int i = 0; i < slices; ++i) {
        idx.push_back(tipIndex);
        idx.push_back(ringStart + i + 1);
        idx.push_back(ringStart + i);
    }

    // optional base cap: center vertex at y=1
    unsigned centerIndex = (unsigned)(verts.size() / 6);
    verts.push_back(0.0f); verts.push_back(1.0f); verts.push_back(0.0f);
    verts.push_back(0.0f); verts.push_back(1.0f); verts.push_back(0.0f);
    for (int i = 0; i < slices; ++i) {
        idx.push_back(centerIndex);
        idx.push_back(ringStart + i);
        idx.push_back(ringStart + i + 1);
    }

    glGenVertexArrays(1, &coneVAO);
    glGenBuffers(1, &coneVBO);
    glGenBuffers(1, &coneEBO);

    glBindVertexArray(coneVAO);
    glBindBuffer(GL_ARRAY_BUFFER, coneVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coneEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned), idx.data(), GL_STATIC_DRAW);

    // pos(3) + normal(3)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
    coneCount = (unsigned)idx.size();
}

void initMeasurement3D() {
    measurementProg = createMeasurementShader();
    buildSphere(10, 20);
    buildCone(32);

    // line VAO (dynamic)
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void shutdownMeasurement3D() {
    if (sphereVAO) { glDeleteVertexArrays(1, &sphereVAO); sphereVAO = 0; }
    if (sphereVBO) { glDeleteBuffers(1, &sphereVBO); sphereVBO = 0; }
    if (sphereEBO) { glDeleteBuffers(1, &sphereEBO); sphereEBO = 0; }
    if (coneVAO) { glDeleteVertexArrays(1, &coneVAO); coneVAO = 0; }
    if (coneVBO) { glDeleteBuffers(1, &coneVBO); coneVBO = 0; }
    if (coneEBO) { glDeleteBuffers(1, &coneEBO); coneEBO = 0; }
    if (lineVAO) { glDeleteVertexArrays(1, &lineVAO); lineVAO = 0; }
    if (lineVBO) { glDeleteBuffers(1, &lineVBO); lineVBO = 0; }
    if (measurementProg) { glDeleteProgram(measurementProg); measurementProg = 0; }
}

void drawMeasurements3D(const glm::mat4& view, const glm::mat4& projection, float planeScale) {
    if (measurementPoints.empty()) return;
    GLFWwindow* ctx = glfwGetCurrentContext();
    if (!ctx) return;

    int fbW = 0, fbH = 0;
    glfwGetFramebufferSize(ctx, &fbW, &fbH);
    if (fbW == 0 || fbH == 0) return;

    glUseProgram(measurementProg);
    GLint locM = glGetUniformLocation(measurementProg, "uM");
    GLint locV = glGetUniformLocation(measurementProg, "uV");
    GLint locP = glGetUniformLocation(measurementProg, "uP");
    GLint locColor = glGetUniformLocation(measurementProg, "uColor");

    glUniformMatrix4fv(locV, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(locP, 1, GL_FALSE, glm::value_ptr(projection));

    // Convert stored framebuffer measurement points -> world coords using unProject + ray-plane intersection.
    // That correctly accounts for camera perspective and tilt.
    std::vector<glm::vec3> worldPts;
    worldPts.reserve(measurementPoints.size());
    for (auto &p : measurementPoints) {
        // p holds framebuffer pixel coords (x,y).
        float px = p.first;
        float py = p.second;

        // window Y origin for glm::unProject is bottom-left, so flip Y:
        float winY = float(fbH) - py;

        glm::vec3 nearWin(px, winY, 0.0f);
        glm::vec3 farWin(px, winY, 1.0f);

        // unProject with model = view (we want camera-space -> world)
        glm::vec3 nearW = glm::unProject(nearWin, view, projection, glm::vec4(0, 0, fbW, fbH));
        glm::vec3 farW  = glm::unProject(farWin,  view, projection, glm::vec4(0, 0, fbW, fbH));

        glm::vec3 dir = glm::normalize(farW - nearW);

        // intersect ray with plane y = 0
        if (fabs(dir.y) < 1e-6f) {
            // nearly parallel -> fallback to projecting to plane using texture mapping as a best-effort
            float u_screen = px / float(fbW);
            float v_screen = py / float(fbH);
            float texU = saved_mapOffsetX + u_screen * saved_mapTexScale;
            float texV = saved_mapOffsetY + v_screen * saved_mapTexScale;
            float worldX = (0.5f - texU) * planeScale;
            float worldZ = (texV - 0.5f) * planeScale;
            worldPts.emplace_back(worldX, 0.0f, worldZ);
        } else {
            float t = (0.0f - nearW.y) / dir.y;
            if (t < 0.0f) {
                // intersection behind near plane; fallback same as above
                float u_screen = px / float(fbW);
                float v_screen = py / float(fbH);
                float texU = saved_mapOffsetX + u_screen * saved_mapTexScale;
                float texV = saved_mapOffsetY + v_screen * saved_mapTexScale;
                float worldX = (0.5f - texU) * planeScale;
                float worldZ = (texV - 0.5f) * planeScale;
                worldPts.emplace_back(worldX, 0.0f, worldZ);
            } else {
                glm::vec3 hit = nearW + dir * t;
                worldPts.push_back(glm::vec3(hit.x, 0.0f, hit.z));
            }
        }
    }

    // Draw line segments (slightly above plane)
    std::vector<glm::vec3> lineVerts;
    lineVerts.reserve(worldPts.size());
    for (auto &wp : worldPts) {
        glm::vec3 v = wp;
        v.y += 0.02f; // slight lift so it is visible above plane
        lineVerts.push_back(v);
    }

    // upload dynamic line vertices and draw
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, lineVerts.size() * sizeof(glm::vec3), lineVerts.data(), GL_DYNAMIC_DRAW);

    // line color = blue (sky blue)
    glUniform4f(locColor, 123.0f/255.0f, 194.0f/255.0f, 252.0f/255.0f, 1.0f);
    glUniformMatrix4fv(locM, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
    glLineWidth(3.0f);
    glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)lineVerts.size());
    glLineWidth(1.0f);  

    // Draw pins: cone needle (tip at plane) + red sphere on top
    float needleHeight = 0.9f;    // needle height in world units
    float needleRadius = 0.09f;   // base radius (will scale model)
    float sphereRadius = 0.25f;   // ball size

    // last added index (only this one is lit)
    int lastIndex = (int)worldPts.size() - 1;

    for (size_t i = 0; i < worldPts.size(); ++i) {
        glm::vec3 base = worldPts[i];

        // Cone: model has tip at y=0 and base at y=1 -> translate to plane then scale
        glm::mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(base.x, 0.0f, base.z));
        M = glm::scale(M, glm::vec3(needleRadius, needleHeight, needleRadius));
        glUniformMatrix4fv(locM, 1, GL_FALSE, glm::value_ptr(M));
        glUniform4f(locColor, 0.6f, 0.6f, 0.6f, 1.0f); // grey needle
        glBindVertexArray(coneVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)coneCount, GL_UNSIGNED_INT, 0);

        // Sphere: positioned at top of needle (centered)
        glm::mat4 Ms = glm::translate(glm::mat4(1.0f), glm::vec3(base.x, needleHeight + sphereRadius, base.z));
        Ms = glm::scale(Ms, glm::vec3(sphereRadius));
        glUniformMatrix4fv(locM, 1, GL_FALSE, glm::value_ptr(Ms));

        // If this is the last added point, render it brighter and draw a stronger, tighter additive glow.
        if ((int)i == lastIndex) {
            // Brighter core (pure red)
            glUniform4f(locColor, 1.0f, 0.0f, 0.0f, 1.0f);
            glBindVertexArray(sphereVAO);
            glDrawElements(GL_TRIANGLES, (GLsizei)sphereCount, GL_UNSIGNED_INT, 0);

            // Glow: additive blended, smaller spread and stronger red
            GLboolean prevBlend = glIsEnabled(GL_BLEND);
            GLint prevDepthMask;
            glGetIntegerv(GL_DEPTH_WRITEMASK, &prevDepthMask);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDepthMask(GL_FALSE);

            // tighter shells configuration: smaller scale multipliers and higher alpha for intensity
            const float shellScales[3] = { 1.25f, 1.9f, 2.8f };
            const float shellAlphas[3] = { 0.85f, 0.55f, 0.30f };

            for (int s = 0; s < 3; ++s) {
                glm::mat4 Mg = glm::translate(glm::mat4(1.0f), glm::vec3(base.x, needleHeight + sphereRadius, base.z));
                Mg = glm::scale(Mg, glm::vec3(sphereRadius * shellScales[s]));
                glUniformMatrix4fv(locM, 1, GL_FALSE, glm::value_ptr(Mg));
                // pure red shells
                glUniform4f(locColor, 1.0f, 0.0f, 0.0f, shellAlphas[s]);
                glBindVertexArray(sphereVAO);
                glDrawElements(GL_TRIANGLES, (GLsizei)sphereCount, GL_UNSIGNED_INT, 0);
            }

            // restore
            glDepthMask((GLboolean)prevDepthMask);
            if (!prevBlend) glDisable(GL_BLEND);

            // restore model matrix for any subsequent objects
            glUniformMatrix4fv(locM, 1, GL_FALSE, glm::value_ptr(Ms));
        } else {
            // regular (unlit) red sphere
            glUniform4f(locColor, 212.0f/255.0f, 3.0f/255.0f, 3.0f/255.0f, 1.0f);
            glBindVertexArray(sphereVAO);
            glDrawElements(GL_TRIANGLES, (GLsizei)sphereCount, GL_UNSIGNED_INT, 0);
        }
    }

    glBindVertexArray(0);
    glUseProgram(0);
}
