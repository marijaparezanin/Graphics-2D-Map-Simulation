#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "../Header/Util.h"

extern int screenWidth;
extern int screenHeight;

extern unsigned int VAOrect;
extern unsigned int VAOmap;
extern unsigned int VAOstandingMan;
extern unsigned int VAOtopPin;
extern unsigned int VAOtopPinWide;
        
// Donji ljevi 
float verticesPersonalInfoRect[] = {
    -1.0f, -1.0f, 0.0f, 0.0f, // bottom-left
    -1.0f, -0.9f, 0.0f, 1.0f, // top-left
    -0.82f, -0.9f, 1.0f, 1.0f, // top-right
    -0.82f, -1.0f, 1.0f, 0.0f  // bottom-right
};

// NOTE: only changed the texture coordinates to un-mirror the map (flipped U).
float verticesMapPlane[] = {
    // pos.x, pos.y, pos.z,    normal.x, normal.y, normal.z,    tex.u, tex.v
    -0.5f, 0.0f, -0.5f,        0.0f, 1.0f, 0.0f,                 1.0f, 0.0f, // bottom-left (U flipped)
    -0.5f, 0.0f,  0.5f,        0.0f, 1.0f, 0.0f,                 1.0f, 1.0f, // top-left
     0.5f, 0.0f,  0.5f,        0.0f, 1.0f, 0.0f,                 0.0f, 1.0f, // top-right
     0.5f, 0.0f, -0.5f,        0.0f, 1.0f, 0.0f,                 0.0f, 0.0f  // bottom-right
};


void formMapVAO(unsigned int& VAOmap){
    size_t mapSize = sizeof(verticesMapPlane);
    unsigned int VBOmap;
    glGenVertexArrays(1, &VAOmap);
    glGenBuffers(1, &VBOmap);
    glBindVertexArray(VAOmap);
    glBindBuffer(GL_ARRAY_BUFFER, VBOmap);
    glBufferData(GL_ARRAY_BUFFER, mapSize, verticesMapPlane, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

void formTopPinVAO(unsigned int& VAOtopPin, float marginPx)
{
    // Pixel size of the pin sprite (original pin image)
    float pxW = 80.0f;
    float pxH = 109.0f;

    float ndcW = (pxW / (float)screenWidth) * 2.0f;
    float ndcH = (pxH / (float)screenHeight) * 2.0f;

    float marginNdcX = (marginPx / (float)screenWidth) * 2.0f;
    float marginNdcY = (marginPx / (float)screenHeight) * 2.0f;

    // Top-left placement with margin:
    float left  = -1.0f + marginNdcX;
    float right = left + ndcW;
    float top    = 1.0f - marginNdcY;
    float bottom = top - ndcH;

    float vertices[] = {
        left,  bottom,  0.0f, 0.0f,
        left,  top,     0.0f, 1.0f,
        right, top,     1.0f, 1.0f,
        right, bottom,  1.0f, 0.0f
    };

    unsigned int VBO;
    glGenVertexArrays(1, &VAOtopPin);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAOtopPin);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

// New: wider VAO for standing-man appearance (square-ish)
void formTopPinWideVAO(unsigned int& VAOtopPinWide, float marginPx)
{
    // Wider square for standing-man icon (use same height as pin to keep consistent vertical size)
    float pxW = 109.0f; // make wider so standing-man appears wider than the pin
    float pxH = 109.0f;

    float ndcW = (pxW / (float)screenWidth) * 2.0f;
    float ndcH = (pxH / (float)screenHeight) * 2.0f;

    float marginNdcX = (marginPx / (float)screenWidth) * 2.0f;
    float marginNdcY = (marginPx / (float)screenHeight) * 2.0f;

    // Top-left placement with margin:
    float left  = -1.0f + marginNdcX;
    float right = left + ndcW;
    float top    = 1.0f - marginNdcY;
    float bottom = top - ndcH;

    float vertices[] = {
        left,  bottom,  0.0f, 0.0f,
        left,  top,     0.0f, 1.0f,
        right, top,     1.0f, 1.0f,
        right, bottom,  1.0f, 0.0f
    };

    unsigned int VBO;
    glGenVertexArrays(1, &VAOtopPinWide);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAOtopPinWide);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}


void formStandingManVAO(unsigned int& VAOstandingMan)
{
    float pxW = 50.0f;
    float pxH = 50.0f;

    float ndcW = (pxW / screenWidth) * 2.0f;
    float ndcH = (pxH / screenHeight) * 2.0f;

    float halfW = ndcW * 0.5f;
    float halfH = ndcH * 0.5f;

    float vertices[] = {
        -halfW, -halfH, 0.0f, 0.0f,
        -halfW,  halfH, 0.0f, 1.0f,
         halfW,  halfH, 1.0f, 1.0f,
         halfW, -halfH, 1.0f, 0.0f
    };

    unsigned int VBO;
    glGenVertexArrays(1, &VAOstandingMan);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAOstandingMan);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}


void formInformationRectVAO(unsigned int& VAOrect) {
    unsigned int VBOrect;
	size_t rectSize = sizeof(verticesPersonalInfoRect);
    glGenVertexArrays(1, &VAOrect);
    glGenBuffers(1, &VBOrect);

    glBindVertexArray(VAOrect);
    glBindBuffer(GL_ARRAY_BUFFER, VBOrect);
    glBufferData(GL_ARRAY_BUFFER, rectSize, verticesPersonalInfoRect, GL_STATIC_DRAW);

    // Atribut 0 (pozicija):
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Atribut 1 (boja):
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void formAllVAOs(unsigned int& VAOmap,
    unsigned int& VAOstandingMan,
    unsigned int& VAOrect
    
 )
{
    // formiranje VAO-ova je izdvojeno u posebnu funkciju radi čitljivijeg koda u main funkciji

// Podsetnik za atribute:
/*
    Jedan VAO se vezuje za jedan deo celokupnog skupa verteksa na sceni.
    Na primer, dobra praksa je da se jedan VAO vezuje za jedan VBO koji se vezuje za jedan objekat, odnosno niz temena koja opisuju objekat.

    VAO je pomoćna struktura koja opisuje kako se podaci u nizu objekta interpretiraju.
    U render-petlji, za crtanje određenog objekta, naredbom glBindVertexArray(nekiVAO) se određuje koji se objekat crta.

    Potrebno je definisati koje atribute svako teme u nizu sadrži, npr. poziciju na lokaciji 0 i boju na lokaciji 1.

    Ova konfiguracija je specifična za naš primer na vežbama i može se menjati za različite potrebe u projektu.


    Atribut se opisuje metodom glVertexAttribPointer(). Argumenti su redom:
        index - identifikacioni kod atributa, u verteks šejderu je povezan preko svojstva location (location = 0 u šejderu -> indeks tog atributa treba staviti isto 0 u ovoj naredbi)
        size - broj vrednosti unutar atributa (npr. za poziciju su x i y, odnosno 2 vrednosti; za boju r, g i b, odnosno 3 vrednosti)
        type - tip vrednosti
        normalized - da li je potrebno mapirati na odgovarajući opseg (mi poziciju već inicijalizujemo na opseg (-1, 1), a boju (0, 1), tako da nam nije potrebno)
        stride - koliko elemenata u nizu treba preskočiti da bi se došlo od datog atributa u jednom verteksu do istog tog atributa u sledećem verteksu
        pointer - koliko elemenata u nizu treba preskočiti od početka niza da bi se došlo do prvog pojavljivanja datog atributa
*/

    formInformationRectVAO(VAOrect);
    formMapVAO(VAOmap);
	formStandingManVAO(VAOmap);

}
