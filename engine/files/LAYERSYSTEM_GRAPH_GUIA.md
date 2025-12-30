# 🎮 LayerSystem Para Graph - Guia de Integração

## O Sistema

Sistema de **6 layers paralelos** com **tiling automático** e **paralax**, desenhado especificamente para funcionar com o teu **Graph.h**.

```
┌────────────────────────────────────────┐
│ Layer 5 (GUI)        - Sem câmara     │
│ Layer 4 (Objects)    - Paralax 1.0    │
│ Layer 3 (Terrain)    - Paralax 0.9    │
│ Layer 2 (Far)        - Paralax 0.6    │
│ Layer 1 (Clouds)     - Paralax 0.4    │
│ Layer 0 (Sky)        - Paralax 0.2    │
└────────────────────────────────────────┘
```

## Incorporar no Teu Projeto

1. **Copiar ficheiros:**
   ```
   LayerSystem_Graph.h
   LayerSystem_Graph.cpp
   ```

2. **Incluir no header:**
   ```cpp
   #include "LayerSystem_Graph.h"
   ```

3. **Usar:**
   ```cpp
   LayerSystem layerSystem(SCREEN_WIDTH, SCREEN_HEIGHT);
   ```


## Setup Básico (3 linhas)

```cpp
// Criar
LayerSystem layers(1280, 720);

// Adicionar background com tiling
Graph* skyTile = graphManager.create();
skyTile->loadFile("assets/sky.png");
layers.setLayerBackground(0, skyTile);
layers.setLayerScrollFactor(0, 0.2f, 0.2f);  // Paralax

// Adicionar objeto
Graph* player = graphManager.create();
player->loadFile("assets/player.png");
layers.addGraphToLayer(4, player);

// Update e Render
layers.setMainCamera(playerX, playerY);
layers.render();
```


## API Completa

### Câmara

```cpp
// Definir câmara
layers.setMainCamera(x, y, zoom);

// Atualizar (mais eficiente que setMainCamera)
layers.updateMainCamera(x, y);

// Obter posição
float camX, camY;
layers.getMainCameraPosition(camX, camY);

// Obter zoom
float zoom = layers.getMainCameraZoom();
```

### Layers

```cpp
// Obter layer
Layer* layer = layers.getLayer(0);

// Ativar/Desativar
layers.setLayerActive(0, true);
layers.setLayerActive(0, false);

// Verificar se ativa
bool active = layers.isLayerActive(0);
```

### Background Tiling

```cpp
// Configurar background (com tiling automático)
Graph* tile = graphManager.create();
tile->loadFile("assets/tile.png");
layers.setLayerBackground(0, tile);

// Remover background
layers.removeLayerBackground(0);

// Atualizar scroll (raro, normalmente automático)
layers.updateBackgroundScroll(0, scrollX, scrollY);
```

### Gráficos

```cpp
// Adicionar gráfico
Graph* player = graphManager.create();
player->loadFile("assets/player.png");
layers.addGraphToLayer(4, player);

// Remover gráfico
layers.removeGraphFromLayer(4, player);

// Atualizar posição (normal)
player->setPosition(x, y);
player->setRotation(angle);

// Render automático
// (LayerSystem cuida da câmara e paralax)
```

### Paralax (ScrollFactor)

```cpp
// Define quanto a layer se move com câmara
// 0.0 = imóvel (fundo distante)
// 0.5 = move 50% (paralax)
// 1.0 = move 100% (normal)

layers.setLayerScrollFactor(0, 0.2f, 0.2f);  // Sky (muito distante)
layers.setLayerScrollFactor(1, 0.4f, 0.4f);  // Clouds
layers.setLayerScrollFactor(2, 0.6f, 0.8f);  // Far terrain
layers.setLayerScrollFactor(3, 0.9f, 0.9f);  // Near terrain
layers.setLayerScrollFactor(4, 1.0f, 1.0f);  // Objects (normal)
layers.setLayerScrollFactor(5, 0.0f, 0.0f);  // GUI (imóvel)
```

