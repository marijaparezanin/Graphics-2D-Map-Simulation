#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "../Header/Text.h"
#include "../Header/stb_easy_font.h"
#include "../Header/Util.h"


static GLuint textProgram = 0;
static GLuint textVBO = 0;
static GLuint textVAO = 0;
static GLint uResolutionLoc = -1;
static GLint uColorLoc = -1;

void initText() {
    // create shader program for text (uses createShader from Util.h)
    textProgram = createShader("text.vert", "text.frag");
    uResolutionLoc = glGetUniformLocation(textProgram, "uResolution");
    uColorLoc = glGetUniformLocation(textProgram, "uColor");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);

    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);

}

void drawText(const char* text, float xPx, float yPx, float r, float g, float b) {
    if (!textProgram || !text) {
        return;
    }

    // scale factor (6x bigger)
    const float scale = 6.0f;

    // compute text bounding box using stb helper (returns px for base size)
    int baseW = stb_easy_font_width((char*)text);
    int baseH = stb_easy_font_height((char*)text);
    float textW = baseW * scale;
    float textH = baseH * scale;

    // small padding around background, scaled
    const float pad = 4.0f * scale;

    // generate stb_easy_font geometry into a char buffer (binary floats + color)
    static char buffer[99999];
    int quads = stb_easy_font_print((float)xPx, (float)yPx, (char*)text, NULL, buffer, sizeof(buffer));
    if (quads <= 0) return;

    // Draw background rectangle first (semi-transparent black)
    // rectangle in pixel coordinates: top-left = (xPx - pad, yPx - pad)
    // stb_easy_font uses y increasing downward, so this aligns correctly.
    float bx0 = xPx - pad;
    float by0 = yPx - pad;
    float bx1 = xPx + textW + pad;
    float by1 = yPx + textH + pad;

    // two triangles for rectangle (6 verts)
    float rectVerts[12] = {
        bx0, by0,
        bx1, by0,
        bx1, by1,

        bx0, by0,
        bx1, by1,
        bx0, by1
    };

    // stb_easy_font produces quads: 4 vertices per quad, each vertex is 16 bytes:
    // float x; float y; float z; unsigned char color[4];
    // We must convert quads (4 verts) into triangles (6 verts) and upload only x,y floats,
    // and apply scaling about the text origin (xPx, yPx).
    const int vertsPerQuadInBuf = 4;
    const int bytesPerVertexInBuf = 16; // 4+4+4+4
    int nverts_tri = quads * 6;                     // 6 vertices per quad for triangles
    std::vector<float> verts;
    verts.reserve(nverts_tri * 2);

    for (int q = 0; q < quads; ++q) {
        int baseVert = q * vertsPerQuadInBuf;
        // read the 4 quad vertices (x,y) from the raw buffer and scale them about (xPx, yPx)
        float vx[4], vy[4];
        for (int i = 0; i < 4; ++i) {
            char* vptr = buffer + (baseVert + i) * bytesPerVertexInBuf;
            // x at offset 0, y at offset 4
            float fx = *(float*)(vptr + 0);
            float fy = *(float*)(vptr + 4);
            // scale about anchor (xPx, yPx)
            float sx = xPx + (fx - xPx) * scale;
            float sy = yPx + (fy - yPx) * scale;
            vx[i] = sx;
            vy[i] = sy;
        }

        // make two triangles: (0,1,2) and (0,2,3)
        verts.push_back(vx[0]); verts.push_back(vy[0]);
        verts.push_back(vx[1]); verts.push_back(vy[1]);
        verts.push_back(vx[2]); verts.push_back(vy[2]);

        verts.push_back(vx[0]); verts.push_back(vy[0]);
        verts.push_back(vx[2]); verts.push_back(vy[2]);
        verts.push_back(vx[3]); verts.push_back(vy[3]);
    }

    int nbytes_rect = sizeof(rectVerts);
    int nbytes_text = static_cast<int>(verts.size() * sizeof(float));

    // Use program and set uniforms
    glUseProgram(textProgram);

    int fbW = 0, fbH = 0;
    GLFWwindow* ctx = glfwGetCurrentContext();
    if (ctx) glfwGetFramebufferSize(ctx, &fbW, &fbH);
    if (fbW == 0) fbW = 1;
    if (fbH == 0) fbH = 1;
    glUniform2f(uResolutionLoc, (float)fbW, (float)fbH);

    // Bind VAO/VBO once and reuse for rect + text
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);

    // Re-specify attribute pointer for the bound VBO (robust against other VAO/VBO usage)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    // Save state we will change
    GLboolean prevDepthTest = glIsEnabled(GL_DEPTH_TEST);
    GLboolean prevCullFace = glIsEnabled(GL_CULL_FACE);
    GLint prevDepthMask;
    glGetIntegerv(GL_DEPTH_WRITEMASK, &prevDepthMask);

    // Ensure overlay draws on top
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);

    // draw background (semi-transparent black)
    // set uniform color with alpha (text.frag now uses vec4)
    glUniform4f(uColorLoc, 0.0f, 0.0f, 0.0f, 0.6f);
    glBufferData(GL_ARRAY_BUFFER, nbytes_rect, rectVerts, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // draw text (opaque)
    glUniform4f(uColorLoc, r, g, b, 1.0f);
    glBufferData(GL_ARRAY_BUFFER, nbytes_text, verts.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, nverts_tri);

    // Restore GL state
    glDepthMask((GLboolean)prevDepthMask);
    if (prevDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (prevCullFace)  glEnable(GL_CULL_FACE);  else glDisable(GL_CULL_FACE);

    glBindVertexArray(0);
}

void cleanupText() {
    if (textVBO) { glDeleteBuffers(1, &textVBO); textVBO = 0; }
    if (textVAO) { glDeleteVertexArrays(1, &textVAO); textVAO = 0; }
    if (textProgram) { glDeleteProgram(textProgram); textProgram = 0; }
}