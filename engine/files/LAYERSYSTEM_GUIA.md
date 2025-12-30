# 🎮 LayerSystem - Guia Completo

## O que é LayerSystem?

Um sistema de **6 layers independentes** com **paralax automático**, **tiling de backgrounds**, e **câmara sincronizada**. Estilo Divgames Studio!

```
Layer 5 (GUI)        ← Sem câmara, sempre visível
Layer 4 (Objects)    ← Paralax 1.0 (move com câmara)
Layer 3 (Terrain)    ← Paralax 0.9 (pouco movimento)
Layer 2 (Far)        ← Paralax 0.6 (efeito de profundidade)
Layer 1 (Clouds)     ← Paralax 0.4 (muito paralax)
Layer 0 (Sky)        ← Paralax 0.2 (quase imóvel)
```


## Estrutura

```cpp
LayerSystem
├─ Layer 0 (Céu)
│  ├─ Background tiling
│  └─ ScrollFactor: 0.2
├─ Layer 1 (Nuvens)
│  ├─ Background tiling
│  └─ ScrollFactor: 0.4
├─ Layer 2 (Terreno Distante)
│  ├─ Background tiling
│  └─ ScrollFactor: 0.6
├─ Layer 3 (Terreno Próximo)
│  ├─ Background tiling
│  └─ ScrollFactor: 0.9
├─ Layer 4 (Objetos/Jogador)
│  ├─ Gráficos
│  └─ ScrollFactor: 1.0
└─ Layer 5 (GUI)
   ├─ UI Elements
   └─ Sem câmara
```


## Setup Básico

```cpp
// 1. Criar LayerSystem
LayerSystem layerSystem(SCREEN_WIDTH, SCREEN_HEIGHT);

// 2. Criar ou carregar gráficos
Graph* skyTile = graphManager.create();
skyTile->loadFile("assets/sky.png");

// 3. Configurar layer 0 com background
layerSystem.setLayerBackground(0, skyTile, 32, 32);  // tile 32x32
layerSystem.setLayerScrollFactor(0, 0.2f, 0.2f);     // paralax 0.2

// 4. Adicionar objetos
Graph* player = graphManager.create();
player->loadFile("assets/player.png");
layerSystem.addGraphToLayer(4, player);

// 5. Render
while (!WindowShouldClose()) {
    // Update câmara
    layerSystem.setMainCamera(playerPos);
    
    // Render todas as layers
    layerSystem.render();
}
```


## API Completa

### Câmara

```cpp
// Definir posição da câmara
layerSystem.setMainCamera(Vec2(640, 360), 1.0f);

// Atualizar posição (mais eficiente)
layerSystem.updateMainCamera(Vec2(newX, newY));

// Obter posição
Vec2 camPos = layerSystem.getMainCameraPosition();

// Obter zoom
float zoom = layerSystem.getMainCameraZoom();
```

### Gráficos

```cpp
// Adicionar gráfico a uma layer
layerSystem.addGraphToLayer(4, playerGraph);
layerSystem.addGraphToLayer(4, enemyGraph);

// Remover gráfico
layerSystem.removeGraphFromLayer(4, enemyGraph);
```

### Background Tiling

```cpp
// Definir background
layerSystem.setLayerBackground(0, skyTile, 32, 32);
//                             layer, tile,    W,   H

// Atualizar scroll do background
layerSystem.updateBackgroundScroll(0, scrollX, scrollY);

// Remover background
layerSystem.removeLayerBackground(0);
```

### Paralax

```cpp
// Definir scroll factor (paralax)
layerSystem.setLayerScrollFactor(0, 0.2f, 0.2f);
// 0.0 = imóvel com câmara
// 0.5 = move 50% com câmara
// 1.0 = move 100% com câmara

// Definir offset local
layerSystem.setLayerOffset(0, 50, 100);
```

### Layers

```cpp
// Obter layer
Layer* layer = layerSystem.getLayer(0);

// Ativar/Desativar
layerSystem.setLayerActive(0, true);
layerSystem.setLayerActive(0, false);

// Verificar se ativa
bool active = layerSystem.isLayerActive(0);
```

### Conversão de Coordenadas

```cpp
// Screen -> World (com paralax)
Vec2 worldPos = layerSystem.screenToWorldPos(mousePos, 4);

// World -> Screen
Vec2 screenPos = layerSystem.worldToScreenPos(worldPos, 4);
```

### Render

```cpp
// Renderizar todas as layers
layerSystem.render();

// Renderizar uma layer específica
layerSystem.renderLayer(0);
```


## Exemplo Completo

```cpp
#include "LayerSystem.h"

int main() {
    InitWindow(1280, 720, "Layers");
    
    GraphManager manager;
    LayerSystem layers(1280, 720);
    
    // Criar backgrounds
    Graph* sky = manager.create();
    sky->loadFile("sky.png");
    
    Graph* clouds = manager.create();
    clouds->loadFile("clouds.png");
    
    // Setup layers
    layers.setLayerBackground(0, sky, 32, 32);
    layers.setLayerScrollFactor(0, 0.2f, 0.2f);
    
    layers.setLayerBackground(1, clouds, 64, 32);
    layers.setLayerScrollFactor(1, 0.4f, 0.4f);
    
    // Criar player
    Graph* player = manager.create();
    player->loadFile("player.png");
    player->setPosition(640, 360);
    layers.addGraphToLayer(4, player);
    
    // Loop
    while (!WindowShouldClose()) {
        // Update
        if (IsKeyDown(KEY_RIGHT))
            player->x += 2;
        
        // Câmara segue player
        layers.setMainCamera(Vec2(player->x, player->y));
        
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


## ScrollFactor - Paralax Explicado

ScrollFactor controla como cada layer se move com a câmara.

```
ScrollFactor = 0.0f  (imóvel)
  ↓
  Player move 100px para direita, câmara move 100px
  → Layer NÃO se move (fica imóvel)
  
