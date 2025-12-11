#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cmath> // for sqrt
#include <string>
#include <vector>
#include <utility>
#include "../Header/stb_easy_font.h"

#include "../Header/Util.h"
#include "../Header/Callbacks.h"
#include "../Header/DrawShapes.h"
#include "../Header/CreateVAOs.h"
#include "../Header/Text.h"
#include "../Header/OverlayDraw.h"
#include "../Header/Globals.h" 


void formAllVAOs()
{
    formInformationRectVAO(VAOrect);
    formMapVAO(VAOmap);
    formStandingManVAO(VAOstandingMan);
    formTopPinVAO(VAOtopPin);
    formTopPinWideVAO(VAOtopPinWide);
}

//uzeto sa vjezbi
void preprocessTexture(unsigned& texture, const char* filepath) {
    texture = loadImageToTexture(filepath); // Učitavanje teksture
    glBindTexture(GL_TEXTURE_2D, texture); // Vezujemo se za teksturu kako bismo je podesili

    // Generisanje mipmapa - predefinisani različiti formati za lakše skaliranje po potrebi (npr. da postoji 32 x 32 verzija slike, ali i 16 x 16, 256 x 256...)
    glGenerateMipmap(GL_TEXTURE_2D);

    // Podešavanje strategija za wrap-ovanje - šta da radi kada se dimenzije teksture i poligona ne poklapaju
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // S - tekseli po x-osi
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // T - tekseli po y-osi

    // Podešavanje algoritma za smanjivanje i povećavanje rezolucije: nearest - bira najbliži piksel, linear - usrednjava okolne piksele
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}


void setupTextures() {
    //glClearColor(0.2f, 0.8f, 0.6f, 1.0f);
    preprocessTexture(personalInformationTexture, "Resources/personal-info.png");
    preprocessTexture(mapTexture, "Resources/novi-sad-map-0.jpg");
    preprocessTexture(pinTexture, "Resources/pin-icon1.png");

    //running little guy
    preprocessTexture(standingManTexture, "Resources/icon_standing.png");
    preprocessTexture(standingManTextureRight, "Resources/man_running_right.png");
    preprocessTexture(standingManTextureRightAlt, "Resources/man_running_right_alt.png");
    preprocessTexture(standingManTextureLeft, "Resources/man_running_left.png");
    preprocessTexture(standingManTextureLeftAlt, "Resources/man_running_left_alt.png");

    // up/down frames 
    preprocessTexture(standingManTextureUp, "Resources/man_walking_up1.png");
    preprocessTexture(standingManTextureUpAlt, "Resources/man_walking_up_alt1.png");
    preprocessTexture(standingManTextureDown, "Resources/man_walking_up1.png");
    preprocessTexture(standingManTextureDownAlt, "Resources/man_walking_up_alt1.png");
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
        // Convert measurement (recorded in framebuffer pixels at overview scale)
        // to the same pixel basis used by walking distance:
        // walking uses pixels = texture_delta * (screenWidth / mapTexScale_saved)
        // measurementDistancePixels (pts in framebuffer when mapTexScale==1)
        // -> equivalent walking pixels = measurementDistancePixels / saved_mapTexScale
        float effectiveSavedScale = (saved_mapTexScale > 0.0f) ? saved_mapTexScale : 1.0f;
        float walkingEquivalentPixels = measurementDistancePixels / effectiveSavedScale;
        int meters = (int)roundf(walkingEquivalentPixels * METERS_PER_PIXEL);
        snprintf(buf, sizeof(buf), "%dm", meters);
        float textWidthPx = float(stb_easy_font_width(buf)) * TEXT_SCALE;
        float margin = 8.0f;
        drawText(buf, fbW - textWidthPx - margin, margin, 1.0f, 1.0f, 1.0f);
    } else {
        // walking distance (already accumulated in the same pixel basis)
        int meters = (int)roundf(distancePixels * METERS_PER_PIXEL); // meter-per-pixel ratio
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

	//Uzeto sa vjezbi za full screen
    // Formiranje prozora za prikaz sa datim dimenzijama i naslovom
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    screenWidth = mode->width;
    screenHeight = mode->height;

    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "2D Map", monitor, NULL);
    if (window == NULL) return endProgram("Prozor nije uspeo da se kreira.");
    glfwMakeContextCurrent(window);

    glfwSetMouseButtonCallback(window, center_callback);
    glfwSetKeyCallback(window, key_callback);


    cursor = loadImageToCursor("Resources/compass-icon-left.png");
    cursorPressed = loadImageToCursor("Resources/compass-icon-right.png");
    glfwSetCursor(window, cursor);
    if (glewInit() != GLEW_OK) return endProgram("GLEW nije uspeo da se inicijalizuje.");


    // Potrebno naglasiti da program koristi alfa kanal za providnost
    glDisable(GL_DEPTH_TEST); 
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	setupTextures();
    
    unsigned int rectShader = createShader("rect.vert", "rect.frag");
	
    formAllVAOs();

    initText(); // Initialize text rendering

    // i want to start at the center of the map
    mapOffsetX = (1.0f - mapTexScale) * 0.5f;
    mapOffsetY = (1.0f - mapTexScale) * 0.5f;

    // inicjalizacija tajmera
    double prevTime = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        // Tajmer na početku frejma
        double initFrameTime = glfwGetTime();
        glClear(GL_COLOR_BUFFER_BIT);


        // Obračunavanje delta vremena
        double now = glfwGetTime();
        float dt = float(now - prevTime);
        prevTime = now;

        // Only update movement and distance when not in overview
        if (!overviewMode) {
            updateMapMovement(window, dt);
        }

        drawMap(rectShader, VAOmap);
        drawRect(rectShader, VAOrect);

        // draw standing man only when not in overview
        if (!overviewMode) {
            drawStandinMan(rectShader, VAOstandingMan);
        }

        // top-left pin still drawn in both views (draw appropriate VAO/texture based on pinShowsStanding)
        drawTopPin(rectShader, VAOtopPin, VAOtopPinWide);

        // Measurement overlay in overview: draw polyline and points
        if (overviewMode && !measurementPoints.empty()) {
            std::vector<float> pts;
            pts.reserve(measurementPoints.size() * 2);
            for (auto &p : measurementPoints) {
                pts.push_back(p.first);
                pts.push_back(p.second);
            }
            // white polyline and white points
            drawPolylinePixels(pts, 1.0f, 1.0f, 1.0f, 1.0f);
            drawPointsPixels(pts, 1.0f, 1.0f, 1.0f, 1.0f, 8.0f);
        }

        // Render distance (either measurement or walking)
        renderDistance(window);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // Frame limiter: kada se frejm iscrta, čeka se da prođe 1 / 60 sekunde, zatim se prelazi na sledeći frejm frejma
		// Uzeto sa vjezbi, samo sam promjenila na 75 FPS
        while (glfwGetTime() - initFrameTime < 1 / 75.0) {}
    }

    cleanupText(); // Clean up text resources
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}