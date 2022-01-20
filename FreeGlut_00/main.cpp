// Includes:
#include <Windows.h>
#include <algorithm> // Para usar la funcion sort()

// Cosas necesarias para importar modelos:
#include <string>
#include <fstream>
#include <math.h>
#include <time.h>
#include <vector>
#include <GL/freeglut.h>
#include <GL/glut.h>
#include <SOIL2.h>
#include <math.h> // necesitamos esta biblioteca para calcular la posicion "adelante" de la camara
#include <iostream>
#include <stdlib.h>

#include "imageloader.h" // La usaremos para cargar el canal alpha
#include "vec3f.h"



// Bibliografia:
// Instalacion: https://www.youtube.com/watch?v=A1LqGsyl3C4&ab_channel=TechLearners
// Blog interesante: http://www.videotutorialsrock.com/opengl_tutorial/basic_shapes/text.php
// Primera persona: https://thepentamollisproject.blogspot.com/2018/02/setting-up-first-person-camera-in.html
// Importar modelos3d: https://www.youtube.com/watch?v=CpEfHhSpQrw&ab_channel=Kh%C3%A1nhNguy%E1%BB%85n

#define FPS 60
#define TO_RADIANS 3.14/180.0 // esto lo usaremos para convertir rapidamente de grados a radianes

//width and height of the window ( Aspect ration 16:9 )
const int width = 16 * 50;
const int height = 9 * 50;

// Estructuras de datos y variables globales usadas para la camara y movimiento:
float pitch = 0.0, yaw = 0.0; // Angulos x(pitch) e y(yaw) para la rotacion de la camara
float camX = 0.0, camZ = 0.0; // posicion de la camara

// Variables globales Jesus
const float PI = 3.1415926535f;
const float BOX_SIZE = 7.0f; // Longitud de cada cara del cubo
const float ALPHA = 0.3f; // Opacidad

// Particulas
const float GRAVITY = 3.0f;
const int NUM_PARTICLES = 1000;
const float STEP_TIME = 0.01f;
// Anchura de los cuadrados que conforman cada particula
const float PARTICLE_SIZE = 0.05f;

// Estructura usada para almacenar el movimiento
struct Motion
{
    bool Forward, Backward, Left, Right;
};

Motion motion = { false,false,false,false };

// Prototipos
void handleKeypress(unsigned char key, int x, int y);
void initRendering();
void handleResize(int w, int h);
void display();
void drawScene();
void drawModels();
void update(int value);

// Prototipos de funciones usadas para la camara fps:
void passive_motion(int, int);
void camera();
void keyboard(unsigned char key, int x, int y);
void keyboard_up(unsigned char key, int x, int y);

// Nos permite conocer la posicion en que se encuentra la cara del cubo, esto es necesario para encontrar donde
// estan las caras para ver si estan detras o delante de otra cara.
// Estos 3 vectores son perpendiculares los unos a los otros y son unitarios, el vector out nos ayudara a encontrar
// el centro de la cara ya que se trata del vector Normal de la misma
struct Face {
    Vec3f up;
    Vec3f right;
    Vec3f out;
};

// Cubo, consta de 6 caras
struct Cube {
    Face top;
    Face bottom;
    Face left;
    Face right;
    Face front;
    Face back;
};

GLuint _textureIdAlphaChannel;
GLuint _textureId;
Cube _cube;

bool isNubladisimo = false;

// Clase model:
// 
// Funcionamiento:
// los archivos .obj, si se abren con un editor de texto se puede ver que su informacion viene en el siguiente formato:
// # linea comentario                                           
// mtllib archivo.mtl                                           Indica el archivo donde se guarda la informacion sobre el material
// o nombreObjeto                                               Indica el nombre del objeto que se va a definir a continuacion
// v float float float                                          Indices de vertices
// vt float float                                               Indices de coordenadas de las texturas en los vertices
// vn float float float                                         Indices de las normales de los vertices
// usemtl nombreMaterial                                        Indica el nombre del material usado                                              
// f int/int/int int/int/int int/int/int ... int/int/int        Informacion sobre las caras
// 
// Con esta informacion, podemos siempre y cuando el objeto a renderizar tenga todas sus caras formando triangulos,
// , renderizar en pantalla la figura definida, ya que teniendo la lista de vertices con crear un GL_POLYGON se pueden
// definir todos los vertices con glVertex3fv(); introduciendo los datos de cada "v" del archivo ademas de poder definir 
// la textura con los vt y definir la normal con los vn.
// 
// Esta clase es una version modificada por nosotros para nuestro proyecto, para los archivos originales de esta clase: 
// https://github.com/WHKnightZ/OpenGL-Load-Model
//
class Model {
private:
    class Face {
    public:
        int edge;
        int* vertices;
        int* texcoords;
        int normal;

