#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>

#include "../Header/OverlayDraw.h"
#include "../Header/Util.h" // for createShader

// Local shader + GL objects used to draw simple overlay geometry in pixel coords.
// We reuse the same text shaders ("text.vert"/"text.frag") which provide a
// uResolution uniform and uColor (vec4) uniform expected below.

static GLuint overlayProg = 0;
static GLuint overlayVAO = 0;
static GLuint overlayVBO = 0;
static GLint uResolutionLoc = -1;
static GLint uColorLoc = -1;

static void ensureInitialized() {
    if (overlayProg != 0) return;

    overlayProg = createShader("text.vert", "text.frag");
    uResolutionLoc = glGetUniformLocation(overlayProg, "uResolution");
    uColorLoc = glGetUniformLocation(overlayProg, "uColor");

    glGenVertexArrays(1, &overlayVAO);
    glGenBuffers(1, &overlayVBO);

    glBindVertexArray(overlayVAO);
    glBindBuffer(GL_ARRAY_BUFFER, overlayVBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void drawPolylinePixels(const std::vector<float>& pts, float r, float g, float b, float a) {
    if (pts.empty()) return;
    ensureInitialized();

    GLFWwindow* ctx = glfwGetCurrentContext();
    if (!ctx) return;

    int fbW = 0, fbH = 0;
    glfwGetFramebufferSize(ctx, &fbW, &fbH);
    if (fbW == 0) fbW = 1;
    if (fbH == 0) fbH = 1;

    // Save GL state we change
    GLboolean prevDepthTest = glIsEnabled(GL_DEPTH_TEST);
    GLboolean prevCullFace = glIsEnabled(GL_CULL_FACE);
    GLint prevDepthMask;
    glGetIntegerv(GL_DEPTH_WRITEMASK, &prevDepthMask);
    GLfloat prevLineWidth;
    glGetFloatv(GL_LINE_WIDTH, &prevLineWidth);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(overlayProg);
    glUniform2f(uResolutionLoc, (float)fbW, (float)fbH);

    glBindVertexArray(overlayVAO);
    glBindBuffer(GL_ARRAY_BUFFER, overlayVBO);
    glBufferData(GL_ARRAY_BUFFER, pts.size() * sizeof(float), pts.data(), GL_DYNAMIC_DRAW);

    GLsizei nverts = static_cast<GLsizei>(pts.size() / 2);

    // Draw black border (thicker)
    glUniform4f(uColorLoc, 0.0f, 0.0f, 0.0f, a);
    glLineWidth(6.0f);
    glDrawArrays(GL_LINE_STRIP, 0, nverts);

    // Draw actual colored line on top (thinner)
    glUniform4f(uColorLoc, r, g, b, a);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINE_STRIP, 0, nverts);

    // Restore GL state
    glLineWidth(prevLineWidth);
    glDepthMask((GLboolean)prevDepthMask);
    if (prevDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (prevCullFace)  glEnable(GL_CULL_FACE);  else glDisable(GL_CULL_FACE);
}

void drawPointsPixels(const std::vector<float>& pts, float r, float g, float b, float a, float pxSize) {
    if (pts.empty()) return;
    ensureInitialized();

    GLFWwindow* ctx = glfwGetCurrentContext();
    if (!ctx) return;

    int fbW = 0, fbH = 0;
    glfwGetFramebufferSize(ctx, &fbW, &fbH);
    if (fbW == 0) fbW = 1;
    if (fbH == 0) fbH = 1;

    // Save GL state we change
    GLboolean prevDepthTest = glIsEnabled(GL_DEPTH_TEST);
    GLboolean prevCullFace = glIsEnabled(GL_CULL_FACE);
    GLint prevDepthMask;
    glGetIntegerv(GL_DEPTH_WRITEMASK, &prevDepthMask);
    GLfloat prevPointSize;
    glGetFloatv(GL_POINT_SIZE, &prevPointSize);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(overlayProg);
    glUniform2f(uResolutionLoc, (float)fbW, (float)fbH);

    glBindVertexArray(overlayVAO);
    glBindBuffer(GL_ARRAY_BUFFER, overlayVBO);
    glBufferData(GL_ARRAY_BUFFER, pts.size() * sizeof(float), pts.data(), GL_DYNAMIC_DRAW);

    GLsizei nverts = static_cast<GLsizei>(pts.size() / 2);

    // Draw black border points first (slightly larger)
    glUniform4f(uColorLoc, 0.0f, 0.0f, 0.0f, a);
    glPointSize(pxSize + 6.0f); // border thickness
    glDrawArrays(GL_POINTS, 0, nverts);

    // Draw colored points on top (requested size)
    glUniform4f(uColorLoc, r, g, b, a);
    glPointSize(pxSize);
    glDrawArrays(GL_POINTS, 0, nverts);

    // Restore GL state
    glPointSize(prevPointSize);
    glDepthMask((GLboolean)prevDepthMask);
    if (prevDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (prevCullFace)  glEnable(GL_CULL_FACE);  else glDisable(GL_CULL_FACE);
}