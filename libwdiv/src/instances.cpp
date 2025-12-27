#include "instances.hpp"
#include "value.hpp"
#include "arena.hpp"
#include "interpreter.hpp"
#include <ctype.h>
#include <new>

void InstancePool::setInterpreter(Interpreter *interpreter)
{
    this->interpreter = interpreter;
}

Interpreter *InstancePool::getInterpreter()
{
    return interpreter;
}

InstancePool::InstancePool()
{
}

InstancePool::~InstancePool()
{
    // Info("Instance pool released");
}

StructInstance *InstancePool::createStruct()
{
    void *mem = (StructInstance *)arena.Allocate(sizeof(StructInstance)); // 40kb
    StructInstance *instance = new (mem) StructInstance();
    instance->refCount = 1;
    structInstances.push(instance);
    return instance;
}

void InstancePool::freeStruct(StructInstance *s)
{
    s->~StructInstance();
    arena.Free(s, sizeof(StructInstance));
}
ArrayInstance *InstancePool::createArray(int reserve)
{
    void *mem = (ArrayInstance *)arena.Allocate(sizeof(ArrayInstance)); // 32kb
    ArrayInstance *instance = new (mem) ArrayInstance();
    if (reserve != 0)
        instance->values.reserve(reserve);
    arrayInstances.push(instance);
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
   
    void *mem = (ClassInstance *)arena.Allocate(sizeof(ClassInstance)); // 40kb
    ClassInstance *instance = new (mem) ClassInstance();
    classesInstances.push(instance);
    // Info("class size %ld", sizeof(ClassInstance));
    return instance;
}

void InstancePool::freeClass(ClassInstance *c)
{
    if (!c)
        return;
    c->~ClassInstance();
    arena.Free(c, sizeof(ClassInstance));
}

NativeInstance *InstancePool::createNativeClass()
{
    void *mem = (NativeInstance *)arena.Allocate(sizeof(NativeInstance)); // 32kb
    NativeInstance *instance = new (mem) NativeInstance();
    // Info(" Create of class instance %ld ", sizeof(NativeInstance));
    // nativeInstances.push(instance);
    return instance;
}

void InstancePool::freeNativeClass(NativeInstance *n)
{
    // Info("Fre of class instance %s ", n->klass->name->chars());

    if (n->klass->destructor)
    {
        n->klass->destructor(this->interpreter, n->userData);
    }

    n->~NativeInstance();
    arena.Free(n, sizeof(NativeInstance));
}

NativeStructInstance *InstancePool::createNativeStruct(uint32 structSize)
{
    //   Info(" Create of struct instance %ld size %ld", nativeStructInstances.size( ), structSize);
    void *mem = (NativeStructInstance *)arena.Allocate(sizeof(NativeStructInstance));
    NativeStructInstance *instance = new (mem) NativeStructInstance();
    instance->data = arena.Allocate(structSize);
    std::memset(instance->data, 0, structSize);
    // nativeStructInstances.push(instance);
    return instance;
}

void InstancePool::freeNativeStruct(NativeStructInstance *n)
{
    // Info("Fre of struct instance %s size %ld", n->def->name->chars(), n->def->structSize);

    if (n->data)
    {
        if (n->def->destructor)
        {
            n->def->destructor(interpreter, n->data);
        }

        arena.Free(n->data, n->def->structSize);
        n->data = nullptr;
    }
    n->~NativeStructInstance();
    arena.Free(n, sizeof(NativeStructInstance));
}

void InstancePool::clear()
{
    Info("Instance pool clear");

    for (size_t i = 0; i < structInstances.size(); i++)
    {
        StructInstance *s = structInstances[i];
        if (!s)
            continue;
        if (s->refCount == 1)
        {
            s->release();
        }
        else
        {
            Warning("Struct instance '%s' has refCount = %d (still in use)\n", s->def->name->chars(), s->refCount);
        }
    }
    structInstances.clear();

    for (size_t i = 0; i < arrayInstances.size(); i++)
    {
        ArrayInstance *a = arrayInstances[i];
        freeArray(a);
    }
    arrayInstances.clear();

    for (size_t i = 0; i < classesInstances.size(); i++)
    {
        ClassInstance *s = classesInstances[i];
        if (!s)
            continue;
        if (s->refCount == 1)
        {
            s->release();
        }
        else
        {
            Warning("Class instance '%s' has refCount = %d (still in use)", s->klass->name->chars(), s->refCount);
        }
    }
    classesInstances.clear();

    // Warning("Freeing %d native class instances", nativeInstances.size());

    for (size_t i = 0; i < nativeInstances.size(); i++)
    {
        // NativeInstance *s = nativeInstances[i];

        // if (!s)
        //     continue;
        // if (s->refCount == 1)
        // {
        //     s->release(); // Chama drop() → deallocString()
        // }
        // else
        // {
        //     Warning("Native class instance '%s' has refCount = %d (still in use)\n", s->klass->name->chars(), s->refCount);

        // }
    }
    nativeInstances.clear();

    // for (size_t i = 0; i < nativeStructInstances.size(); i++)
    // {
    //     NativeStructInstance *a = nativeStructInstances[i];
    //     freeNativeStruct(a);
    // }

    // Warning("Freeing %d native struct instances", nativeStructInstances.size());
    // for (size_t j = 0; j < nativeStructInstances.size(); j++)
    // {
    //     NativeStructInstance *s = nativeStructInstances[j];
    //     if (!s)
    //         continue;
    //     if (s->refCount == 1)
    //     {
    //         s->release(); // Chama drop() → deallocString()
    //     }
    //     else
    //     {
    //         Warning("Native struct instance '%s' has refCount = %d (still in use)\n", s->def->name->chars(), s->refCount);

    //     }
    // }

    nativeStructInstances.clear();

    arena.Stats();
    arena.Clear();
}
