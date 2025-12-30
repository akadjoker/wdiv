#include "Graph.h"
#include <cmath>

namespace GraphMath
{

    void advance(float &x, float &y, float angle, float distance)
    {
        // Converte ângulo de centimilésimos para radianos
        float rad = angle * M_PI / -180000.0f;
        x += cosf(rad) * distance;
        y -= sinf(rad) * distance; // note o "-"
    }

    int getAngle(float x1, float y1, float x2, float y2)
    {
        float dx = x2 - x1;
        float dy = y2 - y1;

        if (dx == 0)
            return (dy > 0) ? 270000 : 90000;

        int angle = (int)(atan(dy / dx) * 180000.0 / M_PI);
        return (dx > 0) ? -angle : -angle + 180000;
    }

    void rotatePoint(float &px, float &py, float angle)
    {
        if (angle == 0)
            return;

        float rad = angle * M_PI / -180000.0f;
        float cos_a = cosf(rad);
        float sin_a = sinf(rad);

        float rx = px * cos_a - py * sin_a;
        float ry = px * sin_a + py * cos_a;
        px = rx;
        py = ry;
    }

    void applyMirror(float &px, float &py, float centerX, float centerY,
                     bool hMirror, bool vMirror)
    {
        if (hMirror)
        {
            px = centerX - px;
        }
        if (vMirror)
        {
            py = centerY - py;
        }
    }

    void scalePoint(float &px, float &py, float scaleX, float scaleY)
    {
        px *= (scaleX / 100.0f);
        py *= (scaleY / 100.0f);
    }

    void applyResolution(float &x, float &y, int resolution)
    {
        if (resolution > 0)
        {
            x *= resolution;
            y *= resolution;
        }
        else if (resolution < 0)
        {
            x /= -resolution;
            y /= -resolution;
        }
    }

} // namespace GraphMath

Graph::Graph()
    : x(0), y(0), angle(0), sizeX(100), sizeY(100),
      resolution(0), flags(0), parent(nullptr), texWidth(32), texHeight(32)
{
    sourceRect = Rect(0, 0, 32, 32);
    centerPoint = Vec2(16, 16);
    texture.id = 0;
}

Graph::~Graph()
{
    if (texture.id > 0)
        UnloadTexture(texture);
}

void Graph::loadFile(const char *name)
{

    texture = LoadTexture(name);
    texWidth = texture.width;
    texHeight = texture.height;
    sourceRect = Rect(0, 0, texture.width, texture.height);

    // Center point por default é o centro da imagem
    centerPoint = Vec2(texture.width / 2.0f, texture.height / 2.0f);

    // Ponto 0 é sempre o centro
    localPoints.clear();
    localPoints.push(centerPoint);
}

void Graph::loadAtlas(Rect clip)
{
    sourceRect = clip;
    texWidth = clip.width;
    texHeight = clip.height;

    centerPoint = Vec2(clip.width / 2.0f, clip.height / 2.0f);

    localPoints.clear();
    localPoints.push(centerPoint);
}

void Graph::addPoint(Vec2 p)
{
    localPoints.push(p);
}

Vec2 Graph::getPoint(int idx)
{
    if (idx < 0 || idx >= (int)localPoints.size())
    {
        return Vec2(0, 0);
    }
    return localPoints[idx];
}

Vec2 Graph::getRealPoint(int idx)
{
    if (idx < 0 || idx >= (int)localPoints.size())
        return Vec2(0, 0);
    
    Vec2 p = localPoints[idx];
    float px = p.x - centerPoint.x;
    float py = p.y - centerPoint.y;
    
    GraphMath::applyMirror(px, py, centerPoint.x, centerPoint.y,
        flags & H_MIRROR, flags & V_MIRROR);
    
    GraphMath::scalePoint(px, py, sizeX, sizeY);
    GraphMath::rotatePoint(px, py, -angle);
    
    float rx = x + px;
    float ry = y + py;
    
    if (parent)
    {
        GraphMath::rotatePoint(rx, ry, -parent->angle);
        rx += parent->x;
        ry += parent->y;
    }
    
    GraphMath::applyResolution(rx, ry, resolution);
    
  
    return Vec2((float)rx, (float)ry);
}

void Graph::setTransform(float px, float py, float pangle, float psizeX, float psizeY)
{
    x = px;
    y = py;
    angle = pangle;
    sizeX = psizeX;
    sizeY = psizeY;
}

void Graph::setPosition(float px, float py)
{
    x = px;
    y = py;
}

void Graph::setRotation(float pangle)
{
    angle = pangle;
}

void Graph::setScale(float sx, float sy)
{
    sizeX = sx;
    sizeY = sy;
}

void Graph::render()
{

    // Pega o ponto 0 (centro)
    Vec2 center = getRealPoint(0);

    int pixelX = (int)roundf(center.x);
    int pixelY = (int)roundf(center.y);

    // Calcula dimensões finais com escala
    float finalWidth = sourceRect.width * sizeX / 100.0f;
    float finalHeight = sourceRect.height * sizeY / 100.0f;

    if (texture.id <= 0)
    {
        DrawRectangleLines((int)center.x, (int)center.y, (int)finalWidth, (int)finalHeight, RED);
        return;
    }

    float angleInDegrees = angle / 1000.0f;
    if (parent)
    {
        angleInDegrees += parent->angle / 1000.0f; // ADICIONA o ângulo do parent!
    }

    // Desenha a textura
    DrawTexturePro(
        texture,
        {(float)sourceRect.x, (float)sourceRect.y, (float)sourceRect.width, (float)sourceRect.height},
        {(float)pixelX, (float)pixelY, (float)finalWidth, (float)finalHeight},
        {(float)finalWidth / 2.0f, (float)finalHeight / 2.0f},
        angleInDegrees,
        WHITE);

    // Debug: desenha os pontos de attachement
    for (int i = 0; i < (int)localPoints.size(); i++)
    {
        Vec2 p = getRealPoint(i);
        int px = (int)roundf(p.x);
        int py = (int)roundf(p.y);
        DrawCircle(px, py, 4, i == 0 ? YELLOW : RED);
    }
}

void Graph::setParent(Graph *p)
{
    parent = p;
}

GraphManager::GraphManager()
{
}

GraphManager::~GraphManager()
{
    cleanup();
}

Graph *GraphManager::create()
{
    Graph *g = new Graph();
    graphs.push(g);
    return g;
}

void GraphManager::cleanup()
{
    for (size_t i = 0; i < graphs.size(); i++)
    {
        Graph *g = graphs[i];

        delete g;
    }
    graphs.clear();
}