# 🔗 Integração Completa: LayerSystem + Graph + Physics2D

Guia para usar os **3 sistemas juntos** num projeto coerente.

## Arquitetura

```
┌─────────────────────────────────────┐
│         LayerSystem                 │
│  (6 layers com paralax e tiling)    │
├─────────────────────────────────────┤
│  Layer 0-3: Backgrounds (Graph)     │
│  Layer 4:   Game Objects (Graph)    │
│  Layer 5:   GUI (Graph)             │
└──────────────────┬──────────────────┘
                   │
        ┌──────────┴──────────┐
        │                     │
   ┌────▼─────┐         ┌─────▼──────┐
   │  Graph   │         │  Physics2D │
   │  (Draw)  │         │  (Logic)   │
   └──────────┘         └────────────┘
```


## Padrão: GameEntity

### Classe Base

```cpp
#pragma once

#include "Graph.h"
#include "Physics2D.h"
#include "LayerSystem_Graph.h"

class GameEntity
{
public:
    // Visual (Graph)
    Graph* graphic;
    
    // Physics (Physics2D)
    Physics2D::Body* physicsBody;
    
    // Informação
    int id;
    int layerIndex;
    bool active;
    
    // Tipo
    enum Type {
        PLAYER,
        ENEMY,
        PLATFORM,
        COLLECTIBLE,
        PROJECTILE
    } type;
    
    GameEntity(int id, Type t) 
        : id(id), type(t), graphic(nullptr),
          physicsBody(nullptr), layerIndex(4), active(true)
    {}
    
    virtual ~GameEntity() {}
    
    // Sincronizar Physics -> Graphics
    void updateFromPhysics()
    {
        if (graphic && physicsBody)
        {
            graphic->setPosition(
                physicsBody->position.x,
                physicsBody->position.y
            );
            graphic->setRotation(physicsBody->orient * 1000.0f);  // Converter para centimilésimos
        }
    }
    
    // Sincronizar Graphics -> Physics
    void updateToPhysics()
    {
        if (graphic && physicsBody)
        {
            physicsBody->position.x = graphic->x;
            physicsBody->position.y = graphic->y;
        }
    }
    
    virtual void update(float dt) {}
};
```


## Manager de Entidades

```cpp
class GameEntityManager
{
private:
    std::vector<GameEntity*> entities;
    LayerSystem* layerSystem;
    Physics2D::World* physicsWorld;
    GraphManager* graphManager;
    int nextId;
    
public:
    GameEntityManager(LayerSystem* ls, Physics2D::World* pw, GraphManager* gm)
        : layerSystem(ls), physicsWorld(pw), graphManager(gm), nextId(0)
    {}
    
    ~GameEntityManager()
    {
        cleanup();
    }
    
    // Criar entidade
    GameEntity* createEntity(GameEntity::Type type, float x, float y)
    {
        GameEntity* entity = new GameEntity(nextId++, type);
        
        // Criar gráfico
        entity->graphic = graphManager->create();
        
        // Criar corpo físico
        entity->physicsBody = physicsWorld->createCircle(
            Physics2D::Vec2(x, y), 10, 1.0f
        );
        
        // Adicionar à layer
        if (type == GameEntity::COLLECTIBLE) {
            entity->layerIndex = 4;
        }
        layerSystem->addGraphToLayer(entity->layerIndex, entity->graphic);
        
        entities.push_back(entity);
        return entity;
    }
    
    // Remover entidade
    void removeEntity(GameEntity* entity)
    {
        if (!entity) return;
        
        // Remover visual
        layerSystem->removeGraphFromLayer(entity->layerIndex, entity->graphic);
        
        // Remover física
        physicsWorld->destroyBody(entity->physicsBody);
        
        // Remover da lista
        auto it = std::find(entities.begin(), entities.end(), entity);
        if (it != entities.end()) {
            entities.erase(it);
        }
        
        delete entity;
    }
    
    // Update todos
    void update(float dt)
    {
        // 1. Atualizar física
        physicsWorld->step();
        
        // 2. Sincronizar
        for (auto entity : entities) {
            if (entity->active) {
                entity->updateFromPhysics();
                entity->update(dt);
            }
        }
    }
    
    void cleanup()
    {
        for (auto entity : entities) {
            delete entity;
        }
        entities.clear();
    }
    
    size_t getEntityCount() const { return entities.size(); }
    GameEntity* getEntity(int id) {
        for (auto e : entities) {
            if (e->id == id) return e;
        }
        return nullptr;
    }
};
```


