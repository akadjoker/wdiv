#include "LayerSystem.h"
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
    mainCamera.position = Vec2(width / 2.0f, height / 2.0f);
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

void LayerSystem::setMainCamera(Vec2 position, float zoom)
{
    mainCamera.position = position;
    mainCamera.zoom = zoom;
}

void LayerSystem::updateMainCamera(Vec2 newPosition)
{
    mainCamera.position = newPosition;
}

void LayerSystem::addGraphToLayer(int layerIndex, Graph* graph)
{
    if (layerIndex >= 0 && layerIndex < MAX_LAYERS && graph)
    {
        layers[layerIndex]->graphs.push(graph);
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

void LayerSystem::setLayerBackground(int layerIndex, Graph* tileGraph, int tileW, int tileH)
{
    if (layerIndex >= 0 && layerIndex < MAX_LAYERS && tileGraph)
    {
        Layer* layer = layers[layerIndex];
        layer->background.tileGraph = tileGraph;
        layer->background.tileWidth = tileW;
        layer->background.tileHeight = tileH;
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

void LayerSystem::drawTiledBackground(const Layer* layer, const Camera2D_Custom& camera)
{
    if (!layer->background.active || !layer->background.tileGraph)
        return;
    
    Graph* tileGraph = layer->background.tileGraph;
    int tileW = layer->background.tileWidth;
    int tileH = layer->background.tileHeight;
    
    // Calcular offset do background
    float bgScrollX = layer->background.scrollX;
    float bgScrollY = layer->background.scrollY;
    
    // Com paralax
    if (layer->index != GUI_LAYER)
    {
        bgScrollX += camera.position.x * (1.0f - layer->scrollFactorX);
        bgScrollY += camera.position.y * (1.0f - layer->scrollFactorY);
    }
    
    // Obter offset dentro do tile
    float offsetX, offsetY;
    LayerMath::getTileOffset(bgScrollX, bgScrollY, tileW, tileH, offsetX, offsetY);
    
    // Quantos tiles desenhar
    int tilesX = LayerMath::getTileCountX(screenWidth, tileW);
    int tilesY = LayerMath::getTileCountY(screenHeight, tileH);
    
    // Desenhar tiles
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

void LayerSystem::applyLayerTransform(const Layer* layer, const Camera2D_Custom& camera)
{
    if (layer->index == GUI_LAYER)
    {
        // GUI layer: sem transformação
        return;
    }
    
    // Calcular offset com paralax
    float offsetX = layer->offsetX;
    float offsetY = layer->offsetY;
    
    // Aplicar scroll factor (paralax)
    offsetX -= camera.position.x * (layer->scrollFactorX - 1.0f);
    offsetY -= camera.position.y * (layer->scrollFactorY - 1.0f);
    
    // TODO: Aqui seria aplicado o transform com raylib
    // Por enquanto, as posições dos gráficos já vão ser ajustadas no render
}

void LayerSystem::resetLayerTransform()
{
    // TODO: Reset das transformações
}

void LayerSystem::renderLayer(int layerIndex)
{
    if (layerIndex < 0 || layerIndex >= MAX_LAYERS)
        return;
    
    Layer* layer = layers[layerIndex];
    
    if (!layer->active)
        return;
    
    // Usar câmara apropriada
    Camera2D_Custom camera = layer->useLocalCamera ? layer->camera : mainCamera;
    
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
            float adjustedX = origX - camera.position.x * (layer->scrollFactorX - 1.0f);
            float adjustedY = origY - camera.position.y * (layer->scrollFactorY - 1.0f);
            
            graph->setPosition(adjustedX, adjustedY);
            graph->render();
            
            // Restaurar posição
            graph->setPosition(origX, origY);
        }
    }
    
    resetLayerTransform();
}

void LayerSystem::render()
{
    // Renderizar todas as layers
    for (int i = 0; i < MAX_LAYERS; i++)
    {
        renderLayer(i);
    }
}

Vec2 LayerSystem::screenToWorldPos(Vec2 screenPos, int layerIndex)
{
    if (layerIndex < 0 || layerIndex >= MAX_LAYERS)
        return screenPos;
    
    Layer* layer = layers[layerIndex];
    Camera2D_Custom camera = layer->useLocalCamera ? layer->camera : mainCamera;
    
    if (layerIndex == GUI_LAYER)
    {
        // GUI: não precisa conversão
        return screenPos;
    }
    
    // Converter screen -> world
    Vec2 worldPos = screenPos;
    worldPos.x += camera.position.x * (layer->scrollFactorX - 1.0f);
    worldPos.y += camera.position.y * (layer->scrollFactorY - 1.0f);
    
    return worldPos;
}

Vec2 LayerSystem::worldToScreenPos(Vec2 worldPos, int layerIndex)
{
    if (layerIndex < 0 || layerIndex >= MAX_LAYERS)
        return worldPos;
    
    Layer* layer = layers[layerIndex];
    Camera2D_Custom camera = layer->useLocalCamera ? layer->camera : mainCamera;
    
    if (layerIndex == GUI_LAYER)
    {
        // GUI: não precisa conversão
        return worldPos;
    }
    
    // Converter world -> screen
    Vec2 screenPos = worldPos;
    screenPos.x -= camera.position.x * (layer->scrollFactorX - 1.0f);
    screenPos.y -= camera.position.y * (layer->scrollFactorY - 1.0f);
    
    return screenPos;
}