        Face(int edge, int* vertices, int* texcoords, int normal = -1) {
            this->edge = edge;
            this->vertices = vertices;
            this->texcoords = texcoords;
            this->normal = normal;
        }
    };
    std::vector<float*> vertices;
    std::vector<float*> texcoords;
    std::vector<float*> normals;
    std::vector<Face> faces;
    GLuint list;
public:
    void load(const char* filename) {
        std::string line;
        std::vector<std::string> lines;
        std::ifstream in(filename);
        if (!in.is_open()) {
            printf("Cannot load model %s\n", filename);
            return;
        }
        while (!in.eof()) {
            std::getline(in, line);
            lines.push_back(line);
        }
        in.close();
        float a, b, c;
        for (std::string& line : lines) {
            if (line[0] == 'v') {
                if (line[1] == ' ') {
                    sscanf_s(line.c_str(), "v %f %f %f", &a, &b, &c);
                    vertices.push_back(new float[3]{ a, b, c });
                }
                else if (line[1] == 't') {
                    sscanf_s(line.c_str(), "vt %f %f", &a, &b);
                    texcoords.push_back(new float[2]{ a, b });
                }
                else {
                    sscanf_s(line.c_str(), "vn %f %f %f", &a, &b, &c);
                    normals.push_back(new float[3]{ a, b, c });
                }
            }
            else if (line[0] == 'f') {
                int v0, v1, v2, t0, t1, t2, n;
                sscanf_s(line.c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d", &v0, &t0, &n, &v1, &t1, &n, &v2, &t2, &n);
                int* v = new int[3]{ v0 - 1, v1 - 1, v2 - 1 };
                faces.push_back(Face(3, v, NULL, n - 1));
            }
        }
        list = glGenLists(1);
        glNewList(list, GL_COMPILE);
        for (Face& face : faces) {
            if (face.normal != -1)
                glNormal3fv(normals[face.normal]);
            else
                glDisable(GL_LIGHTING);
            glBegin(GL_POLYGON);
            for (int i = 0; i < face.edge; i++)
                glVertex3fv(vertices[face.vertices[i]]);
            glEnd();
            if (face.normal == -1)
                glEnable(GL_LIGHTING);
        }
        glEndList();
        printf("Model: %s\n", filename);
        printf("Vertices: %d\n", vertices.size());
        printf("Texcoords: %d\n", texcoords.size());
        printf("Normals: %d\n", normals.size());
        printf("Faces: %d\n", faces.size());
        for (float* f : vertices)
            delete f;
        vertices.clear();
        for (float* f : texcoords)
            delete f;
        texcoords.clear();
        for (float* f : normals)
            delete f;
        normals.clear();
        faces.clear();
    }
    void draw() { glCallList(list); }
};


Model museo;
Model fuente;
Model arbol;

// Particula
struct Particle {
    Vec3f pos;
    Vec3f velocity;
    Vec3f color;
    float timeAlive; // Tiempo que la particula lleva existiendo
    float lifespan;  // Tiempo total de existencia permitido
};

// Nos permite rotar un vector undeterminado numero de grados en un determinado eje
Vec3f rotate(Vec3f v, Vec3f axis, float degrees) {
    axis = axis.normalize();
    float radians = degrees * PI / 180;
    float s = sin(radians);
    float c = cos(radians);
    return v * c + axis * axis.dot(v) * (1 - c) + v.cross(axis) * s;
}

// Nos devuelve la posicion de la particula, para poder dibujarlas de atras hacia adelante
// Esto lo hace teniendo en cuenta que rotaremos la camara 30º
Vec3f adjParticlePos(Vec3f pos) {
    return rotate(pos, Vec3f(1, 0, 0), -30);
}

// Nos dice si la particula1 esta detras de la particula2
bool compareParticles(Particle* particle1, Particle* particle2) {
    return adjParticlePos(particle1->pos)[2] <
        adjParticlePos(particle2->pos)[2];
}

float randomFloat() {
    return (float)rand() / ((float)RAND_MAX + 1);
}

// CLASE PARTICULA
class ParticleEngine {
private:
    // Almacena el ID de la textura con AlphaChannel, asi como todas las particulas existentes
    // Tambien posee la variable timeUntilnextStep que es el tiempo que se tarda en llamar a step() 
    GLuint textureId;
    Particle particles[NUM_PARTICLES];
    float timeUntilNextStep;
    // El color de las particulas lanzadas por la "fuente", 0 indica rojo y al llegar a 1 regresa a rojo de nuevo
    // siempre oscila de 0 a 1
    float colorTime;
    // Angulo en que se disparan las particulas, para hacer que gire podemos hacerlo en update como con el cubo
    float angle;

