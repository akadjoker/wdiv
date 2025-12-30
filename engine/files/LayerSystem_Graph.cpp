#include "LayerSystem_Graph.h"
#include <algorithm>
#include <cmath>

LayerSystem::LayerSystem(int width, int height) 
    : screenWidth(width), screenHeight(height)
{
    // Criar layers
    for (int i = 0; i < MAX_LAYERS; i++)
    {
        layers[i] = new Layer(i);
        
        // Layer 5 (GUI) nunca usa câmara
        if (i == GUI_LAYER)
        {
            layers[i]->useLocalCamera = false;
            layers[i]->scrollFactorX = 0.0f;
            layers[i]->scrollFactorY = 0.0f;
        }
    }
    
    // Câmara principal
    mainCamera.x = width / 2.0f;
    mainCamera.y = height / 2.0f;
    mainCamera.zoom = 1.0f;
}

LayerSystem::~LayerSystem()
{
    for (int i = 0; i < MAX_LAYERS; i++)
    {
        if (layers[i])
        {
            delete layers[i];
        }
    }
}

Layer* LayerSystem::getLayer(int index)
{
    if (index < 0 || index >= MAX_LAYERS)
        return nullptr;
    return layers[index];
}

void LayerSystem::setLayerActive(int index, bool active)
{
    if (index >= 0 && index < MAX_LAYERS)
        layers[index]->active = active;
}

bool LayerSystem::isLayerActive(int index) const
{
    if (index < 0 || index >= MAX_LAYERS)
        return false;
    return layers[index]->active;
}

void LayerSystem::setMainCamera(float x, float y, float zoom)
{
    mainCamera.x = x;
    mainCamera.y = y;
    mainCamera.zoom = zoom;
}

void LayerSystem::updateMainCamera(float x, float y)
{
    mainCamera.x = x;
    mainCamera.y = y;
}

void LayerSystem::getMainCameraPosition(float& x, float& y) const
{
    x = mainCamera.x;
    y = mainCamera.y;
}

void LayerSystem::addGraphToLayer(int layerIndex, Graph* graph)
{
    if (layerIndex >= 0 && layerIndex < MAX_LAYERS && graph)
    {
        layers[layerIndex]->graphs.push_back(graph);
    }
}

void LayerSystem::removeGraphFromLayer(int layerIndex, Graph* graph)
{
    if (layerIndex >= 0 && layerIndex < MAX_LAYERS)
    {
        Layer* layer = layers[layerIndex];
        auto it = std::find(layer->graphs.begin(), layer->graphs.end(), graph);
        if (it != layer->graphs.end())
        {
            layer->graphs.erase(it);
        }
    }
}

void LayerSystem::setLayerBackground(int layerIndex, Graph* tileGraph)
{
    if (layerIndex >= 0 && layerIndex < MAX_LAYERS && tileGraph)
    {
        Layer* layer = layers[layerIndex];
        layer->background.tileGraph = tileGraph;
        layer->background.active = true;
        layer->background.scrollX = 0;
        layer->background.scrollY = 0;
    }
}

void LayerSystem::removeLayerBackground(int layerIndex)
{
    if (layerIndex >= 0 && layerIndex < MAX_LAYERS)
    {
        layers[layerIndex]->background.active = false;
        layers[layerIndex]->background.tileGraph = nullptr;
    }
}

void LayerSystem::updateBackgroundScroll(int layerIndex, float scrollX, float scrollY)
{
    if (layerIndex >= 0 && layerIndex < MAX_LAYERS)
    {
        layers[layerIndex]->background.scrollX = scrollX;
        layers[layerIndex]->background.scrollY = scrollY;
    }
}

void LayerSystem::setLayerScrollFactor(int layerIndex, float factorX, float factorY)
{
    if (layerIndex >= 0 && layerIndex < MAX_LAYERS)
    {
        // GUI layer nunca tem scroll factor
        if (layerIndex != GUI_LAYER)
        {
            layers[layerIndex]->scrollFactorX = factorX;
            layers[layerIndex]->scrollFactorY = factorY;
        }
    }
}

void LayerSystem::setLayerOffset(int layerIndex, float offsetX, float offsetY)
{
    if (layerIndex >= 0 && layerIndex < MAX_LAYERS)
    {
        layers[layerIndex]->offsetX = offsetX;
        layers[layerIndex]->offsetY = offsetY;
    }
}

