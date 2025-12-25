#include "instances.hpp"
#include "value.hpp"
#include "arena.hpp"
#include "interpreter.hpp"
#include <ctype.h>
#include <new>

InstancePool::InstancePool()
{
}

InstancePool::~InstancePool()
{
}

StructInstance *InstancePool::createStruct( )
{
    void *mem = (StructInstance *)arena.Allocate(sizeof(StructInstance)); // 40kb
    StructInstance *instance = new (mem) StructInstance();
    return instance;
}

void InstancePool::freeStruct(StructInstance *s)
{
    s->~StructInstance();
    arena.Free(s, sizeof(StructInstance));
}
ArrayInstance *InstancePool::createArray()
{
    void *mem = (ArrayInstance *)arena.Allocate(sizeof(ArrayInstance)); // 32kb
    ArrayInstance *instance = new (mem) ArrayInstance();
   // Info("array size %ld",sizeof(ArrayInstance));
    return instance;
}

void InstancePool::freeArray(ArrayInstance *a)
{
 
    a->~ArrayInstance();
    arena.Free(a, sizeof(ArrayInstance));
}

MapInstance *InstancePool::createMap()
{
    void *mem = (MapInstance *)arena.Allocate(sizeof(MapInstance)); // 40kb
    MapInstance *instance = new (mem) MapInstance();
    // Info("map size %ld",sizeof(MapInstance));
    return instance;
}

void InstancePool::freeMap(MapInstance *m)
{
    m->~MapInstance();
    arena.Free(m, sizeof(MapInstance));
}

ClassInstance *InstancePool::creatClass()
{
    void *mem = (MapInstance *)arena.Allocate(sizeof(ClassInstance)); // 40kb
    ClassInstance *instance = new (mem) ClassInstance();
    //Info("class size %ld", sizeof(ClassInstance));
    return instance;
}

void InstancePool::freeClass(ClassInstance *c)
{
    c->~ClassInstance();
    arena.Free(c, sizeof(ClassInstance));
}

void InstancePool::clear()
{
    arena.Stats();
    arena.Clear();
}
