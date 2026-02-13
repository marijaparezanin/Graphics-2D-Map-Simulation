#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "../Header/Util.h"
#include "../Header/DrawShapes.h"

extern unsigned personalInformationTexture;
extern unsigned mapTexture;
extern unsigned pinTexture;

// read-only global used to decide which texture to draw for the top-left pin
extern bool pinShowsStanding;

extern unsigned standingManTexture;
extern unsigned standingManTextureRight;
extern unsigned standingManTextureRightAlt;
extern unsigned standingManTextureLeft;
extern unsigned standingManTextureLeftAlt;
// up/down frames
extern unsigned standingManTextureUp;
extern unsigned standingManTextureUpAlt;
extern unsigned standingManTextureDown;
extern unsigned standingManTextureDownAlt;

extern float lowerOpacity;
extern float fullOpacity;


extern float mapOffsetX;
extern float mapOffsetY;
extern float mapTexScale;

extern int standingManState;
extern int standingManAnimFrame;

void setupShader(unsigned int shader,
    int texture,
    float x,
    float y,
    float scale,
    float opacity,
    float texOffsetX,
    float texOffsetY,
    float texScale){
    glUseProgram(shader);
    //•	The value 0 tells the shader to use the texture bound to texture unit 0 (i.e., GL_TEXTURE0).
    glUniform1i(glGetUniformLocation(shader, "uTex0"), texture);
    glUniform1f(glGetUniformLocation(shader, "uX"), x);
    glUniform1f(glGetUniformLocation(shader, "uY"), y);
    glUniform1f(glGetUniformLocation(shader, "uS"), scale);
    glUniform1f(glGetUniformLocation(shader, "uOpacity"), opacity);
    glUniform2f(glGetUniformLocation(shader, "uTexOffset"), texOffsetX, texOffsetY);
    glUniform1f(glGetUniformLocation(shader, "uTexScale"), texScale);
}

void drawRect(unsigned int rectShader, unsigned int VAOrect) {
    setupShader(rectShader, 0, 0.0f, 0.0f, 1.0f, lowerOpacity);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, personalInformationTexture);

    glBindVertexArray(VAOrect);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4); // Crtaju se trouglovi tako da formiraju četvorougao
}

// Legacy 2D fullscreen map draw (keeps compatibility with any code still calling drawMap)
void drawMap(unsigned int rectShader, unsigned int VAOmap) {
    setupShader(rectShader, 0, 0.0f, 0.0f, 1.0f, fullOpacity, mapOffsetX, mapOffsetY, mapTexScale);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mapTexture);
    glBindVertexArray(VAOmap);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

// New: draw the 3D map plane. Caller must set uM/uV/uP and uTexOffset/uTexScale before calling.
// This function binds the map texture and issues the draw call.
void drawMap3D(unsigned int mapShader, unsigned int VAOmap) {
    glUseProgram(mapShader);
    // ensure the shader samples texture unit 0
    glUniform1i(glGetUniformLocation(mapShader, "uTex0"), 0);
    // texture pan/scale uniforms may already be set by caller; setting again is harmless
    glUniform2f(glGetUniformLocation(mapShader, "uTexOffset"), mapOffsetX, mapOffsetY);
    glUniform1f(glGetUniformLocation(mapShader, "uTexScale"), mapTexScale);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mapTexture);

    glBindVertexArray(VAOmap);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void drawStandinMan(unsigned int rectShader, unsigned int VAOstandingMan) {
    setupShader(rectShader);

    glActiveTexture(GL_TEXTURE0);
    if (standingManState == 1) {
        if (standingManAnimFrame == 0) {
            glBindTexture(GL_TEXTURE_2D, standingManTextureRight);
        }
        else {
            glBindTexture(GL_TEXTURE_2D, standingManTextureRightAlt);
        }
    } else if (standingManState == 2) { 
        if (standingManAnimFrame == 0) glBindTexture(GL_TEXTURE_2D, standingManTextureLeft);
        else                           glBindTexture(GL_TEXTURE_2D, standingManTextureLeftAlt);
    } else if (standingManState == 3) {
        // moving up
        if (standingManAnimFrame == 0) glBindTexture(GL_TEXTURE_2D, standingManTextureUp);
        else                           glBindTexture(GL_TEXTURE_2D, standingManTextureUpAlt);
    } else if (standingManState == 4) {
        // moving down
        if (standingManAnimFrame == 0) glBindTexture(GL_TEXTURE_2D, standingManTextureDown);
        else                           glBindTexture(GL_TEXTURE_2D, standingManTextureDownAlt);
    } else {
        glBindTexture(GL_TEXTURE_2D, standingManTexture); // default/idle
    }

    glBindVertexArray(VAOstandingMan);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}


void drawTopPin(unsigned int rectShader, unsigned int VAOtopPin, unsigned int VAOtopPinWide) {
    setupShader(rectShader);

    glActiveTexture(GL_TEXTURE0);
    // bind either the pin icon or the standing-man texture depending on toggle
    if (pinShowsStanding) {
        glBindTexture(GL_TEXTURE_2D, standingManTexture);
        glBindVertexArray(VAOtopPinWide); // use wider VAO for standing-man
    }
    else {
        glBindTexture(GL_TEXTURE_2D, pinTexture);
        glBindVertexArray(VAOtopPin); // use original pin VAO
    }
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}