    // Devuelve el color actual de las particulas emitidas por la fuente
    Vec3f curColor() {
        // Aqui programamos que los colores oscilen entre 0 y 1, podemos ver que 1.0/3.0 equivale a verde
        // mientras que 2.0/3.0 equivale a azul, al llegar a 1.0 vale rojo de nuevo 
        Vec3f color;
        if (colorTime < 0.166667f) {
            color = Vec3f(1.0f, colorTime * 6, 0.0f);
        }
        else if (colorTime < 0.333333f) {
            color = Vec3f((0.333333f - colorTime) * 6, 1.0f, 0.0f);
        }
        else if (colorTime < 0.5f) {
            color = Vec3f(0.0f, 1.0f, (colorTime - 0.333333f) * 6);
        }
        else if (colorTime < 0.666667f) {
            color = Vec3f(0.0f, (0.666667f - colorTime) * 6, 1.0f);
        }
        else if (colorTime < 0.833333f) {
            color = Vec3f((colorTime - 0.666667f) * 6, 0.0f, 1.0f);
        }
        else {
            color = Vec3f(1.0f, 0.0f, (1.0f - colorTime) * 6);
        }

        // Nos aseguramos de que los colores oscilen entre 0 y 1
        for (int i = 0; i < 3; i++) {
            if (color[i] < 0) {
                color[i] = 0;
            }
            else if (color[i] > 1) {
                color[i] = 1;
            }
        }

        return color;
    }

    // Devuelve la velocidad de la particula, al final todas van a velocidad aleatoria, pero con este valor
    // logramos que no sea una aleatoriedad muy significativa
    Vec3f curVelocity() {
        return Vec3f(2 * cos(angle), 2.0f, 2 * sin(angle));
    }

    // Se crea una particula "muerta" y altera sus campos para hacerla funcional, lo primero que se hace es 
    // colocarla en el medio de la ventana y se le asigna una velocidad usando curVelocity + un vector random
    // y para el color usamos curColor, que nos devolvera muy probablemente un valor de 0
    void createParticle(Particle* p) {
        p->pos = Vec3f(0, 0, 0);
        p->velocity = curVelocity() + Vec3f(0.5f * randomFloat() - 0.25f,
            0.5f * randomFloat() - 0.25f,
            0.5f * randomFloat() - 0.25f);
        p->color = curColor();
        p->timeAlive = 0; // Acaba de nacer asi que su timeAlive es de 0
        p->lifespan = randomFloat() + 1; // Se le pone un tiempo de vida entre 1 y 2 segundos
    }

    // Realizamos un avance en funcion de la constante global STEP_TIME, el concepto de avance consiste en 
    // actualizar el valor del color de las particulas y el del angulo de lanzamiento de las mismas
    // por supuesto nos aseguramos de mantener estos valores en sus rangos permitidos
    void step() {
        colorTime += STEP_TIME / 10;
        while (colorTime >= 1) {
            colorTime -= 1;
        }

        angle += 0.5f * STEP_TIME;
        while (angle > 2 * PI) {
            angle -= 2 * PI; // 360ª
        }

        // Aqui es donde hacemos que estos efectos se apliquen en todas las particulas:
        for (int i = 0; i < NUM_PARTICLES; i++) {
            // En primer lugar usamos un puntero a particula, su valor sera la posicion de la misma + 1, asi sera 
            // sencillo localizarla
            Particle* p = particles + i;

            // Aqui es donde actualizamos estos valores, siempre en funcion de STEP_TIME
            // Actualizamos su posicion para que gire junto a la fuente manteniendo su velocidad
            p->pos += p->velocity * STEP_TIME;
            // Con esto simulamos el efecto de gravedad ya que hacemos decrecer la componente y en funcion
            // de la constante global GRAVITI
            p->velocity += Vec3f(0.0f, -GRAVITY * STEP_TIME, 0.0f);
            // Actualizamos su tiempo viva y comprobamos que no haya llegado a su maximo permitido
            // en ese caso, como estamos trabajando con un puntero p, sobreescribiremos la particula 
            // actual por una nueva al llamar a createParticle
            p->timeAlive += STEP_TIME;
            if (p->timeAlive > p->lifespan) {
                createParticle(p);
            }
        }
    }
public:
    // Constructor
    ParticleEngine(GLuint textureId1) {
        textureId = textureId1;
        timeUntilNextStep = 0;
        colorTime = 0;
        angle = 0;
        for (int i = 0; i < NUM_PARTICLES; i++) {
            createParticle(particles + i);
        }
        // Hacemos 5 steps para que no quede raro el inico ya que seria un poco extraño ver como se generan las
        // primeras particulas
        for (int i = 0; i < 5 / STEP_TIME; i++) {
            step();
        }
    }

    // Funcion auxiliar para llamar a step() el numero de veces indicadas
    void advance(float dt) {
        while (dt > 0) {
            if (timeUntilNextStep < dt) {
                dt -= timeUntilNextStep;
                step();
                timeUntilNextStep = STEP_TIME;
            }
            else {
                timeUntilNextStep -= dt;
                dt = 0;
            }
        }
    }

