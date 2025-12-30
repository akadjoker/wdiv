// exemplo_layer_system.cpp - Demonstração completa do LayerSystem
#include "raylib.h"
#include "LayerSystem.h"
#include "Graph.h"
#include <iostream>

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Layer System Demo - Divgames Style");
    SetTargetFPS(60);
    
    GraphManager graphManager;
    LayerSystem layerSystem(SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // ===== CRIAR GRÁFICOS =====
    
    // Background da layer 0 (céu - muito paralax)
    Graph* skyTile = graphManager.create();
    skyTile->loadFile("assets/sky_tile.png");  // Criar um tile simples
    
    // Background da layer 1 (nuvens - paralax médio)
    Graph* cloudsTile = graphManager.create();
    cloudsTile->loadFile("assets/clouds_tile.png");
    
    // Background da layer 2 (terreno distante - paralax médio)
    Graph* terrainFarTile = graphManager.create();
    terrainFarTile->loadFile("assets/terrain_far_tile.png");
    
    // Background da layer 3 (terreno próximo - pouco paralax)
    Graph* terrainNearTile = graphManager.create();
    terrainNearTile->loadFile("assets/terrain_near_tile.png");
    
    // Gráficos da layer 4 (objetos do mundo)
    Graph* player = graphManager.create();
    player->loadFile("assets/player.png");
    player->setPosition(640, 400);
    
    Graph* enemy1 = graphManager.create();
    enemy1->loadFile("assets/enemy.png");
    enemy1->setPosition(400, 450);
    
    Graph* enemy2 = graphManager.create();
    enemy2->loadFile("assets/enemy.png");
    enemy2->setPosition(900, 450);
    
    Graph* coin = graphManager.create();
    coin->loadFile("assets/coin.png");
    coin->setPosition(700, 300);
    
    // ===== CONFIGURAR LAYERS =====
    
    // Layer 0: Céu (paralax muito alto = 0.2)
    layerSystem.setLayerBackground(0, skyTile, 32, 32);
    layerSystem.setLayerScrollFactor(0, 0.2f, 0.2f);
    
    // Layer 1: Nuvens (paralax 0.4)
    layerSystem.setLayerBackground(1, cloudsTile, 64, 32);
    layerSystem.setLayerScrollFactor(1, 0.4f, 0.4f);
    
    // Layer 2: Terreno distante (paralax 0.6)
    layerSystem.setLayerBackground(2, terrainFarTile, 128, 64);
    layerSystem.setLayerScrollFactor(2, 0.6f, 0.8f);
    
    // Layer 3: Terreno próximo (paralax 0.9 = quase normal)
    layerSystem.setLayerBackground(3, terrainNearTile, 64, 48);
    layerSystem.setLayerScrollFactor(3, 0.9f, 0.9f);
    
    // Layer 4: Objetos do jogo (paralax 1.0 = normal)
    layerSystem.setLayerScrollFactor(4, 1.0f, 1.0f);
    layerSystem.addGraphToLayer(4, player);
    layerSystem.addGraphToLayer(4, enemy1);
    layerSystem.addGraphToLayer(4, enemy2);
    layerSystem.addGraphToLayer(4, coin);
    
    // Layer 5: GUI (sem paralax, sempre visível)
    // Aqui iríamos adicionar UI elements
    
    // ===== VARIÁVEIS DE JOGO =====
    Vec2 playerPos = Vec2(640, 400);
    float cameraX = playerPos.x;
    float cameraY = playerPos.y;
    
    // ===== LOOP PRINCIPAL =====
    
    while (!WindowShouldClose())
    {
        // ===== INPUT =====
        
        if (IsKeyDown(KEY_LEFT))
            playerPos.x -= 3;
        if (IsKeyDown(KEY_RIGHT))
            playerPos.x += 3;
        if (IsKeyDown(KEY_UP))
            playerPos.y -= 3;
        if (IsKeyDown(KEY_DOWN))
            playerPos.y += 3;
        
        // ===== UPDATE =====
        
        // Atualizar posição do player
        player->setPosition(playerPos.x, playerPos.y);
        
        // Smooth camera follow
        cameraX += (playerPos.x - cameraX) * 0.1f;
        cameraY += (playerPos.y - cameraY) * 0.1f;
        
        // Atualizar câmara principal
        layerSystem.setMainCamera(Vec2(cameraX, cameraY), 1.0f);
        
        // ===== RENDER =====
        
        BeginDrawing();
        ClearBackground(BLACK);
        
        // Renderizar todas as layers (com paralax automático)
        layerSystem.render();
        
        // ===== UI =====
        
        // Painel de informações
        DrawRectangle(10, 10, 350, 200, ColorAlpha(BLACK, 0.7f));
        
        DrawText("LAYER SYSTEM DEMO", 20, 20, 16, YELLOW);
        DrawText("Divgames Studio Style", 20, 40, 12, LIGHTGRAY);
        
        DrawText("Controls:", 20, 65, 12, WHITE);
        DrawText("ARROWS - Move Player", 20, 80, 10, LIGHTGRAY);
        
        DrawText("Layers:", 20, 100, 12, WHITE);
        DrawText("0: Sky (paralax 0.2)", 20, 115, 10, LIGHTGRAY);
        DrawText("1: Clouds (paralax 0.4)", 20, 128, 10, LIGHTGRAY);
        DrawText("2: Terrain Far (paralax 0.6)", 20, 141, 10, LIGHTGRAY);
        DrawText("3: Terrain Near (paralax 0.9)", 20, 154, 10, LIGHTGRAY);
        DrawText("4: World Objects (paralax 1.0)", 20, 167, 10, LIGHTGRAY);
        DrawText("5: GUI (no paralax)", 20, 180, 10, LIGHTGRAY);
        
        DrawText(TextFormat("FPS: %d", GetFPS()), 20, 200, 12, GREEN);
        
        // Mostrar posição da câmara
        Vec2 camPos = layerSystem.getMainCameraPosition();
        DrawText(TextFormat("Camera: (%.0f, %.0f)", camPos.x, camPos.y), 
                 SCREEN_WIDTH - 200, 10, 12, YELLOW);
        
        // Mostrar posição do player
        DrawText(TextFormat("Player: (%.0f, %.0f)", playerPos.x, playerPos.y), 
                 SCREEN_WIDTH - 200, 25, 12, WHITE);
        
        EndDrawing();
    }
    
    // Cleanup
    graphManager.cleanup();
    CloseWindow();
    
    return 0;
}

/*
COMPILAÇÃO:
g++ -std=c++17 exemplo_layer_system.cpp LayerSystem.cpp Graph.cpp Physics2D.cpp -o layerdemo -lraylib -lm

FEATURES:
✓ 6 layers com paralax independente
✓ Background tiling automático
✓ Câmara suave
✓ GUI layer sem paralax
✓ Conversão world <-> screen
✓ Estilo Divgames Studio

MELHORIAS FUTURAS:
- Câmaras locais por layer
- Scroll offset customizável
- Blend modes por layer
- Efeitos visuais per layer
*/
