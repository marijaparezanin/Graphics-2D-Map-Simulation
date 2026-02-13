#include "../Header/Util.h";

#define _CRT_SECURE_NO_WARNINGS
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include "../Header/stb_image.h"

// Autor: Nedeljko Tesanovic
// Opis: pomocne funkcije za zaustavljanje programa, ucitavanje sejdera, tekstura i kursora
// Smeju se koristiti tokom izrade projekta

int endProgram(std::string message) {
    std::cout << message << std::endl;
    glfwTerminate();
    return -1;
}

unsigned int compileShader(GLenum type, const char* source)
{
    // Uzima kod iz fajla na putanji "source", kompajlira ga i vraca sejder tipa "type"
    // Citanje izvornog koda iz fajla
    std::string content = "";
    std::ifstream file(source, std::ios::binary);
    std::stringstream ss;
    if (file.is_open())
    {
        ss << file.rdbuf();
        file.close();
        std::cout << "Uspjesno procitao fajl sa putanje \"" << source << "\"!" << std::endl;
    }
    else {
        ss << "";
        std::cout << "Greska pri citanju fajla sa putanje \"" << source << "\"!" << std::endl;
    }
    std::string temp = ss.str();

    // Strip UTF-8 BOM if present (0xEF,0xBB,0xBF)
    if (temp.size() >= 3 &&
        static_cast<unsigned char>(temp[0]) == 0xEF &&
        static_cast<unsigned char>(temp[1]) == 0xBB &&
        static_cast<unsigned char>(temp[2]) == 0xBF) {
        temp.erase(0, 3);
    }

    // Ensure #version is the first meaningful token.
    // Find "#version" and remove anything before it (including whitespace/newlines/comments that may break some drivers).
    const std::string versionToken = "#version";
    size_t pos = temp.find(versionToken);
    if (pos != std::string::npos) {
        // Move back to start of line containing #version to be safe
        size_t lineStart = temp.rfind('\n', pos);
        if (lineStart == std::string::npos) {
            // #version is on first line already - nothing to remove
        } else {
            // remove content before the line that contains #version
            temp.erase(0, lineStart + 1);
        }
    } else {
        // No #version found — GLSL default may accept but better to warn.
        std::cout << "Warning: shader file \"" << source << "\" does not contain a #version directive." << std::endl;
    }

    // Remove any leading stray '\r' characters (safe-guard)
    while (!temp.empty() && (temp.front() == '\r' || temp.front() == '\n')) {
        temp.erase(temp.begin());
    }

    const char* sourceCode = temp.c_str(); // Izvorni kod sejdera

    unsigned int shader = glCreateShader(type); // Napravimo prazan sejder odredjenog tipa (vertex ili fragment)

    int success; // Da li je kompajliranje bilo uspjesno (1 - da)
    char infoLog[1024]; // Poruka o gresci (veci buffer)

    glShaderSource(shader, 1, &sourceCode, NULL); // Postavi izvorni kod sejdera
    glCompileShader(shader); // Kompajliraj sejder

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success); // Provjeri da li je sejder uspjesno kompajliran
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(shader, sizeof(infoLog), NULL, infoLog); // Pribavi poruku o gresci
        if (type == GL_VERTEX_SHADER)
            printf("VERTEX");
        else if (type == GL_FRAGMENT_SHADER)
            printf("FRAGMENT");
        printf(" sejder ima gresku! Greska: \n");
        printf("%s\n", infoLog);
    }
    return shader;
}
unsigned int createShader(const char* vsSource, const char* fsSource)
{
    // Pravi objedinjeni sejder program koji se sastoji od Vertex sejdera ciji je kod na putanji vsSource

    unsigned int program; // Objedinjeni sejder
    unsigned int vertexShader; // Verteks sejder (za prostorne podatke)
    unsigned int fragmentShader; // Fragment sejder (za boje, teksture itd)

    program = glCreateProgram(); // Napravi prazan objedinjeni sejder program

    vertexShader = compileShader(GL_VERTEX_SHADER, vsSource); // Napravi i kompajliraj vertex sejder
    fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSource); // Napravi i kompajliraj fragment sejder

    // Zakaci verteks i fragment sejdere za objedinjeni program
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program); // Povezi ih u jedan objedinjeni sejder program
    glValidateProgram(program); // Izvrsi provjeru novopecenog programa

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success); // Slicno kao za sejdere
    if (success == GL_FALSE)
    {
        glGetProgramInfoLog(program, sizeof(infoLog), NULL, infoLog);
        std::cout << "Objedinjeni sejder ima gresku! Greska: \n";
        std::cout << infoLog << std::endl;
    }

    // Posto su kodovi sejdera u objedinjenom sejderu, oni pojedinacni programi nam ne trebaju, pa ih brisemo zarad ustede na memoriji
    glDetachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    return program;
}

unsigned loadImageToTexture(const char* filePath) {
    int w, h, channels;
    unsigned char* data = stbi_load(filePath, &w, &h, &channels, STBI_rgb_alpha);

    if (!data) {
        std::cout << "Textura nije ucitana! Putanja: " << filePath << std::endl;
        return 0;
    }

    stbi__vertical_flip(data, w, h, 4);

    unsigned tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return tex;
}

GLFWcursor* loadImageToCursor(const char* filePath) {
    int TextureWidth;
    int TextureHeight;
    int TextureChannels;

    unsigned char* ImageData = stbi_load(filePath, &TextureWidth, &TextureHeight, &TextureChannels, 0);

    if (ImageData != NULL)
    {
        GLFWimage image;
        image.width = TextureWidth;
        image.height = TextureHeight;
        image.pixels = ImageData;

        // Tacka na površini slike kursora koja se ponaša kao hitboks, moze se menjati po potrebi
        // Trenutno je gornji levi ugao, odnosno na 20% visine i 20% sirine slike kursora
        int hotspotX = TextureWidth / 5;
        int hotspotY = TextureHeight / 5;

        GLFWcursor* cursor = glfwCreateCursor(&image, hotspotX, hotspotY);
        stbi_image_free(ImageData);
        return cursor;
    }
    else {
        std::cout << "Kursor nije ucitan! Putanja kursora: " << filePath << std::endl;
        stbi_image_free(ImageData);

    }
}