## Exemplo Completo Integrado

```cpp
#include "raylib.h"
#include "LayerSystem_Graph.h"
#include "Graph.h"
#include "Physics2D.h"
#include "GameEntity.h"

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Integrated Game Engine");
    SetTargetFPS(60);
    
    // ===== MANAGERS =====
    
    GraphManager graphManager;
    Physics2D::World physicsWorld;
    physicsWorld.setGravity(0, 9.8f);
    
    LayerSystem layerSystem(SCREEN_WIDTH, SCREEN_HEIGHT);
    GameEntityManager entityManager(&layerSystem, &physicsWorld, &graphManager);
    
    // ===== SETUP LAYERS =====
    
    // Layer 0: Sky
    Graph* skyTile = graphManager.create();
    skyTile->loadFile("assets/sky.png");
    layerSystem.setLayerBackground(0, skyTile);
    layerSystem.setLayerScrollFactor(0, 0.2f, 0.2f);
    
    // Layer 3: Ground
    Graph* groundTile = graphManager.create();
    groundTile->loadFile("assets/ground.png");
    layerSystem.setLayerBackground(3, groundTile);
    layerSystem.setLayerScrollFactor(3, 0.9f, 0.9f);
    
    // Layer 5: GUI
    Graph* hud = graphManager.create();
    hud->loadFile("assets/hud.png");
    hud->setPosition(10, 10);
    layerSystem.addGraphToLayer(5, hud);
    
    // ===== CRIAR ENTIDADES =====
    
    // Player
    GameEntity* player = entityManager.createEntity(GameEntity::PLAYER, 640, 400);
    player->graphic->loadFile("assets/player.png");
    player->graphic->sizeX = 100;
    player->graphic->sizeY = 100;
    
    // Inimigos
    for (int i = 0; i < 3; i++) {
        GameEntity* enemy = entityManager.createEntity(
            GameEntity::ENEMY, 
            400 + i * 300, 450
        );
        enemy->graphic->loadFile("assets/enemy.png");
        enemy->graphic->sizeX = 100;
        enemy->graphic->sizeY = 100;
    }
    
    // Plataforma estática
    GameEntity* platform = entityManager.createEntity(GameEntity::PLATFORM, 640, 600);
    platform->graphic->loadFile("assets/platform.png");
    platform->physicsBody->inverseMass = 0.0f;  // Estática
    platform->layerIndex = 3;
    
    // ===== LOOP PRINCIPAL =====
    
    float camX = 640, camY = 360;
    
    while (!WindowShouldClose())
    {
        // ===== INPUT =====
        
        if (IsKeyDown(KEY_LEFT)) {
            player->physicsBody->addForce(Physics2D::Vec2(-5, 0));
        }
        if (IsKeyDown(KEY_RIGHT)) {
            player->physicsBody->addForce(Physics2D::Vec2(5, 0));
        }
        if (IsKeyPressed(KEY_SPACE)) {
            player->physicsBody->addForce(Physics2D::Vec2(0, -100));
        }
        
        // ===== UPDATE =====
        
        // Atualizar entidades (inclui physicsWorld.step())
        entityManager.update(1.0f / 60.0f);
        
        // Câmara suave segue player
        camX += (player->graphic->x - camX) * 0.1f;
        camY += (player->graphic->y - camY) * 0.1f;
        
        // Limitar câmara
        if (camX < 640) camX = 640;
        if (camY < 360) camY = 360;
        
        layerSystem.setMainCamera(camX, camY);
        
        // ===== RENDER =====
        
        BeginDrawing();
        ClearBackground({20, 20, 40, 255});
        
        // Renderizar layers (com paralax automático)
        layerSystem.render();
        
        // Debug UI
        DrawRectangle(10, 50, 300, 150, ColorAlpha(BLACK, 0.7f));
        DrawText(TextFormat("Player: (%.0f, %.0f)", player->graphic->x, player->graphic->y), 20, 60, 12, WHITE);
        DrawText(TextFormat("Entities: %zu", entityManager.getEntityCount()), 20, 80, 12, GREEN);
        DrawText(TextFormat("Physics Bodies: %d", physicsWorld.getBodyCount()), 20, 100, 12, GREEN);
        DrawText(TextFormat("FPS: %d", GetFPS()), 20, 120, 12, GREEN);
        
        EndDrawing();
    }
    
    // Cleanup
    entityManager.cleanup();
    CloseWindow();
    
    return 0;
}
```