ScrollFactor = 0.5f  (paralax)
  ↓
  Player move 100px para direita, câmara move 100px
  → Layer move 50px (metade)
  
ScrollFactor = 1.0f  (normal)
  ↓
  Player move 100px para direita, câmara move 100px
  → Layer move 100px (acompanha tudo)
```

### Recomendações de ScrollFactor:

```cpp
Layer 0 (Sky):           0.1f - 0.3f   (muito distante)
Layer 1 (Clouds):        0.3f - 0.5f   (moderadamente distante)
Layer 2 (Far terrain):   0.5f - 0.7f   (terreno distante)
Layer 3 (Near terrain):  0.8f - 0.95f  (terreno próximo)
Layer 4 (Objects):       1.0f          (move com tudo)
Layer 5 (GUI):           0.0f          (estático na tela)
```


## Background Tiling

O tiling é **automático**. Basta:

```cpp
// Definir tile
layerSystem.setLayerBackground(0, tileGraph, 32, 32);

// Resto é automático!
// - Tiles são repetidos até cobrir a tela
// - Scroll é automático com câmara
// - Paralax é aplicado
```

Como funciona:
```
Tile 32x32    Camera em (100, 50)
┌─────┐
│     │  → Tiled automaticamente
└─────┘
         ┌─────┬─────┬─────┐
         │     │     │     │
         ├─────┼─────┼─────┤
         │     │     │     │
         └─────┴─────┴─────┘
```


## Layer 5 - GUI Layer

A camada 5 é **especial**:

```cpp
// GUI sempre na tela, sem câmara
Graph* lifeBar = manager.create();
lifeBar->setPosition(10, 10);  // Canto superior esquerdo
layers.addGraphToLayer(5, lifeBar);

// Mesmo que câmara se mova:
// → GUI fica no mesmo lugar!

// ScrollFactor é sempre 0.0f
// Não pode ser alterado
```


## Dicas & Truques

### 1. Smooth Camera Follow
```cpp
// Em vez de:
camera = playerPos;

// Fazer:
camera += (playerPos - camera) * 0.1f;
```

### 2. Fog Effect
```cpp
// Layer 0 (Sky) com cor semi-transparent
// Cria efeito de nevoeiro
```

### 3. Parallax Infinito
```cpp
// Com fmod no getTileOffset, tiling é infinito
// Player pode se mover indefinidamente
```

### 4. Multi-layers com Offsets
```cpp
// Cada layer pode ter seu próprio offset
layerSystem.setLayerOffset(0, 50, 100);
```


## Performance

```cpp
// LayerSystem é eficiente:
• 6 layers                    → Negligenciável
• Tiling automático           → O(tiles na tela)
• Paralax                     → O(1) por layer
• Render de gráficos          → O(gráficos)

// Recomendações:
✓ 1-100 gráficos por layer   → Zero problemas
✓ 100-500 gráficos           → Sem problemas
⚠ 500+                       → Considere optimizações
```


## Estrutura de Ficheiros

```
Núcleo:
  LayerSystem.h          ← Header
  LayerSystem.cpp        ← Implementação
  Graph.h/Graph.cpp      ← Usado por LayerSystem

Demo:
  exemplo_layer_system.cpp  ← Exemplo funcional
```


## Compilação

```bash
# Básico
g++ -std=c++17 exemplo_layer_system.cpp LayerSystem.cpp Graph.cpp -o demo -lraylib -lm

# Com seu projeto
g++ -std=c++17 *.cpp -o game -lraylib -lm
```


## Próximos Passos

1. Criar os assets (sky.png, clouds.png, etc)
2. Usar `exemplo_layer_system.cpp` como base
3. Adaptar para seu jogo
4. Adicionar física (Physics2D) na layer 4

```cpp
// Integração com Physics2D:
Physics2D::World world;
world.setGravity(0, 9.8f);

// Gráfico segue corpo físico
physics_body->position = player->x;
physics_body->position = player->y;

// Render segue gráfico que segue corpo
player->setPosition(physics_body->position.x, physics_body->position.y);
```


## Troubleshooting

**P: Tiles não aparecem?**
R: Verificar se `tileGraph` está carregado e `setLayerBackground` foi chamado.

**P: Paralax não funciona?**
R: Verificar se `setLayerScrollFactor` foi chamado com valores 0.0-1.0.

**P: GUI fica hidden atrás de objetos?**
R: GUI layer (5) é renderizada por último, deve estar visível.

**P: Camera muito rápida/lenta?**
R: Ajustar o fator de smooth: `camera += (target - camera) * 0.05f;`

---

**LayerSystem completo, pronto para usar!** 🎮🎨

