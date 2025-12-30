#include "instances.hpp"
#include "value.hpp"
#include "arena.hpp"
#include "interpreter.hpp"
#include <ctype.h>
#include <new>

InstancePool::InstancePool()
{
    classesInstances.reserve(1024);
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
    m->table.destroy();
    m->~MapInstance();
    arena.Free(m, sizeof(MapInstance));
}

ClassInstance *InstancePool::creatClass()
{
    void *mem = (MapInstance *)arena.Allocate(sizeof(ClassInstance)); // 40kb
    ClassInstance *instance = new (mem) ClassInstance();

    instance->index = classesInstances.size();
    classesInstances.push_back(instance);
    //Info("class size %ld", sizeof(ClassInstance));
    return instance;
}

void InstancePool::freeClass(ClassInstance *c)
{
    c->~ClassInstance();
    arena.Free(c, sizeof(ClassInstance));
}

NativeInstance *InstancePool::createNativeClass()
{
    void *mem = (NativeInstance *)arena.Allocate(sizeof(NativeInstance)); // 32kb
    NativeInstance *instance = new (mem) NativeInstance();
//    Info(" Create of class instance %ld ", sizeof(NativeInstance));
    return instance;
}

void InstancePool::freeNativeClass(NativeInstance *n)
{
   // Info("Fre of class instance %s ", n->klass->name->chars());
    n->~NativeInstance();
    arena.Free(n, sizeof(NativeInstance));
}

NativeStructInstance *InstancePool::createNativeStruct()
{
    void *mem = (NativeStructInstance *)arena.Allocate(sizeof(NativeStructInstance)); // 32kb
    NativeStructInstance *instance = new (mem) NativeStructInstance();
   // Info(" Create of struct instance %ld ", sizeof(NativeStructInstance));
    return instance;
}

void InstancePool::freeNativeStruct(NativeStructInstance *n)
{
 //   Info("Fre of struct instance %s ", n->def->name->chars());
    n->~NativeStructInstance();
    arena.Free(n, sizeof(NativeStructInstance));
}

ClassInstance *InstancePool::getClass(int index)
{
    if (index >= classesInstances.size())
        return nullptr;
    return  classesInstances[index];
}

void InstancePool::clear()
{

    for (size_t i = 0; i < classesInstances.size(); i++)
    {
        ClassInstance *a = classesInstances[i];
        InstancePool::instance().freeClass(a);
    }
    classesInstances.clear();
    Info("Instance pool stats:");
   // arena.Stats();
    arena.Clear();
}