    // Dibuja la fuente de particulas
    void draw() {

        // Lo primero que hacemos es ordenarlas de mas atras a mas adelante
        std::vector<Particle*> ps;
        for (int i = 0; i < NUM_PARTICLES; i++) {
            ps.push_back(particles + i);
        }
        sort(ps.begin(), ps.end(), compareParticles);

        // Misma historia que siempre para establecer la textura a las particulas
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Si recuerdas, dijimos que las particulas son cuadrados paralelos a la camara
        glBegin(GL_QUADS);
        for (unsigned int i = 0; i < ps.size(); i++) {
            Particle* p = ps[i];
            glColor4f(p->color[0], p->color[1], p->color[2],
                (1 - p->timeAlive / p->lifespan));
            float size = PARTICLE_SIZE / 2;

            Vec3f pos = adjParticlePos(p->pos);

            glTexCoord2f(0, 0);
            glVertex3f(pos[0] - size, pos[1] - size, pos[2]);
            glTexCoord2f(0, 1);
            glVertex3f(pos[0] - size, pos[1] + size, pos[2]);
            glTexCoord2f(1, 1);
            glVertex3f(pos[0] + size, pos[1] + size, pos[2]);
            glTexCoord2f(1, 0);
            glVertex3f(pos[0] + size, pos[1] - size, pos[2]);
        }
        glEnd();
    }
};

ParticleEngine* _particleEngine;



// Como la anterior pero para caras del cubo
void rotate(Face& face, Vec3f axis, float degrees) {
    face.up = rotate(face.up, axis, degrees);
    face.right = rotate(face.right, axis, degrees);
    face.out = rotate(face.out, axis, degrees);
}

// Como la anterior pero para el cubo completo
void rotate(Cube& cube, Vec3f axis, float degrees) {
    rotate(cube.top, axis, degrees);
    rotate(cube.bottom, axis, degrees);
    rotate(cube.left, axis, degrees);
    rotate(cube.right, axis, degrees);
    rotate(cube.front, axis, degrees);
    rotate(cube.back, axis, degrees);
}

// Damos valores iniciales a las caras del cubo 
void initCube(Cube& cube) {
    cube.top.up = Vec3f(0, 0, -1);
    cube.top.right = Vec3f(1, 0, 0);
    cube.top.out = Vec3f(0, 1, 0);

    cube.bottom.up = Vec3f(0, 0, 1);
    cube.bottom.right = Vec3f(1, 0, 0);
    cube.bottom.out = Vec3f(0, -1, 0);

    cube.left.up = Vec3f(0, 0, -1);
    cube.left.right = Vec3f(0, 1, 0);
    cube.left.out = Vec3f(-1, 0, 0);

    cube.right.up = Vec3f(0, -1, 0);
    cube.right.right = Vec3f(0, 0, 1);
    cube.right.out = Vec3f(1, 0, 0);

    cube.front.up = Vec3f(0, 1, 0);
    cube.front.right = Vec3f(1, 0, 0);
    cube.front.out = Vec3f(0, 0, 1);

    cube.back.up = Vec3f(1, 0, 0);
    cube.back.right = Vec3f(0, 1, 0);
    cube.back.out = Vec3f(0, 0, -1);
}

// Funcion auxiliar que nos permite determinar si una cara se encuentra detras de otra
// Esto funciona porque el out de las caras se encuentra en el centro de las mismas, para otros poligonos no tiene
// por que ser asi, pero para cubos si
bool compareFaces(Face* face1, Face* face2) {
    return face1->out[2] < face2->out[2];
}

// esta funcion nos hace la vida mas facil ya que calcula los 4 vertices (esquinas) de una cara y las almacena en el
// vector vs
void faceVertices(Face& face, Vec3f* vs) {
    vs[0] = BOX_SIZE / 2 * (face.out - face.right - face.up);
    vs[1] = BOX_SIZE / 2 * (face.out - face.right + face.up);
    vs[2] = BOX_SIZE / 2 * (face.out + face.right + face.up);
    vs[3] = BOX_SIZE / 2 * (face.out + face.right - face.up);
}

void drawTopFace(Face& face, GLuint textureId) {
    Vec3f vs[4];
    faceVertices(face, vs);

    glDisable(GL_TEXTURE_2D);

    glPushMatrix();
    glBegin(GL_QUADS);


    glColor4f(0.0f, 0.0f, 1.0f, ALPHA);
    glNormal3f(face.out[0], face.out[1], face.out[2]);
    glVertex3f(vs[0][0], vs[0][1], vs[0][2]);
    glVertex3f(vs[1][0], vs[1][1], vs[1][2]);
    glVertex3f(vs[2][0], vs[2][1], vs[2][2]);
    glVertex3f(vs[3][0], vs[3][1], vs[3][2]);

    glEnd();
    glPopMatrix();
}

void drawBottomFace(Face& face, GLuint textureId) {
    Vec3f vs[4];
    faceVertices(face, vs);

    glDisable(GL_TEXTURE_2D);

    glPushMatrix();
    glBegin(GL_QUADS);

    glColor4f(0.0f, 0.0f, 1.0f, ALPHA - 0.2f);
    glNormal3f(face.out[0], face.out[1], face.out[2]);
    glVertex3f(vs[0][0], vs[0][1], vs[0][2]);
    glVertex3f(vs[1][0], vs[1][1], vs[1][2]);
    glVertex3f(vs[2][0], vs[2][1], vs[2][2]);
    glVertex3f(vs[3][0], vs[3][1], vs[3][2]);

    glEnd();
    glPopMatrix();
}

void drawLeftFace(Face& face, GLuint textureId) {
    Vec3f vs[4];
    faceVertices(face, vs);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPushMatrix();
    glColor4f(1.0f, 1.0f, 1.0f, ALPHA);

    glBegin(GL_QUADS);

    glNormal3f(face.out[0], face.out[1], face.out[2]);
    glTexCoord2f(0, 0);
    glVertex3f(vs[0][0], vs[0][1], vs[0][2]);
    glTexCoord2f(0, 1);
    glVertex3f(vs[1][0], vs[1][1], vs[1][2]);
    glTexCoord2f(1, 1);
    glVertex3f(vs[2][0], vs[2][1], vs[2][2]);
    glTexCoord2f(1, 0);
    glVertex3f(vs[3][0], vs[3][1], vs[3][2]);

    glEnd();
    glPopMatrix();
}

void drawRightFace(Face& face, GLuint textureId) {
    Vec3f vs[4];
    faceVertices(face, vs);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPushMatrix();
    glColor4f(1.0f, 1.0f, 1.0f, ALPHA);

    glBegin(GL_QUADS);

    glNormal3f(face.out[0], face.out[1], face.out[2]);
    glTexCoord2f(0, 0);
    glVertex3f(vs[0][0], vs[0][1], vs[0][2]);
    glTexCoord2f(0, 1);
    glVertex3f(vs[1][0], vs[1][1], vs[1][2]);
    glTexCoord2f(1, 1);
    glVertex3f(vs[2][0], vs[2][1], vs[2][2]);
    glTexCoord2f(1, 0);
    glVertex3f(vs[3][0], vs[3][1], vs[3][2]);

    glEnd();
    glPopMatrix();
}

void drawFrontFace(Face& face, GLuint textureId) {
    Vec3f vs[4];
    faceVertices(face, vs);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPushMatrix();
    glColor4f(1.0f, 1.0f, 1.0f, ALPHA);

    glBegin(GL_QUADS);

    glNormal3f(face.out[0], face.out[1], face.out[2]);
    glTexCoord2f(0, 0);
    glVertex3f(vs[0][0], vs[0][1], vs[0][2]);
    glTexCoord2f(0, 1);
    glVertex3f(vs[1][0], vs[1][1], vs[1][2]);
    glTexCoord2f(1, 1);
    glVertex3f(vs[2][0], vs[2][1], vs[2][2]);
    glTexCoord2f(1, 0);
    glVertex3f(vs[3][0], vs[3][1], vs[3][2]);

    glEnd();
    glPopMatrix();
}


void drawBackFace(Face& face, GLuint textureId) {
    Vec3f vs[4];
    faceVertices(face, vs);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPushMatrix();
    glColor4f(1.0f, 1.0f, 1.0f, ALPHA);

    glBegin(GL_QUADS);

    glNormal3f(face.out[0], face.out[1], face.out[2]);
    glTexCoord2f(0, 0);
    glVertex3f(vs[0][0], vs[0][1], vs[0][2]);
    glTexCoord2f(0, 1);
    glVertex3f(vs[1][0], vs[1][1], vs[1][2]);
    glTexCoord2f(1, 1);
    glVertex3f(vs[2][0], vs[2][1], vs[2][2]);
    glTexCoord2f(1, 0);
    glVertex3f(vs[3][0], vs[3][1], vs[3][2]);

    glEnd();
    glPopMatrix();
}

//Draws the indicated face on the specified cube.
void drawFace(Face* face, Cube& cube, GLuint textureId) {

    // Aqui dibujamos las cara pasada como argumento en su respectiva funcion los diferentes ifs ayudan a 
    // determinar que cara es cual y asi aplicar el codigo respectivo a cada una de ellas
    if (face == &(cube.top)) {
        drawTopFace(cube.top, textureId);
    }
    else if (face == &(cube.bottom)) {
        drawBottomFace(cube.bottom, textureId);
    }
    else if (face == &(cube.left)) {
        drawLeftFace(cube.left, textureId);
    }
    else if (face == &(cube.right)) {
        drawRightFace(cube.right, textureId);
    }
    else if (face == &(cube.front)) {
        drawFrontFace(cube.front, textureId);
    }
    else {
        drawBackFace(cube.back, textureId);
    }
}

// Funcion de SOIL2
// Convierte la Image en textura y devuelve su ID
GLuint loadTexture(const char* path) {
    GLuint textureId = SOIL_load_OGL_texture
    (
        path,
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_INVERT_Y

    );
    return textureId;
}

// Esta funcion recorre nuestra imagen que cargamos previamente y crea un nuevo vector llamado pixels el cual
// contendra los mismos valores que image en su RGB, ademas de esto, añadimos un 4o parametro a pixels:
// el valor R del AlphaChannel, realmente el AlphaChannel esta en B&N asi que da igual si usamos su R, G o B
char* addAlphaChannel(Image* image, Image* alphaChannel) {
    char* pixels = new char[image->width * image->height * 4];
    for (int y = 0; y < image->height; y++) {
        for (int x = 0; x < image->width; x++) {
            for (int j = 0; j < 3; j++) {
                pixels[4 * (y * image->width + x) + j] =
                    image->pixels[3 * (y * image->width + x) + j];
            }
            pixels[4 * (y * image->width + x) + 3] =
                alphaChannel->pixels[3 * (y * image->width + x)];
        }
    }

    return pixels;
}

// Exactamente igual que loadTexture pero adaptada a AlphaChannels, como se puede observar, es igual
// solo que en el flag de GL_RGB usamos GL_RGBA siendo la A la especificacion de que tenemos Alpha en la textura
GLuint loadAlphaTexture(Image* image, Image* alphaChannel) {
    char* pixels = addAlphaChannel(image, alphaChannel);

    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D,
        0,
        GL_RGBA,
        image->width, image->height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels);

