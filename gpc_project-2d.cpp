#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h> 
#include <math.h>

// --- CONSTANTES DE PANTALLA ---
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// --- DEFINICIONES DE LÓGICA ---
#define PI 3.14159265

// PALETA DE COLORES
const float COL_FONDO[3] = {0.85f, 0.82f, 0.75f}; 
const float COL_ROJO[3] = {0.8f, 0.25f, 0.15f};   
const float COL_CAQUI[3] = {0.55f, 0.4f, 0.25f};  
const float COL_OSCURO[3] = {0.15f, 0.15f, 0.15f}; 
const float COL_VERDE_GRIS[3] = {0.3f, 0.35f, 0.3f}; 
const float COL_BLANCO[3] = {1.0f, 1.0f, 1.0f};    
const float COL_MADERA[3] = {0.45f, 0.3f, 0.2f};   
const float COL_METAL[3] = {0.25f, 0.25f, 0.25f};  
const float COL_FUEGO_INT[3] = {1.0f, 1.0f, 0.0f}; 
const float COL_FUEGO_EXT[3] = {1.0f, 0.5f, 0.0f}; 

// ESTADOS
typedef enum { INTRO, DESARROLLO, DISPAROS, CIERRE } EstadoHistoria;
EstadoHistoria estadoActual = INTRO;

typedef enum { FIN_ESPERA_PALOMA, FIN_MIRAR, FIN_SOLTAR, FIN_ABRAZO } SubEstadoFin;
SubEstadoFin subEstadoActual = FIN_ESPERA_PALOMA;

// VARIABLES DE TIEMPO Y ESTADO
double lastTime = 0.0;
int timerFinal = 0; 
int timerGlobal = 0;
int timerDisparos = 0;
int contadorDisparos = 0;
int esFogonazo = 0;

// TEXTURAS
GLuint texturaPlumas = 0;

// Animación
float posPalomaX = 10.0f, posPalomaY = 40.0f;
float dirPalomaX = 1.0f, dirPalomaY = 0.5f;
int numPisadasTotal = 0; 

float posSolIzqX = -15.0f;
float posSolDerX = -35.0f;
int tieneCasco = 0, tieneArmaIzq = 0, tieneArmaDer = 0;

const float POS_CASCO = 35.0f;
const float POS_ARMA_IZQ = 55.0f;
const float POS_ARMA_DER = 75.0f;
float anguloBrazo = 0.0f;
float anguloPierna = 0.0f;

// --- FUNCIONES AUXILIARES DE DIBUJO ---

void colorRGB(const float color[3]) { glColor3fv(color); }
void colorRGBA(const float color[3], float alpha) { glColor4f(color[0], color[1], color[2], alpha); }

void dibujarRect(float w, float h, const float col[3]) {
    colorRGB(col);
    glBegin(GL_QUADS);
        glVertex2f(-w/2, -h/2); glVertex2f(w/2, -h/2);
        glVertex2f(w/2, h/2); glVertex2f(-w/2, h/2);
    glEnd();
}

void dibujarOvalo(float radioX, float radioY, const float col[3]) {
    colorRGB(col);
    glBegin(GL_POLYGON);
    for(int i=0; i<360; i+=15) {
        float rad = i*PI/180;
        glVertex2f(radioX*cos(rad), radioY*sin(rad));
    }
    glEnd();
}

void cargarTextura() {
    printf(">> CARGANDO TEXTURA...\n");
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(1); 
    unsigned char *data = stbi_load("plumas.jpg", &width, &height, &nrChannels, 0);
    
    if (data) {
        glGenTextures(1, &texturaPlumas);
        glBindTexture(GL_TEXTURE_2D, texturaPlumas);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        int formato = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, formato, width, height, 0, formato, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
        printf(">> Textura Cargada.\n");
    } else {
        printf(">> ERROR: No se encontro texturas/plumas.jpg\n");
    }
}

// --- OBJETOS Y PERSONAJES ---

