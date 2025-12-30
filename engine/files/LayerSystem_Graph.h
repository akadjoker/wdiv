#pragma once

#include "Graph.h"
#include "raylib.h"
#include <vector>

// ===== CONSTANTES =====
const int MAX_LAYERS = 6;
const int GUI_LAYER = 5;  // Última layer é sempre GUI (sem câmara)

// ===== ESTRUTURAS =====

struct LayerCamera
{
    float x, y;           // Posição da câmara
    float zoom;           // Zoom (1.0 = normal)
    
    LayerCamera() : x(0), y(0), zoom(1.0f) {}
};

struct TileLayer
{
    Graph* tileGraph;     // Gráfico do tile
    float scrollX;        // Posição X do scroll
    float scrollY;        // Posição Y do scroll
    bool active;          // Se está ativo
    
    TileLayer() : tileGraph(nullptr), scrollX(0), scrollY(0), active(false) {}
};

struct Layer
{
    int index;                    // 0-5
    bool active;                  // Se está visível
    
    // Transformação
    float offsetX, offsetY;        // Offset local da layer
    float scrollFactorX;          // Paralax X (0.0 = imóvel, 1.0 = move com câmara)
    float scrollFactorY;          // Paralax Y
    
    // Background tiling
    TileLayer background;
    
    // Gráficos
    std::vector<Graph*> graphs;   // Gráficos nesta layer
    
    // Câmara
    LayerCamera camera;
    bool useLocalCamera;          // Se usa câmara própria ou da main
    
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
    LayerCamera mainCamera;
    
    int screenWidth;
    int screenHeight;
    
    // Utilitários
    void drawTiledBackground(const Layer* layer, const LayerCamera& camera);
    
public:
    LayerSystem(int width, int height);
    ~LayerSystem();
    
    // Criação e gestão
    Layer* getLayer(int index);
    void setLayerActive(int index, bool active);
    bool isLayerActive(int index) const;
    
    // Câmara principal
    void setMainCamera(float x, float y, float zoom = 1.0f);
    void updateMainCamera(float x, float y);
    void getMainCameraPosition(float& x, float& y) const;
    float getMainCameraZoom() const { return mainCamera.zoom; }
    
    // Gráficos
    void addGraphToLayer(int layerIndex, Graph* graph);
    void removeGraphFromLayer(int layerIndex, Graph* graph);
    
    // Background Tiling
    void setLayerBackground(int layerIndex, Graph* tileGraph);
    void removeLayerBackground(int layerIndex);
    void updateBackgroundScroll(int layerIndex, float scrollX, float scrollY);
    
    // Paralax
    void setLayerScrollFactor(int layerIndex, float factorX, float factorY);
    void setLayerOffset(int layerIndex, float offsetX, float offsetY);
    
    // Render
    void render();
    void renderLayer(int layerIndex);
    
    // Utilitários
    void screenToWorldPos(float screenX, float screenY, int layerIndex, float& worldX, float& worldY);
    void worldToScreenPos(float worldX, float worldY, int layerIndex, float& screenX, float& screenY);
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
        return (screenW / tileW) + 3;
    }
    
    inline int getTileCountY(int screenH, int tileH)
    {
        return (screenH / tileH) + 3;
    }
}