    delete pixels;
    return textureId;
}

// Init
void initRendering()
{
    glEnable(GL_DEPTH_TEST); // El depth_test comprueba que objetos estan delante los unos de los otros
    glEnable(GL_COLOR_MATERIAL); // Permite renderizar colores en los objetos
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f); // Cambia el color del fondo

    glEnable(GL_LIGHTING); // Activa la iluminacion de la escena, se podria desactivar con: glDisable(GL_LIGHTING)
    glEnable(GL_LIGHT0); // crea una fuente de luz, se puede hasta 8 fuentes con este metodo
    glEnable(GL_LIGHT1); // se podrian desactivar con glDisable(GL_LIGHT0)
    glEnable(GL_NORMALIZE);

    // Alpha Blending Cube
    glEnable(GL_BLEND); // Para permitir Alpha
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Tipo de Alpha (1- Alpha)
    
    glEnable(GL_FOG); // Para permitir el uso de niebla

    // Cosas para la primera persona en init():
    glutWarpPointer(width / 2, height / 2); // bloquea el cursor del mouse en medio de la pantalla
    glutSetCursor(GLUT_CURSOR_NONE); // Oculta el cursor

    //Particulas
    // Cargamos ambos bitmap en nuestras variables Image
    Image* image = loadBMP("circle.bmp");
    Image* alphaChannel = loadBMP("circlealpha.bmp");

    // Creamos la textura con Alpha Channel
    _textureIdAlphaChannel = loadAlphaTexture(image, alphaChannel);
    delete image;
    delete alphaChannel;

    // AlphaBlending
    _textureId = loadTexture("TexturaVidriera1.jpeg");

    // Cargar modelos
    museo.load("Models/museo.obj");
    fuente.load("Models/fuente.obj");
    arbol.load("Models/pino.obj");

}