void dibujarFogonazo() {
    glPushMatrix();
    glTranslatef(9.0f, 0.2f, 0.0f); 
    glScalef(1.5f + (rand()%10)/10.0f, 1.5f + (rand()%10)/10.0f, 1.0f); 
    colorRGB(COL_FUEGO_EXT);
    glBegin(GL_TRIANGLES); 
        glVertex2f(0, 2); glVertex2f(1, 0); glVertex2f(-1, 0);
        glVertex2f(0, -2); glVertex2f(1, 0); glVertex2f(-1, 0);
        glVertex2f(2, 0); glVertex2f(0, 1); glVertex2f(0, -1);
    glEnd();
    colorRGB(COL_FUEGO_INT);
    glScalef(0.6f, 0.6f, 1.0f); 
    glBegin(GL_POLYGON); glVertex2f(1,1); glVertex2f(-1,1); glVertex2f(-1,-1); glVertex2f(1,-1); glEnd();
    glColor3f(1.0f, 1.0f, 0.8f);
    glBegin(GL_LINES); glVertex2f(2.0f, 0.0f); glVertex2f(50.0f, 0.0f); glEnd();
    glPopMatrix();
}

void dibujarRifle(int disparando) {
    glPushMatrix();
    dibujarRect(10.0f, 1.2f, COL_MADERA);
    glTranslatef(5.0f, 0.2f, 0.0f);
    dibujarRect(4.0f, 0.6f, COL_METAL);
    if (disparando) dibujarFogonazo();
    glTranslatef(2.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLES); glVertex2f(0.0f, -0.3f); glVertex2f(3.0f, 0.0f); glVertex2f(0.0f, 0.3f); glEnd();
    glPopMatrix();
}

void dibujarSoldadoIzq(float x, float y, int tieneArma, float animPiernas, float animBrazo, int apuntando, int disparando) {
    glPushMatrix(); glTranslatef(x, y, 0.0f);
    glPushMatrix(); glTranslatef(3.0f, 8.0f, -0.1f); glRotatef(-20.0f, 0,0,1); colorRGB(COL_OSCURO);
    glBegin(GL_QUADS); glVertex2f(0, -0.8); glVertex2f(-6, -0.5); glVertex2f(-6, 0.5); glVertex2f(0, 0.8); glEnd();
    glTranslatef(-6.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLES); glVertex2f(0, 0.5); glVertex2f(-3, 2.0); glVertex2f(-0.5, 0); glEnd(); 
    glBegin(GL_TRIANGLES); glVertex2f(0, -0.5); glVertex2f(-3, -2.0); glVertex2f(-0.5, 0); glEnd(); glPopMatrix();
    glPushMatrix(); glTranslatef(-2.0f, -7.0f, 0.0f); glRotatef(animPiernas, 0,0,1); dibujarRect(2.0f, 6.0f, COL_OSCURO);
    glTranslatef(0.0f, -3.0f, 0.0f); dibujarOvalo(2.2f, 1.2f, COL_ROJO); glPopMatrix();
    glPushMatrix(); glTranslatef(2.5f, -7.0f, 0.0f); glRotatef(-animPiernas, 0,0,1); dibujarRect(2.0f, 6.0f, COL_OSCURO);
    glTranslatef(0.0f, -3.0f, 0.0f); dibujarOvalo(2.2f, 1.2f, COL_ROJO); glPopMatrix();
    glPushMatrix(); glRotatef(-5.0f, 0,0,1); dibujarOvalo(4.5f, 7.5f, COL_BLANCO); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 4.5f, 0.1f); colorRGB(COL_VERDE_GRIS); 
    glBegin(GL_TRIANGLES); glVertex2f(-3.0f, 1.5f); glVertex2f(3.0f, 1.5f); glVertex2f(0.0f, -2.5f); glEnd(); glPopMatrix();
    glPushMatrix(); glTranslatef(0.5f, 8.0f, 0.1f); dibujarOvalo(2.8f, 3.2f, COL_ROJO); 
    glTranslatef(0.0f, 2.5f, 0.1f); glRotatef(-10.0f, 0,0,1); dibujarRect(4.0f, 1.5f, COL_BLANCO); 
    glTranslatef(0.0f, 1.0f, 0.0f); dibujarOvalo(2.0f, 1.0f, COL_BLANCO); glPopMatrix();
    glPushMatrix(); glTranslatef(3.5f, 2.5f, 0.2f);
    if (disparando) glTranslatef(-2.0f, 0.0f, 0.0f);
    if (apuntando) glRotatef(30.0f, 0,0,1); else glRotatef(animBrazo, 0,0,1);
    glPushMatrix(); glTranslatef(-0.5f, 1.5f,0.1f); glRotatef(-10,0,0,1); dibujarRect(2.5f, 3.0f, COL_BLANCO); glPopMatrix(); 
    dibujarOvalo(1.5f, 3.5f, COL_ROJO); 
    if (tieneArma) { glTranslatef(1.0f, -2.0f, 0.0f); glRotatef(70.0f, 0,0,1); dibujarRifle(disparando); }
    glPopMatrix(); glPopMatrix();
}

