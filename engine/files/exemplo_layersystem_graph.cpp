// exemplo_layersystem_graph.cpp - LayerSystem com Graph integrado
#include "raylib.h"
#include "LayerSystem_Graph.h"
#include "Graph.h"

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "LayerSystem + Graph Demo");
    SetTargetFPS(60);
    
    // Managers
    GraphManager graphManager;
    LayerSystem layerSystem(SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // ===== CRIAR BACKGROUNDS PARA TILING =====
    
    // Layer 0: Sky (paralax 0.2)
    Graph* skyTile = graphManager.create();
    skyTile->loadFile("assets/sky_tile.png");  // 32x32
    layerSystem.setLayerBackground(0, skyTile);
    layerSystem.setLayerScrollFactor(0, 0.2f, 0.2f);
    
    // Layer 1: Clouds (paralax 0.4)
    Graph* cloudsTile = graphManager.create();
    cloudsTile->loadFile("assets/clouds_tile.png");  // 64x32
    layerSystem.setLayerBackground(1, cloudsTile);
    layerSystem.setLayerScrollFactor(1, 0.4f, 0.4f);
    
    // Layer 2: Far terrain (paralax 0.6)
    Graph* terrainFarTile = graphManager.create();
    terrainFarTile->loadFile("assets/terrain_far_tile.png");  // 128x64
    layerSystem.setLayerBackground(2, terrainFarTile);
    layerSystem.setLayerScrollFactor(2, 0.6f, 0.8f);
    
    // Layer 3: Near terrain (paralax 0.9)
    Graph* terrainNearTile = graphManager.create();
    terrainNearTile->loadFile("assets/terrain_near_tile.png");  // 64x48
    layerSystem.setLayerBackground(3, terrainNearTile);
    layerSystem.setLayerScrollFactor(3, 0.9f, 0.9f);
    
    // ===== CRIAR GAME OBJECTS (Layer 4) =====
    
    // Player
    Graph* player = graphManager.create();
    player->loadFile("assets/player.png");
    player->setPosition(640, 400);
    player->sizeX = 100;
    player->sizeY = 100;
    layerSystem.addGraphToLayer(4, player);
    
    // Alguns inimigos
    std::vector<Graph*> enemies;
    for (int i = 0; i < 3; i++)
    {
        Graph* enemy = graphManager.create();
        enemy->loadFile("assets/enemy.png");
        enemy->setPosition(400 + i * 300, 450);
        enemy->sizeX = 100;
        enemy->sizeY = 100;
        layerSystem.addGraphToLayer(4, enemy);
        enemies.push_back(enemy);
    }
    
    // Moedas
    std::vector<Graph*> coins;
    for (int i = 0; i < 5; i++)
    {
        Graph* coin = graphManager.create();
        coin->loadFile("assets/coin.png");
        coin->setPosition(300 + i * 200, 250);
        coin->sizeX = 50;
        coin->sizeY = 50;
        layerSystem.addGraphToLayer(4, coin);
        coins.push_back(coin);
    }
    
    // ===== GUI Layer (5) =====
    
    Graph* healthBar = graphManager.create();
    healthBar->loadFile("assets/health_bar.png");
    healthBar->setPosition(20, 20);
    layerSystem.addGraphToLayer(5, healthBar);
    
    Graph* scoreText = graphManager.create();
    scoreText->loadFile("assets/score.png");
    scoreText->setPosition(SCREEN_WIDTH - 200, 20);
    layerSystem.addGraphToLayer(5, scoreText);
    
    // ===== VARIÁVEIS DE JOGO =====
    
    float playerWorldX = 640;
    float playerWorldY = 400;
    float cameraX = playerWorldX;
    float cameraY = playerWorldY;
    
    float playerVelX = 0;
    float playerVelY = 0;
    
    // ===== LOOP PRINCIPAL =====
    
    while (!WindowShouldClose())
    {
        // ===== INPUT =====
        
        if (IsKeyDown(KEY_LEFT))
            playerVelX = -3;
        else if (IsKeyDown(KEY_RIGHT))
            playerVelX = 3;
        else
            playerVelX = 0;
        
        if (IsKeyDown(KEY_UP))
            playerVelY = -3;
        else if (IsKeyDown(KEY_DOWN))
            playerVelY = 3;
        else
            playerVelY = 0;
        
        // ===== UPDATE =====
        
        // Atualizar posição do player (mundo)
        playerWorldX += playerVelX;
        playerWorldY += playerVelY;
        
        // Limitar o player em uma área
        if (playerWorldX < 100) playerWorldX = 100;
        if (playerWorldX > 3000) playerWorldX = 3000;
        if (playerWorldY < 100) playerWorldY = 100;
        if (playerWorldY > 2000) playerWorldY = 2000;
        
        // Atualizar gráfico do player
        player->setPosition(playerWorldX, playerWorldY);
        
        // Câmara segue player com smooth
        cameraX += (playerWorldX - cameraX) * 0.1f;
        cameraY += (playerWorldY - cameraY) * 0.1f;
        
        // Limitar câmara (opcional)
        if (cameraX < SCREEN_WIDTH / 2) cameraX = SCREEN_WIDTH / 2;
        if (cameraY < SCREEN_HEIGHT / 2) cameraY = SCREEN_HEIGHT / 2;
        
        // Atualizar câmara principal
        layerSystem.setMainCamera(cameraX, cameraY, 1.0f);
        
        // Atualizar backgrounds com scroll
        // (Normalmente isto seria automático, mas podes forçar se quiseres efeitos especiais)
        // layerSystem.updateBackgroundScroll(0, cameraX * 0.2f, cameraY * 0.2f);
        
        // ===== RENDER =====
        
        BeginDrawing();
        ClearBackground({20, 20, 40, 255});
        
        // Renderizar todas as layers (com paralax automático)
        layerSystem.render();
        
        // ===== DEBUG UI =====
        
        DrawRectangle(10, 10, 400, 220, ColorAlpha(BLACK, 0.7f));
        DrawText("LAYERSYSTEM + GRAPH DEMO", 20, 20, 16, YELLOW);
        DrawText("6 Layers with Parallax", 20, 40, 12, LIGHTGRAY);
        
        DrawText("Controls:", 20, 65, 12, WHITE);
        DrawText("ARROWS - Move Player", 20, 80, 10, LIGHTGRAY);
        
        DrawText("Layers Configuration:", 20, 100, 12, WHITE);
        DrawText("0: Sky (paralax 0.2)", 20, 115, 10, LIGHTGRAY);
        DrawText("1: Clouds (paralax 0.4)", 20, 128, 10, LIGHTGRAY);
        DrawText("2: Terrain Far (paralax 0.6)", 20, 141, 10, LIGHTGRAY);
        DrawText("3: Terrain Near (paralax 0.9)", 20, 154, 10, LIGHTGRAY);
        DrawText("4: Game Objects (paralax 1.0)", 20, 167, 10, LIGHTGRAY);
        DrawText("5: GUI (no paralax)", 20, 180, 10, LIGHTGRAY);
        
        DrawText(TextFormat("FPS: %d", GetFPS()), 20, 200, 12, GREEN);
        
        // Debug info
        DrawRectangle(SCREEN_WIDTH - 300, 10, 290, 100, ColorAlpha(BLACK, 0.7f));
        float camX, camY;
        layerSystem.getMainCameraPosition(camX, camY);
        DrawText(TextFormat("Camera: (%.0f, %.0f)", camX, camY), SCREEN_WIDTH - 290, 20, 12, YELLOW);
        DrawText(TextFormat("Player: (%.0f, %.0f)", playerWorldX, playerWorldY), SCREEN_WIDTH - 290, 40, 12, WHITE);
        DrawText(TextFormat("Objects in Layer 4: %zu", layerSystem.getLayer(4)->graphs.size()), SCREEN_WIDTH - 290, 60, 12, GREEN);
        DrawText(TextFormat("Enemies: %zu | Coins: %zu", enemies.size(), coins.size()), SCREEN_WIDTH - 290, 80, 12, GREEN);
        
        EndDrawing();
    }
    
    // Cleanup
    graphManager.cleanup();
    CloseWindow();
    
    return 0;
}

/*
COMPILAÇÃO:
g++ -std=c++17 exemplo_layersystem_graph.cpp LayerSystem_Graph.cpp Graph.cpp -o demo -lraylib -lm

FEATURES:
✓ 6 layers independentes
✓ Background tiling automático
✓ Paralax por layer
✓ Câmara principal sincronizada
✓ GUI layer sem paralax
✓ Suporte para múltiplos objetos por layer
✓ Conversão de coordenadas mundo <-> tela

CONTROLES:
→ ARROWS: Mover player
→ ESC: Sair

ESTRUTURA:
Layer 0: Sky background (paralax 0.2)
Layer 1: Clouds background (paralax 0.4)
Layer 2: Far terrain background (paralax 0.6)
Layer 3: Near terrain background (paralax 0.9)
Layer 4: Game objects - Player, enemies, coins
Layer 5: GUI - Health bar, score, etc

PARALAX AUTOMÁTICO:
Cada layer com scrollFactor < 1.0 fica mais distante visualmente
Efeito 3D com múltiplas camadas!
*/
