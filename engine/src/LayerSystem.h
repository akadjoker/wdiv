#pragma once

#include "raylib.h"
#include "Graph.h"
#include <vector>
#include <cmath>

// ===== CONSTANTES =====
const int MAX_LAYERS = 6;
const int GUI_LAYER = 5;  // Última layer é sempre GUI (sem câmara)

// ===== ESTRUTURAS =====

struct Camera2D_Custom
{
    Vec2 position;      // Posição da câmara
    Vec2 target;        // Alvo da câmara (geralmente player)
    float rotation;     // Rotação
    float zoom;         // Zoom (1.0 = normal)
    
    Camera2D_Custom() : position(0, 0), target(0, 0), rotation(0), zoom(1.0f) {}
};

struct TileBackground
{
    Graph* tileGraph;       // Gráfico do tile
    float scrollX;          // Posição X do scroll
    float scrollY;          // Posição Y do scroll
    int tileWidth;          // Largura de cada tile
    int tileHeight;         // Altura de cada tile
    bool active;            // Se está ativo
    
    TileBackground() : tileGraph(nullptr), scrollX(0), scrollY(0), 
                       tileWidth(32), tileHeight(32), active(false) {}
};

struct Layer
{
    // Identidade
    int index;                  // 0-5
    bool active;                // Se está visível
    
    // Transformação
    float offsetX, offsetY;      // Offset local da layer
    float scrollFactorX;        // Paralax X (0.0 = imóvel, 1.0 = move com câmara)
    float scrollFactorY;        // Paralax Y
    
    // Background tiling
    TileBackground background;
    
    // Gráficos
    std::vector<Graph*> graphs;      // Gráficos nesta layer
    
    // Câmara (só usada se não for GUI layer)
    Camera2D_Custom camera;
    bool useLocalCamera;        // Se usa câmara própria ou da main
    
    Layer(int idx) : index(idx), active(true), 
                     offsetX(0), offsetY(0),
                     scrollFactorX(1.0f), scrollFactorY(1.0f),
                     useLocalCamera(false) {}
};

// ===== LAYER SYSTEM =====

class LayerSystem
{
private:
    Layer* layers[MAX_LAYERS];
    Camera2D_Custom mainCamera;
    
    int screenWidth;
    int screenHeight;
    
    // Utilitários
    void drawTiledBackground(const Layer* layer, const Camera2D_Custom& camera);
    void applyLayerTransform(const Layer* layer, const Camera2D_Custom& camera);
    void resetLayerTransform();
    
public:
    LayerSystem(int width, int height);
    ~LayerSystem();
    
    // Criação e gestão
    Layer* getLayer(int index);
    void setLayerActive(int index, bool active);
    bool isLayerActive(int index) const;
    
    // Câmara principal
    void setMainCamera(Vec2 position, float zoom = 1.0f);
    void updateMainCamera(Vec2 newPosition);
    Vec2 getMainCameraPosition() const { return mainCamera.position; }
    float getMainCameraZoom() const { return mainCamera.zoom; }
    
    // Gráficos
    void addGraphToLayer(int layerIndex, Graph* graph);
    void removeGraphFromLayer(int layerIndex, Graph* graph);
    
    // Background Tiling
    void setLayerBackground(int layerIndex, Graph* tileGraph, int tileW, int tileH);
    void removeLayerBackground(int layerIndex);
    void updateBackgroundScroll(int layerIndex, float scrollX, float scrollY);
    
    // Paralax
    void setLayerScrollFactor(int layerIndex, float factorX, float factorY);
    void setLayerOffset(int layerIndex, float offsetX, float offsetY);
    
    // Render
    void render();
    void renderLayer(int layerIndex);
    
    // Utilitários
    Vec2 screenToWorldPos(Vec2 screenPos, int layerIndex);
    Vec2 worldToScreenPos(Vec2 worldPos, int layerIndex);
};

// ===== UTILITÁRIOS LAYER =====

namespace LayerMath
{
    // Calcular posição do tile no tiling
    inline void getTileOffset(float scrollX, float scrollY, 
                              int tileW, int tileH,
                              float& outX, float& outY)
    {
        outX = fmod(scrollX, (float)tileW);
        outY = fmod(scrollY, (float)tileH);
        
        if (outX > 0) outX -= tileW;
        if (outY > 0) outY -= tileH;
    }
    
    // Calcular quantos tiles caber na tela
    inline int getTileCountX(int screenW, int tileW)
    {
        return (screenW / tileW) + 2;  // +2 para segurança
    }
    
    inline int getTileCountY(int screenH, int tileH)
    {
        return (screenH / tileH) + 2;  // +2 para segurança
    }
}
