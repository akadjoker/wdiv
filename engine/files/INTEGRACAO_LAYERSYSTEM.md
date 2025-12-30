# 🔗 Integração: LayerSystem + Physics2D + Graph

Guia completo para integrar o **LayerSystem**, **Physics2D** e **Graph** num projeto coerente.

## Arquitetura do Sistema

```
┌─────────────────────────────────────┐
│         LayerSystem                 │
│  (6 layers com paralax e tiling)    │
├─────────────────────────────────────┤
│                                     │
│  Layer 0-3: Backgrounds (tiling)   │
│  Layer 4: GameObjects + Physics    │
│  Layer 5: GUI                      │
│                                     │
└──────────────────┬──────────────────┘
                   │
        ┌──────────┴──────────┐
        │                     │
   ┌────▼─────┐         ┌─────▼────┐
   │  Graph   │         │Physics2D │
   │  (Visual)│         │(Physics) │
   └──────────┘         └──────────┘
```

## Componentes

### Graph (Visual)
- Textura e posição
- Rotação e escala
- Pontos de attachment
- Hierarchia (parent)

### Physics2D (Lógica)
- Corpos rígidos
- Colisões
- Constraints (joints)
- Simulação

### LayerSystem (Organização)
- 6 layers independentes
- Paralax automático
- Tiling de backgrounds
- Câmara sincronizada


## Game Object Pattern

### Classe GameEntity

```cpp
#pragma once

#include "Graph.h"
#include "Physics2D.h"
#include "LayerSystem.h"

class GameEntity
{
public:
    // Visual
    Graph* graphic;
    
    // Physics
    Physics2D::Body* physicsBody;
    
    // Identificação
    int id;
    int layerIndex;
    bool active;
    
    // Tipo
    enum Type {
        PLAYER,
        ENEMY,
        PROJECTILE,
        COLLECTIBLE,
        PLATFORM
    } type;
    
    GameEntity(int id, Type t)
        : id(id), type(t), graphic(nullptr), 
          physicsBody(nullptr), layerIndex(4), active(true)
    {}
    
    virtual ~GameEntity()
    {
        // Cleanup feito pelo manager
    }
    
    // Virtual para override em subclasses
    virtual void update(float dt) {}
    virtual void render() {}
    
    // Sincronizar graphics <-> physics
    void syncGraphicsToPhysics()
    {
        if (graphic && physicsBody)
        {
            graphic->setPosition(physicsBody->position.x, physicsBody->position.y);
            graphic->setRotation(physicsBody->orient);
        }
    }
    
    void syncPhysicsToGraphics()
    {
        if (graphic && physicsBody)
        {
            physicsBody->position.x = graphic->x;
            physicsBody->position.y = graphic->y;
        }
    }
};
```

### Manager de GameEntities

```cpp
class GameEntityManager
{
private:
    Vector<GameEntity*> entities;
    LayerSystem* layerSystem;
    Physics2D::World* physics;
    GraphManager* graphManager;
    int nextId;
    
public:
    GameEntityManager(LayerSystem* ls, Physics2D::World* ph, GraphManager* gm)
        : layerSystem(ls), physics(ph), graphManager(gm), nextId(0)
    {}
    
    ~GameEntityManager()
    {
        cleanup();
    }
    
    // Criar entidade
    GameEntity* createEntity(GameEntity::Type type)
    {
        GameEntity* entity = new GameEntity(nextId++, type);
        
        // Criar gráfico
        entity->graphic = graphManager->create();
        
        // Criar corpo físico
        entity->physicsBody = physics->createCircle(Vec2(640, 360), 10, 1.0f);
        
        // Adicionar à layer
        layerSystem->addGraphToLayer(entity->layerIndex, entity->graphic);
        
        entities.push(entity);
        return entity;
    }
    
    // Remover entidade
    void removeEntity(GameEntity* entity)
    {
        if (!entity) return;
        
        // Remover do render
        layerSystem->removeGraphFromLayer(entity->layerIndex, entity->graphic);
        
        // Remover da física
        physics->destroyBody(entity->physicsBody);
        
        // Remover da lista
        auto it = std::find(entities.begin(), entities.end(), entity);
        if (it != entities.end())
        {
            entities.erase(it);
        }
        
        delete entity;
    }
    
    // Update
    void update(float dt)
    {
        for (size_t i = 0; i < entities.size(); i++)
        {
            GameEntity* entity = entities[i];
            if (entity->active)
            {
                entity->update(dt);
                entity->syncGraphicsToPhysics();
            }
        }
    }
    
    // Render
    void render()
    {
        // LayerSystem já faz o render
    }
    
    // Query
    GameEntity* getEntityById(int id)
    {
        for (size_t i = 0; i < entities.size(); i++)
        {
            if (entities[i]->id == id)
                return entities[i];
        }
        return nullptr;
    }
    
    void cleanup()
    {
        for (size_t i = 0; i < entities.size(); i++)
        {
            delete entities[i];
        }
        entities.clear();
    }
};
```