## Fluxo de Dados por Frame

```
Input (Keyboard)
  ↓
Update Física (physicsWorld.step())
  ↓
Sincronizar Graphics ← Physics
  ↓
Update Câmara (segue player)
  ↓
Render LayerSystem
  ├─ Layer 0-3: Backgrounds tiling
  ├─ Layer 4: Game objects com paralax
  └─ Layer 5: GUI sem paralax
  ↓
Display na Tela
```


## Exemplo: Player com Animação

```cpp
class PlayerEntity : public GameEntity
{
public:
    float animTimer;
    int animFrame;
    
    PlayerEntity(int id) : GameEntity(id, PLAYER), animTimer(0), animFrame(0) {}
    
    void update(float dt) override
    {
        // Atualizar animação
        animTimer += dt;
        if (animTimer > 0.1f) {
            animTimer = 0;
            animFrame = (animFrame + 1) % 4;
        }
        
        // Atualizar clip de atlas
        // (Se tiver atlas de animação)
        // graphic->loadAtlas(Rect(animFrame * 32, 0, 32, 32));
    }
};

// Uso:
// PlayerEntity* player = new PlayerEntity(0);
// player->graphic->loadFile("assets/player_anim.png");
// layers.addGraphToLayer(4, player->graphic);
```


## Exemplo: Projectil (Criação Dinâmica)

```cpp
void fireProjectile(GameEntityManager* manager, Vec2 pos, Vec2 dir)
{
    GameEntity* proj = manager->createEntity(GameEntity::PROJECTILE, pos.x, pos.y);
    proj->graphic->loadFile("assets/projectile.png");
    
    // Adicionar velocidade
    proj->physicsBody->velocity = dir * 10.0f;
    
    // Destruir após 5 segundos
    // (Implementar num sistema de lifetime)
}
```


## Com Joints (Constraints)

```cpp
// Criar uma corrente de plataformas
for (int i = 0; i < 5; i++)
{
    GameEntity* platform = entityManager.createEntity(GameEntity::PLATFORM, 100 + i * 150, 400);
    
    if (i > 0) {
        GameEntity* prev = entityManager.getEntity(i - 1);
        
        // Conectar com distance joint
        auto joint = physicsWorld.createDistanceJoint(
            prev->physicsBody,
            platform->physicsBody,
            Physics2D::Vec2(100 + (i - 0.5f) * 150, 400),
            Physics2D::Vec2(100 + (i + 0.5f) * 150, 400)
        );
        joint->stiffness = 0.8f;
    }
}
```


## Estrutura de Ficheiros Recomendada

```
Projeto/
├─ src/
│  ├─ main.cpp
│  ├─ Graph.cpp
│  ├─ Physics2D.cpp
│  ├─ LayerSystem_Graph.cpp
│  └─ GameEntity.cpp
├─ include/
│  ├─ Graph.h
│  ├─ Physics2D.h
│  ├─ LayerSystem_Graph.h
│  └─ GameEntity.h
├─ assets/
│  ├─ sky.png
│  ├─ ground.png
│  ├─ player.png
│  ├─ enemy.png
│  └─ ...
└─ Makefile
```

## Makefile Exemplo

```makefile
CXX = g++
CXXFLAGS = -std=c++17 -I./include
LIBS = -lraylib -lm

SOURCES = src/main.cpp src/Graph.cpp src/Physics2D.cpp src/LayerSystem_Graph.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = game

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean
```

## Performance Tips

1. **Usar Object Pooling para Projectis**
   ```cpp
   std::vector<GameEntity*> projectilePool;
   // Reciclar em vez de criar/destruir
   ```

2. **Batch Rendering**
   ```cpp
   // LayerSystem já faz isto!
   // (Renderiza uma layer de cada vez)
   ```

3. **Spatial Partitioning**
   ```cpp
   // Physics2D usa quadtree
   // Colisões são O(n log n)
   ```

4. **Frustum Culling**
   ```cpp
   // Não renderizar objetos fora da câmara
   // (Opcional, mas bom para performance)
   ```

---

**Sistema integrado e pronto para desenvolvimento!** 🚀🎮