### Offset

```cpp
// Offset local da layer (raro)
layers.setLayerOffset(0, 50, 100);
```

### Render

```cpp
// Renderizar todas as 6 layers
layers.render();

// Renderizar apenas uma
layers.renderLayer(0);
```

### Conversão de Coordenadas

```cpp
// Screen -> World (útil para mouse)
float worldX, worldY;
layers.screenToWorldPos(mouseX, mouseY, 4, worldX, worldY);

// World -> Screen (útil para draw)
float screenX, screenY;
layers.worldToScreenPos(entityX, entityY, 4, screenX, screenY);
```


## Exemplo Completo

```cpp
#include "raylib.h"
#include "LayerSystem_Graph.h"
#include "Graph.h"

int main() {
    InitWindow(1280, 720, "Layers");
    
    GraphManager manager;
    LayerSystem layers(1280, 720);
    
    // Setup backgrounds
    Graph* sky = manager.create();
    sky->loadFile("sky.png");
    layers.setLayerBackground(0, sky);
    layers.setLayerScrollFactor(0, 0.2f, 0.2f);
    
    Graph* terrain = manager.create();
    terrain->loadFile("terrain.png");
    layers.setLayerBackground(3, terrain);
    layers.setLayerScrollFactor(3, 0.9f, 0.9f);
    
    // Criar player
    Graph* player = manager.create();
    player->loadFile("player.png");
    player->setPosition(640, 400);
    layers.addGraphToLayer(4, player);
    
    // Loop
    float playerX = 640, playerY = 400;
    while (!WindowShouldClose()) {
        // Input
        if (IsKeyDown(KEY_RIGHT)) playerX += 2;
        if (IsKeyDown(KEY_LEFT))  playerX -= 2;
        
        // Update
        player->setPosition(playerX, playerY);
        layers.setMainCamera(playerX, playerY);
        
        // Render
        BeginDrawing();
        ClearBackground(BLACK);
        layers.render();
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}
```


## Padrões Comuns

### 1. Câmara Suave (Smooth Follow)

```cpp
float camX = 640, camY = 360;
float targetX = playerX, targetY = playerY;

// Update
camX += (targetX - camX) * 0.1f;  // 10% interpolação
camY += (targetY - camY) * 0.1f;

layers.setMainCamera(camX, camY);
```

### 2. Limitar Câmara

```cpp
float camX, camY;
layers.getMainCameraPosition(camX, camY);

// Limitar a não sair do mapa
if (camX < 640) camX = 640;
if (camX > 3000) camX = 3000;
if (camY < 360) camY = 360;
if (camY > 2000) camY = 2000;

layers.setMainCamera(camX, camY);
```

### 3. Múltiplos Objetos por Layer

```cpp
// Layer 4 pode ter muitos objetos
for (int i = 0; i < numEnemies; i++) {
    Graph* enemy = manager.create();
    enemy->loadFile("enemy.png");
    enemy->setPosition(enemyX[i], enemyY[i]);
    layers.addGraphToLayer(4, enemy);  // Todos na layer 4
}
```

### 4. Objetos Dinâmicos

```cpp
// Player pode mudar de posição
float playerWorldX = 640;
float playerWorldY = 400;

// Update
playerWorldX += velocityX;
playerWorldY += velocityY;
player->setPosition(playerWorldX, playerWorldY);

// LayerSystem cuida da câmara automaticamente
```

### 5. GUI Fixo na Tela

```cpp
// GUI sempre na layer 5, nunca afectado por câmara

Graph* healthBar = manager.create();
healthBar->loadFile("hud/health.png");
healthBar->setPosition(20, 20);  // Canto superior esquerdo
layers.addGraphToLayer(5, healthBar);

// Mesmo que câmara se mova:
// → HealthBar fica em (20, 20) na tela!
```