void LayerSystem::drawTiledBackground(const Layer* layer, const LayerCamera& camera)
{
    if (!layer->background.active || !layer->background.tileGraph)
        return;
    
    Graph* tileGraph = layer->background.tileGraph;
    int tileW = (int)tileGraph->texWidth;
    int tileH = (int)tileGraph->texHeight;
    
    // Calcular offset do background
    float bgScrollX = layer->background.scrollX;
    float bgScrollY = layer->background.scrollY;
    
    // Com paralax
    if (layer->index != GUI_LAYER)
    {
        bgScrollX += camera.x * (1.0f - layer->scrollFactorX);
        bgScrollY += camera.y * (1.0f - layer->scrollFactorY);
    }
    
    // Obter offset dentro do tile
    float offsetX, offsetY;
    LayerMath::getTileOffset(bgScrollX, bgScrollY, tileW, tileH, offsetX, offsetY);
    
    // Quantos tiles desenhar
    int tilesX = LayerMath::getTileCountX(screenWidth, tileW);
    int tilesY = LayerMath::getTileCountY(screenHeight, tileH);
    
    // Desenhar tiles (usando RenderTile ou setPosition + render)
    for (int y = 0; y < tilesY; y++)
    {
        for (int x = 0; x < tilesX; x++)
        {
            float posX = offsetX + x * tileW;
            float posY = offsetY + y * tileH;
            
            tileGraph->setPosition(posX, posY);
            tileGraph->render();
        }
    }
}

void LayerSystem::renderLayer(int layerIndex)
{
    if (layerIndex < 0 || layerIndex >= MAX_LAYERS)
        return;
    
    Layer* layer = layers[layerIndex];
    
    if (!layer->active)
        return;
    
    // Usar câmara apropriada
    LayerCamera camera = layer->useLocalCamera ? layer->camera : mainCamera;
    
    // Desenhar background
    drawTiledBackground(layer, camera);
    
    // Desenhar gráficos
    for (size_t i = 0; i < layer->graphs.size(); i++)
    {
        Graph* graph = layer->graphs[i];
        
        if (layer->index == GUI_LAYER)
        {
            // GUI: render direto, sem câmara
            graph->render();
        }
        else
        {
            // Game layers: ajustar com câmara e paralax
            
            // Salvar posição original
            float origX = graph->x;
            float origY = graph->y;
            
            // Aplicar câmara e paralax
            float adjustedX = origX - camera.x * (layer->scrollFactorX - 1.0f);
            float adjustedY = origY - camera.y * (layer->scrollFactorY - 1.0f);
            
            graph->setPosition(adjustedX, adjustedY);
            graph->render();
            
            // Restaurar posição
            graph->setPosition(origX, origY);
        }
    }
}

void LayerSystem::render()
{
    // Renderizar todas as layers (0 a 5)
    for (int i = 0; i < MAX_LAYERS; i++)
    {
        renderLayer(i);
    }
}

void LayerSystem::screenToWorldPos(float screenX, float screenY, int layerIndex, float& worldX, float& worldY)
{
    if (layerIndex < 0 || layerIndex >= MAX_LAYERS)
    {
        worldX = screenX;
        worldY = screenY;
        return;
    }
    
    Layer* layer = layers[layerIndex];
    LayerCamera camera = layer->useLocalCamera ? layer->camera : mainCamera;
    
    if (layerIndex == GUI_LAYER)
    {
        // GUI: não precisa conversão
        worldX = screenX;
        worldY = screenY;
        return;
    }
    
    // Converter screen -> world
    worldX = screenX + camera.x * (layer->scrollFactorX - 1.0f);
    worldY = screenY + camera.y * (layer->scrollFactorY - 1.0f);
}

void LayerSystem::worldToScreenPos(float worldX, float worldY, int layerIndex, float& screenX, float& screenY)
{
    if (layerIndex < 0 || layerIndex >= MAX_LAYERS)
    {
        screenX = worldX;
        screenY = worldY;
        return;
    }
    
    Layer* layer = layers[layerIndex];
    LayerCamera camera = layer->useLocalCamera ? layer->camera : mainCamera;
    
    if (layerIndex == GUI_LAYER)
    {
        // GUI: não precisa conversão
        screenX = worldX;
        screenY = worldY;
        return;
    }
    
    // Converter world -> screen
    screenX = worldX - camera.x * (layer->scrollFactorX - 1.0f);
    screenY = worldY - camera.y * (layer->scrollFactorY - 1.0f);
}