## Exemplo de Uso Completo

```cpp
#include "raylib.h"
#include "LayerSystem.h"
#include "Physics2D.h"
#include "Graph.h"
#include "GameEntity.h"

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Integrated Game");
    SetTargetFPS(60);
    
    // Criar managers
    GraphManager graphManager;
    Physics2D::World physicsWorld;
    physicsWorld.setGravity(0, 9.8f);
    
    LayerSystem layerSystem(SCREEN_WIDTH, SCREEN_HEIGHT);
    GameEntityManager entityManager(&layerSystem, &physicsWorld, &graphManager);
    
    // Setup layers com backgrounds
    Graph* skyTile = graphManager.create();
    skyTile->loadFile("assets/sky.png");
    layerSystem.setLayerBackground(0, skyTile, 32, 32);
    layerSystem.setLayerScrollFactor(0, 0.2f, 0.2f);
    
    Graph* groundTile = graphManager.create();
    groundTile->loadFile("assets/ground.png");
    layerSystem.setLayerBackground(3, groundTile, 64, 32);
    layerSystem.setLayerScrollFactor(3, 0.9f, 0.9f);
    
    // Criar chão estático (physics)
    Physics2D::Body* ground = physicsWorld.createRectangle(
        Physics2D::Vec2(640, 650), 1280, 50, 1000000.0f
    );
    ground->inverseMass = 0.0f;  // Estático
    
    // Criar player
    GameEntity* player = entityManager.createEntity(GameEntity::PLAYER);
    player->graphic->loadFile("assets/player.png");
    player->graphic->setPosition(640, 400);
    player->physicsBody->position = Physics2D::Vec2(640, 400);
    
    // Criar alguns inimigos
    for (int i = 0; i < 3; i++)
    {
        GameEntity* enemy = entityManager.createEntity(GameEntity::ENEMY);
        enemy->graphic->loadFile("assets/enemy.png");
        enemy->graphic->setPosition(400 + i * 200, 450);
        enemy->physicsBody->position = Physics2D::Vec2(400 + i * 200, 450);
    }
    
    // Variáveis de controlo
    Vec2 playerScreenPos(640, 400);
    float cameraX = 640;
    float cameraY = 360;
    
    // ===== LOOP PRINCIPAL =====
    
    while (!WindowShouldClose())
    {
        // ===== INPUT =====
        
        if (IsKeyDown(KEY_LEFT))
            player->physicsBody->addForce(Physics2D::Vec2(-5, 0));
        if (IsKeyDown(KEY_RIGHT))
            player->physicsBody->addForce(Physics2D::Vec2(5, 0));
        if (IsKeyPressed(KEY_SPACE))
            player->physicsBody->addForce(Physics2D::Vec2(0, -100));
        
        // ===== UPDATE PHYSICS =====
        
        physicsWorld.step();  // Simular física
        
        // ===== UPDATE ENTITIES =====
        
        entityManager.update(1.0f / 60.0f);
        
        // ===== UPDATE CAMERA =====
        
        // Câmara segue player
        cameraX += (player->physicsBody->position.x - cameraX) * 0.1f;
        cameraY += (player->physicsBody->position.y - cameraY) * 0.1f;
        layerSystem.setMainCamera(Physics2D::Vec2(cameraX, cameraY), 1.0f);
        
        // ===== RENDER =====
        
        BeginDrawing();
        ClearBackground(RAYBLACK);
        
        // Render todas as layers
        layerSystem.render();
        
        // ===== UI =====
        
        DrawRectangle(10, 10, 400, 150, ColorAlpha(BLACK, 0.7f));
        DrawText("INTEGRATED GAME", 20, 20, 16, YELLOW);
        DrawText("Physics + Graph + Layers", 20, 40, 12, LIGHTGRAY);
        DrawText("ARROWS - Move | SPACE - Jump", 20, 60, 11, WHITE);
        DrawText(TextFormat("Entities: %d", entityManager.getEntityCount()), 
                 20, 80, 11, YELLOW);
        DrawText(TextFormat("FPS: %d", GetFPS()), 20, 100, 11, GREEN);
        DrawText(TextFormat("Physics Bodies: %d", physicsWorld.getBodyCount()), 
                 20, 120, 11, GREEN);
        
        EndDrawing();
    }
    
    // Cleanup
    entityManager.cleanup();
    CloseWindow();
    
    return 0;
}
```