void dibujarSoldadoDer(float x, float y, int tieneCasco, int tieneArma, float animPiernas, float animBrazo, int apuntando, int disparando) {
    glPushMatrix(); glTranslatef(x, y, 0.0f);
    glPushMatrix(); glTranslatef(-2.0f, -7.0f, 0.0f); glRotatef(animPiernas, 0,0,1); dibujarRect(2.2f, 5.0f, COL_VERDE_GRIS);
    glTranslatef(0.0f, -3.0f, 0.0f); dibujarOvalo(2.0f, 1.0f, COL_VERDE_GRIS); glPopMatrix();
    glPushMatrix(); glTranslatef(2.0f, -7.0f, 0.0f); glRotatef(-animPiernas, 0,0,1); dibujarRect(2.2f, 5.0f, COL_VERDE_GRIS);
    glTranslatef(0.0f, -3.0f, 0.0f); dibujarOvalo(2.0f, 1.0f, COL_VERDE_GRIS); glPopMatrix();
    colorRGB(COL_CAQUI); glBegin(GL_POLYGON); glVertex2f(-4, 6); glVertex2f(4, 6); glVertex2f(7, -5); glVertex2f(-6, -5); glEnd();
    glPushMatrix(); glTranslatef(0.0f, 7.0f, 0.1f);
    if (tieneCasco) {
        colorRGB(COL_OSCURO); dibujarRect(4.5f, 2.5f, COL_OSCURO);
        glBegin(GL_TRIANGLES); glVertex2f(-2.25, 1.25); glVertex2f(2.25, 1.25); glVertex2f(0, 5); glEnd();
        glTranslatef(0.0f, 2.0f, 0.1f); dibujarOvalo(0.8f, 0.8f, COL_ROJO); 
    } else { dibujarOvalo(2.5f, 3.0f, COL_ROJO); }
    glPopMatrix();
    glPushMatrix(); glTranslatef(4.0f, 2.0f, 0.2f);
    if (disparando) glTranslatef(-2.0f, 0.0f, 0.0f);
    if (apuntando) glRotatef(40.0f, 0,0,1); else glRotatef(animBrazo, 0,0,1);
    glPushMatrix(); glTranslatef(0.0f, -2.5f, -0.1f); dibujarOvalo(1.4f, 1.4f, COL_VERDE_GRIS); glPopMatrix();
    dibujarRect(2.0f, 5.0f, COL_CAQUI); 
    if (tieneArma) { glTranslatef(0.0f, -2.5f, 0.0f); glRotatef(60.0f, 0,0,1); dibujarRifle(disparando); 
    glTranslatef(3.0f, 0.5f, 0.1f); colorRGBA(COL_FONDO, 0.8f); dibujarOvalo(1.5f, 1.5f, COL_FONDO); }
    glPopMatrix(); glPopMatrix();
}

void dibujarPaloma(float x, float y, int mirandoAbajo) {
    glPushMatrix(); glTranslatef(x, y, 0.0f);
    if (mirandoAbajo) glRotatef(-30.0f, 0,0,1);
    float aleteo = sin(timerGlobal * 0.01f) * 3.0f;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaPlumas);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_POLYGON); 
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-4,0);
        glTexCoord2f(0.5f, 1.0f); glVertex2f(0,-2);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(4,0);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(6,3);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-2,4);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    colorRGBA(COL_BLANCO, 0.6f); 
    glBegin(GL_TRIANGLES); glVertex2f(-2, 2); glVertex2f(-8, 6 + aleteo); glVertex2f(2, 4); glEnd();
    glBegin(GL_TRIANGLES); glVertex2f(2, 2); glVertex2f(8, 6 + aleteo); glVertex2f(-2, 4); glEnd();
    glPopMatrix();
}

