#include "GameEntity.h"
#include "LayerSystem.h"
#include <algorithm>
#include <cmath>

class GameEntityManager

{
private:
    std::vector<GameEntity *> entities;
    LayerSystem *layerSystem;

    GraphManager *graphManager;
    int nextId;

public:
    GameEntityManager(LayerSystem *ls, GraphManager *gm)
        : layerSystem(ls), graphManager(gm), nextId(0)
    {
    }

    ~GameEntityManager()
    {
        cleanup();
    }

    // Criar entidade
    GameEntity *createEntity(GameEntity::Type type, float x, float y)
    {
        GameEntity *entity = new GameEntity(nextId++, type);

        // Criar gráfico
        entity->graphic = graphManager->create();

        // Adicionar à layer
        if (type == GameEntity::COLLECTIBLE)
        {
            entity->layerIndex = 4;
        }
        layerSystem->addGraphToLayer(entity->layerIndex, entity->graphic);

        entities.push_back(entity);
        return entity;
    }

    // Remover entidade
    void removeEntity(GameEntity *entity)
    {
        if (!entity)
            return;

        // Remover visual
        layerSystem->removeGraphFromLayer(entity->layerIndex, entity->graphic);

        // Remover da lista
        auto it = std::find(entities.begin(), entities.end(), entity);
        if (it != entities.end())
        {
            entities.erase(it);
        }

        delete entity;
    }

    // Update todos
    void update(float dt)
    {

        // 2. Sincronizar
        for (auto entity : entities)
        {
            if (entity->active)
            {
                entity->updateFromPhysics();
                entity->update(dt);
            }
        }
    }

    void cleanup()
    {
        for (auto entity : entities)
        {
            delete entity;
        }
        entities.clear();
    }

    size_t getEntityCount() const { return entities.size(); }
    GameEntity *getEntity(int id)
    {
        for (auto e : entities)
        {
            if (e->id == id)
                return e;
        }
        return nullptr;
    }
};