### 6. Scroll Manual (Não Automático)

```cpp
// Normalmente paralax é automático:
// layers.setLayerScrollFactor(0, 0.2f, 0.2f);
// layers.setMainCamera(camX, camY);
// PRONTO! Paralax acontece automaticamente

// Mas podes forçar scroll manual se quiseres:
float customScrollX = GetMouseX() * 0.1f;
layers.updateBackgroundScroll(0, customScrollX, 0);
```


## Comportamento das Layers

```
Renderização: Layer 0 → 1 → 2 → 3 → 4 → 5
              (fundo)                   (frente)

ScrollFactor:
  Layer 0 (0.2): Fica 80% imóvel (distante)
  Layer 1 (0.4): Fica 60% imóvel
  Layer 2 (0.6): Fica 40% imóvel
  Layer 3 (0.9): Fica 10% imóvel (próximo)
  Layer 4 (1.0): Fica 0% imóvel (acompanha tudo)
  Layer 5 (0.0): SEMPRE imóvel na tela (GUI)
```


## Com Physics2D

```cpp
#include "Physics2D.h"

Physics2D::World physics;
physics.setGravity(0, 9.8f);

// Corpo físico
Physics2D::Body* playerBody = physics.createCircle(
    Physics2D::Vec2(640, 400), 10, 1.0f
);

// Gráfico (visual)
Graph* playerGraphic = manager.create();
playerGraphic->loadFile("player.png");
layers.addGraphToLayer(4, playerGraphic);

// Update
physics.step();  // Simular física

// Sincronizar
playerGraphic->setPosition(
    playerBody->position.x,
    playerBody->position.y
);
playerGraphic->setRotation(playerBody->orient);

// Câmara segue jogador
layers.setMainCamera(
    playerBody->position.x,
    playerBody->position.y
);

// Render com paralax automático
layers.render();
```


## Problemas Comuns

**P: Câmara não está centralizada no player?**
R: Usa smooth follow:
```cpp
camX += (playerX - camX) * 0.1f;
layers.setMainCamera(camX, camY);
```

**P: Paralax muito forte/fraco?**
R: Ajusta scrollFactor:
```cpp
// Mais distante (fraco)
layers.setLayerScrollFactor(0, 0.1f, 0.1f);

// Menos distante (forte)
layers.setLayerScrollFactor(0, 0.3f, 0.3f);
```

**P: GUI fica hidden atrás de objetos?**
R: GUI é renderizada por último (layer 5), deve estar sempre visível.

**P: Tiling não funciona?**
R: Verifica se tile está carregado e `setLayerBackground` foi chamado.

**P: Coordenadas do mouse incorrectas?**
R: Usa `screenToWorldPos` para converter:
```cpp
float worldX, worldY;
layers.screenToWorldPos(
    GetMouseX(), GetMouseY(), 4,
    worldX, worldY
);
```


## Performance

- 6 layers: O(1)
- Tiling automático: O(tiles visíveis) ≈ 4-20 tiles
- Paralax: O(1) por layer
- Gráficos: O(count)

Total: **MUITO EFICIENTE**
✓ Centenas de objetos sem problemas
✓ Tiling infinito
✓ Paralax suave


## Ficheiros

```
LayerSystem_Graph.h     - Header
LayerSystem_Graph.cpp   - Implementação
Graph.h                 - O teu Graph (mantém igual)
Graph.cpp               - O teu Graph (mantém igual)

exemplo_layersystem_graph.cpp - Exemplo funcional completo
```

## Compilação

```bash
# Básico
g++ -std=c++17 exemplo_layersystem_graph.cpp LayerSystem_Graph.cpp Graph.cpp -o demo -lraylib -lm

# Com seu projeto
g++ -std=c++17 *.cpp -o game -lraylib -lm -I./headers
```

---

**LayerSystem + Graph integrado e pronto para usar!** 🎮✨