// Se llama cuando se cambia el tamanno de la ventana
void handleResize(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION); // Cambia la camara a perspectiva
    glLoadIdentity(); // Reset de la camara (perspectiva)

    gluPerspective(45.0,        // Angulo de la camara (fov)
        (double)w / (double)h,  // ratio ancho-altura
        1.0,                    // Cliping cerca de la camara eje z
        200.0);                 // Cliping lejos de la camara eje z
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // limpia el ultimo dibujado

    glMatrixMode(GL_MODELVIEW); // Cambia la camara de perspectiva
    glLoadIdentity();

    // Antes de dibujar necesitamos actualizar la posicion de la camara:
    camera();

    
    drawScene(); // ahora mismo solo la uso para la iluminacion
    glDisable(GL_TEXTURE_2D); // para volver a renderizar solor colores.
    drawModels();

    // Particulas
    glPushMatrix();
    glTranslatef(12.0f, -2.0f, 40.0f);
    glScalef(4.0f, 4.0f, 4.0f);

    _particleEngine->draw();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-12.0f, -2.0f, 40.0f);
    glScalef(4.0f, 4.0f, 4.0f);

    _particleEngine->draw();
    glPopMatrix();

    glutSwapBuffers();
}

float alphaAngle = 0;
// Dibuja al escena
void drawScene()
{

    // nota: glTranslatef, glRotatef ,glScalef, glLightfv no se pueden llamar en un bloque glBegin-glEnd
    //       pero glColor3f si

    
    // Niebla
    if (isNubladisimo) {

        GLfloat fogColor[] = { 0.5f, 0.5f, 0.5f, 1 };
        glFogfv(GL_FOG_COLOR, fogColor);
        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogf(GL_FOG_START, 10.0f);
        glFogf(GL_FOG_END, 20.0f);
    }
    else {

        GLfloat fogColor[] = { 0.5f, 0.5f, 0.5f, 1 };
        glFogfv(GL_FOG_COLOR, fogColor);
        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogf(GL_FOG_START, 10.0f);
        glFogf(GL_FOG_END, 200.0f);
    }
    
    glTranslatef(0.0f, 0.0f, -70.0f); // DESPLAZAMIENTO GLOBAL
    // Annadir luz de ambiente
    GLfloat ambientColor[] = { 0.2f, 0.2f, 0.2f, 1.0f }; // Color (0.2, 0.2, 0.2) el ultimo valor (1.0f) es la ganancia
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientColor);

    // Annadir los puntos de luz
    GLfloat lightColor0[] = { 0.5f, 0.5f, 0.5f, 1.0f }; // Color (0.5, 0.5, 0.5)
    GLfloat lightPos0[] = { 4.0f, 0.0f, 8.0f, 1.0f }; // Posicion (4, 0, 8)
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor0);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);

    GLfloat lightColor1[] = { 0.5f, 0.2f, 0.2f, 1.0f };
    GLfloat lightPos1[] = { -1.0f, 0.5f, 0.5f, 0.0f }; // ese 0 del final hace que sea una luz direccional
    glLightfv(GL_LIGHT1, GL_DIFFUSE, lightColor1);
    glLightfv(GL_LIGHT1, GL_POSITION, lightPos1);

    
    // Alpha Blending Cube
    // Añadimos todas las caras en el orden que sea
    std::vector<Face*> faces;
    faces.push_back(&(_cube.top));
    faces.push_back(&(_cube.bottom));
    faces.push_back(&(_cube.left));
    faces.push_back(&(_cube.right));
    faces.push_back(&(_cube.front));
    faces.push_back(&(_cube.back));

    //Aqui las ordenamos de mas atras a mas adelante
    sort(faces.begin(), faces.end(), compareFaces);

    alphaAngle += 1;
    if (alphaAngle >= 360) {
        alphaAngle = 0;
    }

    glPushMatrix();
    glTranslatef(10.0f, 0.0f, -10.0f);
    glRotatef(alphaAngle, 1.0f, 1.0f, 0.0f);
    glScalef(0.75f, 0.75f, 0.75f);
    // Y ahora, una vez ordenadas las dibujamos
    for (unsigned int i = 0; i < faces.size(); i++) {
        drawFace(faces[i], _cube, _textureId);
    }
    glPopMatrix();
}