// --- ESCENAS ---
void dibujarIntro() {
    dibujarPaloma(posPalomaX, posPalomaY, 0);
    colorRGB(COL_OSCURO);
    float angulo = atan2(dirPalomaX, dirPalomaY) * 180 / PI;
    for(int i=0; i < numPisadasTotal; i++) {
        int esPersona2 = (i >= 12);
        int indicePaso = esPersona2 ? (i - 12) : i;
        int esPieIzquierdo = (indicePaso % 2 == 0);
        float offsetLateralPie = esPieIzquierdo ? -1.5f : 1.5f;
        float offsetPersona = esPersona2 ? 8.0f : 0.0f; 
        float distancia = 10.0f + indicePaso * 7.0f; 
        float pxBase = distancia * dirPalomaX; float pyBase = distancia * dirPalomaY;
        float offsetTotal = offsetLateralPie + offsetPersona;
        float pxFinal = pxBase - offsetTotal * dirPalomaY; float pyFinal = pyBase + offsetTotal * dirPalomaX;
        glPushMatrix(); glTranslatef(pxFinal, pyFinal, 0.0f); glRotatef(angulo - 90, 0,0,1);
        if (esPersona2) dibujarOvalo(1.4f, 0.7f, COL_VERDE_GRIS); else dibujarRect(2.5f, 1.2f, COL_OSCURO); 
        glPopMatrix();
    }
}

void dibujarDesarrollo() {
    glColor3f(0.8f, 0.77f, 0.7f); glRectf(0.0f, 0.0f, 100.0f, 15.0f);
    if (!tieneCasco) {
        glPushMatrix(); glTranslatef(POS_CASCO, 17.0f, 0.0f); glRotatef(-20,0,0,1);
        colorRGB(COL_OSCURO); dibujarRect(4.0f, 2.0f, COL_OSCURO);
        glBegin(GL_TRIANGLES); glVertex2f(-2,1); glVertex2f(2,1); glVertex2f(0,4); glEnd(); glPopMatrix();
    }
    if (!tieneArmaIzq) { glPushMatrix(); glTranslatef(POS_ARMA_IZQ, 16.0f, 0.0f); glRotatef(5,0,0,1); dibujarRifle(0); glPopMatrix(); }
    if (!tieneArmaDer) { glPushMatrix(); glTranslatef(POS_ARMA_DER, 16.0f, 0.0f); glRotatef(-5,0,0,1); dibujarRifle(0); glPopMatrix(); }
    dibujarSoldadoIzq(posSolIzqX, 25.0f, tieneArmaIzq, anguloPierna, anguloBrazo, 0, 0);
    dibujarSoldadoDer(posSolDerX, 25.0f, tieneCasco, tieneArmaDer, -anguloPierna, -anguloBrazo, 0, 0);
}

void dibujarDisparos() {
    glColor3f(0.8f, 0.77f, 0.7f); glRectf(0.0f, 0.0f, 100.0f, 15.0f);
    dibujarSoldadoIzq(posSolIzqX, 25.0f, 1, 0, 0, 1, esFogonazo);
    dibujarSoldadoDer(posSolDerX, 25.0f, tieneCasco, 1, 0, 0, 1, esFogonazo);
}

void dibujarCierre() {
    glColor3f(0.8f, 0.77f, 0.7f); glRectf(0.0f, 0.0f, 100.0f, 15.0f);
    if (subEstadoActual >= FIN_SOLTAR) {
         glPushMatrix(); glTranslatef(50.0f, 16.0f, 0.0f); glRotatef(10,0,0,1); dibujarRifle(0); glPopMatrix();
         glPushMatrix(); glTranslatef(60.0f, 16.0f, 0.0f); glRotatef(-15,0,0,1); dibujarRifle(0); glPopMatrix();
    }
    int apuntando = (subEstadoActual == FIN_ESPERA_PALOMA || subEstadoActual == FIN_MIRAR);
    float abrazoAnim = (subEstadoActual == FIN_ABRAZO) ? 45.0f : (apuntando ? 0 : anguloBrazo);
    float offsetIzq = (subEstadoActual == FIN_ABRAZO) ? 7.0f : 0.0f;
    float offsetDer = (subEstadoActual == FIN_ABRAZO) ? -7.0f : 0.0f;
    dibujarSoldadoIzq(posSolIzqX + offsetIzq, 25.0f, (subEstadoActual < FIN_SOLTAR), 0, abrazoAnim, apuntando, 0);
    dibujarSoldadoDer(posSolDerX + offsetDer, 25.0f, tieneCasco, (subEstadoActual < FIN_SOLTAR), 0, -abrazoAnim, apuntando, 0);
    dibujarPaloma(posPalomaX, posPalomaY, (subEstadoActual == FIN_MIRAR));
}

