#pragma once

#include <cmath>
#include <stdio.h>
#include <raylib.h>
#include "config.hpp"
#include "vector.hpp"
#include "Physics2D.h" 

 
struct Rect
{
    float x, y, width, height;
    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(float x, float y, float w, float h) : x(x), y(y), width(w), height(h) {}
};

// Flags
enum GraphFlags
{
    H_MIRROR = 1 << 0,
    V_MIRROR = 1 << 1,
};

struct Graph
{
    Rect sourceRect; // clip da "textura" (x, y, w, h)

    // Transformações
    float x, y;         // posição
    float angle;        // em centimilésimos (90000 = 90 graus)
    float sizeX, sizeY; // escala em percentagem (100 = 1x)
    int resolution;     // positivo multiplica, negativo divide

    Texture2D texture;
    // Pontos locais
    Vector<Vec2> localPoints;
    Vec2 centerPoint; // centro da imagem

    // Flags
    int flags;

    Graph *parent;

    // Dimensões da "textura" (simulado)
    float texWidth, texHeight;

    Graph();
    ~Graph();

    void loadFile(const char *name);
    void loadAtlas(Rect clip);

    void addPoint(Vec2 p);
    Vec2 getPoint(int idx);
    Vec2 getRealPoint(int idx);

    void setTransform(float x, float y, float angle = 0, float sizeX = 100, float sizeY = 100);
    void setPosition(float x, float y);
    void setRotation(float angle);
    void setScale(float sx, float sy);

    void render();

    void setParent(Graph *p);
};

class GraphManager
{
private:
    Vector<Graph *> graphs;

public:
    GraphManager();
    ~GraphManager();

    Graph *create();
    void cleanup();
};

namespace GraphMath
{

    // Advance - move baseado em ângulo e distância
    // angle em centimilésimos (90000 = 90 graus)
    void advance(float &x, float &y, float angle, float distance);

    // Ângulo entre dois pontos (retorna centimilésimos)
    int getAngle(float x1, float y1, float x2, float y2);

    // Rotação de ponto
    void rotatePoint(float &px, float &py, float angle);

    // Mirror handling
    void applyMirror(float &px, float &py, float centerX, float centerY,
                     bool hMirror, bool vMirror);

    // Scale point
    void scalePoint(float &px, float &py, float scaleX, float scaleY);

    // Apply resolution scaling
    void applyResolution(float &x, float &y, int resolution);

}