void drawModels()
{
    // Suelo:
    glPushMatrix();
    glTranslatef(0.0f, -5.0f, 0.0f);
    glRotatef(0.0f, 0.0f, 0.0f, 0.0f);
    glScalef(1000.0f, 1000.0f, 1000.0f);
    glColor3f(0.45f, 0.87f, 0.44f);

    glBegin(GL_QUADS);
    /*
    * GL_QUAD vertices order is as follows :
    * top left
    * bottom left
    * top right
    * bottom right
    */

    glVertex3f(-1.0f, 0.0f,-1.0f);
    glVertex3f(-1.0f, 0.0f, 1.0f);
    glVertex3f( 1.0f, 0.0f, 1.0f);
    glVertex3f( 1.0f, 0.0f,-1.0f);

    glEnd();
    glPopMatrix();

    // Museo:
    glPushMatrix();
    glTranslatef(0.0f, -5.0f, 0.0f);
    glRotatef(0.0f, 0.0f, 0.0f, 0.0f);
    glScalef(4.0f, 4.0f, 4.0f);
    glColor3f(2.35f/2, 2.25f/2, 2.01f/2);


    museo.draw();
    glPopMatrix();
    // Fuentes:
    glPushMatrix();
    glTranslatef(-12.0f, -5.0f, 40.0f);
    //glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
    glScalef(2.0f, 2.0f, 2.0f);
    glColor3f(2.35f / 2, 2.25f / 2, 2.01f / 2);

    fuente.draw();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(12.0f, -5.0f, 40.0f);
    //glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
    glScalef(-2.0f, 2.0f, -2.0f);
    glColor3f(2.35f / 2, 2.25f / 2, 2.01f / 2);

    fuente.draw();
    glPopMatrix();

    // Arboles:
    glPushMatrix();
    glTranslatef(-15.0f, -5.0f, -35.0f);
    glScalef(4.0f, 4.0f, 4.0f);
    glColor3f(0.0f, 0.5f, 0.0f);

    arbol.draw(); // arbol cercano
    glPopMatrix();

    srand(7);
    for (int i = 0; i < 33; i++) {
        glPushMatrix();
        glTranslatef(-150.0f + i * 10, -5.0f, -180.0f + rand() % 30);
        glScalef(8.0f, 8.0f, 8.0f);
        glColor3f(0.0f, 1.0f, 0.0f);

        arbol.draw();
        glPopMatrix();
    }
    
    
}



// Update
void update(int value) {
    // Particulas
    _particleEngine->advance(25.0f / 1000.0f);

    glutPostRedisplay();
    glutTimerFunc(1000 / 200, update, 0);

    // Para primera persona en timer:
    glutWarpPointer(width / 2, height / 2); // Fija el mouse en medio de la pantalla
}


