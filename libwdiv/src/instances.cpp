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

StructInstance *InstancePool::createStruct(String *name)
{
    void *mem = (StructInstance *)arena.Allocate(sizeof(StructInstance));//40kb
    StructInstance *instance = new (mem) StructInstance();
    return instance;
}

void InstancePool::freeStruct(StructInstance *s)
{
    s->~StructInstance();
    arena.Free(s,sizeof(StructInstance));
}
ArrayInstance *InstancePool::createArray()
{
    void *mem = (ArrayInstance *)arena.Allocate(sizeof(ArrayInstance));//40kb
    ArrayInstance *instance = new (mem) ArrayInstance();
    return instance;
}

void InstancePool::freeArray(ArrayInstance* a)
{
    a->~ArrayInstance();
    arena.Free(a,sizeof(ArrayInstance));
}

MapInstance *InstancePool::createMap()
{
    void *mem = (MapInstance *)arena.Allocate(sizeof(MapInstance));//40kb
    MapInstance *instance = new (mem) MapInstance();
    return instance;
}

void InstancePool::freeMap(MapInstance *m)
{
    m->~MapInstance();
    arena.Free(m,sizeof(MapInstance));
}

void InstancePool::clear()
{
    arena.Stats();
    arena.Clear();
}
