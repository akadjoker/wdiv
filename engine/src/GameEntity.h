#pragma once

#include "Graph.h"
#include "Physics2D.h"
#include "LayerSystem.h"

class GameEntity
{
public:
    // Visual (Graph)
    Graph* graphic;
    
 
    
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
            layerIndex(4), active(true)
    {}
    
    virtual ~GameEntity() {}
    
    // Sincronizar Physics -> Graphics
    void updateFromPhysics()
    {
         
    }
    
    // Sincronizar Graphics -> Physics
    void updateToPhysics()
    {
        
    }
    
    virtual void update(float dt) {}
};