/* ----------------- Funciones camara priemra persona -----------------------*/
void passive_motion(int x, int y) // Esta funcion es para tomar los pequeños cambios d posicion del raton para mover la camara
{
    float sensibilidadRaton = 15; // la variacion es muy grande y necesita ser reducida para que funcione correctamente, sensibilidad
    // Usamos estas dos variables para guardar las nuevas coordenadas x e y observadas desde el centro de la ventana
    int dev_x, dev_y;
    dev_x = (width / 2) - x;
    dev_y = (height / 2) - y;

    // se aplican los cambios en las variables globales definidas antes
    yaw += (float)dev_x / sensibilidadRaton;
    pitch += (float)dev_y / sensibilidadRaton;
}

void camera() // funcion que calcula la nueva rotacion de la camara
{
    // calculos necesarios para la posicion relativa de la camara
    if (motion.Forward)
    {
        camX += cos((yaw + 90) * TO_RADIANS) / 5.0;
        camZ -= sin((yaw + 90) * TO_RADIANS) / 5.0;
    }
    if (motion.Backward)
    {
        camX += cos((yaw + 90 + 180) * TO_RADIANS) / 5.0;
        camZ -= sin((yaw + 90 + 180) * TO_RADIANS) / 5.0;
    }
    if (motion.Left)
    {
        camX += cos((yaw + 90 + 90) * TO_RADIANS) / 5.0;
        camZ -= sin((yaw + 90 + 90) * TO_RADIANS) / 5.0;
    }
    if (motion.Right)
    {
        camX += cos((yaw + 90 - 90) * TO_RADIANS) / 5.0;
        camZ -= sin((yaw + 90 - 90) * TO_RADIANS) / 5.0;
    }

    // Limitamos el giro en el eje x para que no se pueda rotar sobre ella misma
    if (pitch >= 70)
        pitch = 70;
    if (pitch <= -60)
        pitch = -60;

    // Es muy importante rotar y luego mover al personaje sino la rotacion sera en la posicion incorrecta
    // Es importante rotar el eje x antes que el y o no funcionara correctamente
    glRotatef(-pitch, 1.0, 0.0, 0.0); // rotacion sobre el eje x
    glRotatef(-yaw, 0.0, 1.0, 0.0);   // rotacion sobre el eje y

    glTranslatef(-camX, 0.0, -camZ);  // Movimiento de la camara
}


void keyboard(unsigned char key, int x, int y) // funcion que detecta cuando una tecla ha sido pulsada
{
    switch (key) // key almacena el valor ASCII de la clave seleccionada por ejemplo el de esc es el 27
    {
    case 'W':
    case 'w':
        motion.Forward = true;
        break;
    case 'A':
    case 'a':
        motion.Left = true;
        break;
    case 'S':
    case 's':
        motion.Backward = true;
        break;
    case 'D':
    case 'd':
        motion.Right = true;
        break;
    case 'N': // Cambio de niebla
    case 'n':
        if (isNubladisimo) {
            isNubladisimo = false;
        }
        else {
            isNubladisimo = true;
        }
        break;
    case 'F':
    case 'f':
        glutFullScreenToggle();
        break;
    case 27: // Escape
        exit(0);
        break;
    }
}

void keyboard_up(unsigned char key, int x, int y) // funcion que detecta cuando una tecla ha dejado de ser pulsada
{
    switch (key)
    {
    case 'W':
    case 'w':
        motion.Forward = false;
        break;
    case 'A':
    case 'a':
        motion.Left = false;
        break;
    case 'S':
    case 's':
        motion.Backward = false;
        break;
    case 'D':
    case 'd':
        motion.Right = false;
        break;
    }
}

int main(int argc, char** argv) {
    // Inicializar glut
    glutInit(&argc, argv);
    glutSetOption(GLUT_MULTISAMPLE, 8);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(width, height); // Tamanno de la ventana

    // Crear la ventana
    glutCreateWindow("A museum with a secret inside...");
    initRendering(); // Inicializar el renderizado

    // Alpha Blending Cube
    initCube(_cube);

    // Definir los handler para las funciones de entrada por teclado, cambio de tamanno de la ventana, dibujado de la escena y update
    glutDisplayFunc(display);
    glutReshapeFunc(handleResize);

    // Particulas
    _particleEngine = new ParticleEngine(_textureIdAlphaChannel);

    glutTimerFunc(0, update, 0);

    // Handler de funciones para la camara en primera persona:
    glutPassiveMotionFunc(passive_motion);  // con esta funcion vamos a hacer que sin tener que pulsar ninguna tecla pille la posicion
                                            // del mouse (los pequeños cambios que hay entre cada update del timer).
    glutKeyboardFunc(keyboard);      // funcion para que al pulsar un tecla se active una accion
    glutKeyboardUpFunc(keyboard_up); // funcion para que al dejar de pulsar una tecla se active una accion
                                     // Necesitamos usar estas dos funciones para hacer que el movimiento continue hasta soltar la tecla


    glutMainLoop(); // Empezar el bucle de renderizado
    return 0;
}