// --- CALLBACKS GLFW ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// --- LOGICA  ---
void update(int ms) {
    timerGlobal += ms;
    if (estadoActual == INTRO) {
        posPalomaX += dirPalomaX * 0.6f; posPalomaY += dirPalomaY * 0.6f;
        if (timerGlobal > 800 * (numPisadasTotal + 1) && numPisadasTotal < 24) numPisadasTotal++;
        if (timerGlobal > 20000) estadoActual = DESARROLLO;
    }
    else if (estadoActual == DESARROLLO) {
        anguloPierna = sin(timerGlobal * 0.005f) * 30.0f;
        anguloBrazo = sin(timerGlobal * 0.005f) * 15.0f;
        if (posSolIzqX < 45.0f) posSolIzqX += 0.25f; if (posSolDerX < 65.0f) posSolDerX += 0.2f;
        if (posSolIzqX > POS_ARMA_IZQ - 5) tieneArmaIzq = 1; if (posSolDerX > POS_CASCO - 5) tieneCasco = 1; if (posSolDerX > POS_ARMA_DER - 5) tieneArmaDer = 1;
        if (posSolIzqX >= 45.0f && posSolDerX >= 65.0f) { estadoActual = DISPAROS; timerDisparos = 0; contadorDisparos = 0; }
    }
    else if (estadoActual == DISPAROS) {
        timerDisparos += ms; esFogonazo = 0; 
        if (timerDisparos > 500 && contadorDisparos == 0) { contadorDisparos++; } if (timerDisparos > 500 && timerDisparos < 600) esFogonazo = 1;
        if (timerDisparos > 1500 && contadorDisparos == 1) { contadorDisparos++; } if (timerDisparos > 1500 && timerDisparos < 1600) esFogonazo = 1;
        if (timerDisparos > 2500 && contadorDisparos == 2) { contadorDisparos++; } if (timerDisparos > 2500 && timerDisparos < 2600) esFogonazo = 1;
        if (timerDisparos > 3500) { estadoActual = CIERRE; subEstadoActual = FIN_ESPERA_PALOMA; timerFinal = 0; posPalomaX = -20.0f; posPalomaY = 60.0f; }
    }
    else if (estadoActual == CIERRE) {
        timerFinal += ms;
        if (subEstadoActual == FIN_ESPERA_PALOMA) { posPalomaX += 0.5f; posPalomaY += sin(timerGlobal * 0.005f) * 0.1f; if (posPalomaX > 20.0f) { subEstadoActual = FIN_MIRAR; timerFinal = 0; } }
        else if (subEstadoActual == FIN_MIRAR) { if (timerFinal > 2000) { subEstadoActual = FIN_SOLTAR; timerFinal = 0; } }
        else if (subEstadoActual == FIN_SOLTAR) { if (timerFinal > 1000) subEstadoActual = FIN_ABRAZO; }
    }
}

// --- MAIN ---
int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Proyecto Lebedev", NULL, NULL);
    if (window == NULL) {
        printf("Fallo al crear ventana GLFW\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Fallo al inicializar GLAD\n");
        return -1;
    }

    glEnable(GL_BLEND); 
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    cargarTextura();

    // Loop Principal
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        if (currentTime - lastTime >= 0.016) {
            update(16);
            lastTime = currentTime;

            glClearColor(COL_FONDO[0], COL_FONDO[1], COL_FONDO[2], 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0.0, 100.0, 0.0, 100.0, -10.0, 10.0);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            switch(estadoActual) {
                case INTRO: dibujarIntro(); break;
                case DESARROLLO: dibujarDesarrollo(); break;
                case DISPAROS: dibujarDisparos(); break;
                case CIERRE: dibujarCierre(); break;
            }

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    glfwTerminate();
    return 0;
}