## Fluxo de Dados

```
Input (Keyboard)
  ↓
Update Física (physicsWorld.step())
  ↓
Sync Graphics ← Physics (posição, rotação)
  ↓
Update Entidades (custom logic)
  ↓
Update Câmara (segue player)
  ↓
Render (layerSystem.render())
  ↓
Display na Tela
```

## Integração com Joints

```cpp
// Criar um joint entre player e uma caixa
Physics2D::RevoluteJoint* joint = 
    physicsWorld.createRevoluteJoint(
        player->physicsBody,
        box->physicsBody,
        Physics2D::Vec2(player->physicsBody->position.x, 
                        player->physicsBody->position.y)
    );

// Adicionar limites
joint->setLimits(-3.14f/4, 3.14f/4);

// O joint é renderizado automaticamente se chamar
// physics.render() ou se tiver debug mode
```

## Dicas de Integração

### 1. Sincronização Automática

```cpp
// Sempre após physicsWorld.step():
for (auto entity : entities) {
    entity->syncGraphicsToPhysics();
}
```

### 2. Collision Callbacks

```cpp
// Detecção simples
for (int i = 0; i < physicsWorld.getBodyCount(); i++) {
    Physics2D::Body* body = physicsWorld.getBody(i);
    
    for (int j = i + 1; j < physicsWorld.getBodyCount(); j++) {
        Physics2D::Body* other = physicsWorld.getBody(j);
        
        // Verificar colisão (usar AABB simples)
        if (checkCollision(body, other)) {
            onCollision(body, other);
        }
    }
}
```

### 3. Layer Assignment

```cpp
// Sempre colocar objetos dinâmicos na layer 4
entity->layerIndex = 4;
layerSystem.addGraphToLayer(4, entity->graphic);

// Platforms podem estar em layer 3
platform->layerIndex = 3;
layerSystem.addGraphToLayer(3, platform->graphic);
```

### 4. Performance

```cpp
// Cache references
GameEntity* player = entityManager.getEntityById(0);
Layer* gameLayer = layerSystem.getLayer(4);

// Usar object pooling para projecteis
Vector<GameEntity*> projectilePool;

// Reciclar em vez de criar/destruir
void fireProjectile(Vec2 pos, Vec2 dir) {
    GameEntity* proj = projectilePool[poolIndex];
    proj->graphic->setPosition(pos.x, pos.y);
    proj->physicsBody->position = Physics2D::Vec2(pos.x, pos.y);
    proj->active = true;
}
```


## Estrutura de Ficheiros

```
Projeto/
├─ Headers/
│  ├─ Graph.h
│  ├─ Physics2D.h
│  ├─ LayerSystem.h
│  └─ GameEntity.h
├─ Source/
│  ├─ Graph.cpp
│  ├─ Physics2D.cpp
│  ├─ LayerSystem.cpp
│  └─ main.cpp
└─ assets/
   ├─ sky.png
   ├─ ground.png
   ├─ player.png
   └─ enemy.png
```

## Compilação

```bash
g++ -std=c++17 source/*.cpp -o game -I./headers -lraylib -lm
```

---

**Sistema integrado, pronto para desenvolvimento!** 🎮🚀

