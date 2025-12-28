#include "instances.hpp"
#include "value.hpp"
#include "arena.hpp"
#include "interpreter.hpp"
#include <ctype.h>
#include <math.h>
#include <new>
#include "pool.hpp"
#include "interpreter.hpp"
#include "value.hpp"

#define USE_ARENA 0

void InstancePool::setInterpreter(Interpreter *interpreter)
{
    this->interpreter = interpreter;
}

Interpreter *InstancePool::getInterpreter()
{
    return interpreter;
}

// void InstancePool::markValue(const Value &v)
// {

//      if (v.type == ValueType::STRING && v.as.string)
//      {
//         Info("Marking string %s", v.as.string->chars());
//         v.as.string->marked = true;
//         strings.push(v.as.string);
//      }
//      else if (v.type == ValueType::CLASSINSTANCE && v.as.sClass)
//         markObject((GCObject *)v.as.sClass);
//     else if (v.type == ValueType::ARRAY && v.as.array)
//         markObject((GCObject *)v.as.array);
//     else if (v.type == ValueType::MAP && v.as.map)
//         markObject((GCObject *)v.as.map);
//     else if (v.type == ValueType::STRUCTINSTANCE && v.as.sInstance)
//         markObject((GCObject *)v.as.sInstance);
//     else if (v.type == ValueType::NATIVESTRUCTINSTANCE && v.as.sNativeStruct)
//         markObject((GCObject *)v.as.sNativeStruct);
//     else if (v.type == ValueType::NATIVECLASSINSTANCE && v.as.sClassInstance)
//         markObject((GCObject *)v.as.sClassInstance);

// }

InstancePool::InstancePool() : interpreter(nullptr), bytesAllocated(0)
{

    totalStructs = 0;
    totalArrays = 0;
    totalMaps = 0;
    totalClasses = 0;
    totalNativeStructs = 0;
    totalNativeClasses = 0;

    structSize = sizeof(StructInstance);
    arraySize = sizeof(ArrayInstance);
    mapSize = sizeof(MapInstance);
    classSize = sizeof(ClassInstance);
    nativeStructSize = sizeof(NativeStructInstance);
    nativeClassSize = sizeof(NativeInstance);

    dummyDefStruct = new StructDef();
    dummyDefStruct->argCount = 0;
    dummyDefStruct->name = createString("dummy");
    dummyDefStruct->names.destroy();

    dummyStructInstance = createStruct();
    dummyStructInstance->def = dummyDefStruct;

    dummyDefClass = new ClassDef();
    dummyDefClass->name = createString("dummy");
    dummyDefClass->fieldCount = 0;
    dummyDefClass->parent = nullptr;
    dummyDefClass->inherited = false;

    dummyClassInstance = createClass();
    dummyClassInstance->klass = dummyDefClass;
    dummyClassInstance->fields.destroy();

    dummyArrayInstance = createArray(0);
    dummyMapInstance = createMap();
    dummyMapInstance->table.destroy();
}

InstancePool::~InstancePool()
{
}

// ============= CREATE METHODS =============

StructInstance *InstancePool::getStruct(int id)
{
    // if (id < 0 || id >= (int)structInstances.size())
    // {
    //     Warning("StructInstance index out of bounds: %d", id);
    //     return dummyStructInstance;
    // }

    return structInstances[id];
}

ClassInstance *InstancePool::getClass(int id)
{
    // if (id < 0 || id >= (int)classesInstances.size())
    // {
    //     Warning("ClassInstance index out of bounds: %d", id);
    //     return dummyClassInstance;
    // }

    return classesInstances[id];
}

ArrayInstance *InstancePool::getArray(int id)
{
    // if (id < 0 || id >= (int)arrayInstances.size())
    // {
    //     Warning("ArrayInstance index out of bounds: %d", id);
    //     return dummyArrayInstance;
    // }

    return arrayInstances[id];
}

MapInstance *InstancePool::getMap(int id)
{
    if (id < 0 || id >= (int)mapInstances.size())
    {
        Warning("MapInstance index out of bounds: %d", id);
        return dummyMapInstance;
    }

    return mapInstances[id];
}

StructInstance *InstancePool::createStruct()
{
    StructInstance *instance = nullptr;

#if USE_ARENA
    void *mem = arena.Allocate(sizeof(StructInstance));
    instance = new (mem) StructInstance();
#else
    instance = new StructInstance();
#endif

    instance->index = structInstances.size();
    structInstances.push(instance);
    bytesAllocated += sizeof(StructInstance);
    totalStructs++;
    return instance;
}

ArrayInstance *InstancePool::createArray(int reserve)
{
    ArrayInstance *instance = nullptr;

#if USE_ARENA

    void *mem = arena.Allocate(sizeof(ArrayInstance));
    instance = new (mem) ArrayInstance();
#else
    instance = new ArrayInstance();
#endif

    if (reserve != 0)
        instance->values.reserve(reserve);

    arrayInstances.push(instance);

    bytesAllocated += arraySize;

    totalArrays++;

    return instance;
}

MapInstance *InstancePool::createMap()
{
    MapInstance *instance = nullptr;
#if USE_ARENA
    void *mem = arena.Allocate(sizeof(MapInstance));
    instance = new (mem) MapInstance();
#else
    instance = new MapInstance();
#endif
    instance->index = mapInstances.size();
    mapInstances.push(instance);
    bytesAllocated += mapSize;

    totalMaps++;
    return instance;
}

ClassInstance *InstancePool::createClass()
{

    ClassInstance *instance = nullptr;

#if USE_ARENA
    void *mem = arena.Allocate(sizeof(ClassInstance));
    instance = new (mem) ClassInstance();
#else
    instance = new ClassInstance();
#endif

    instance->index = classesInstances.size();
    classesInstances.push(instance);

    bytesAllocated += classSize;

    totalClasses++;
    return instance;
}

NativeInstance *InstancePool::createNativeClass()
{
    NativeInstance *instance = nullptr;

#if USE_ARENA
    void *mem = arena.Allocate(sizeof(NativeInstance));
    instance = new (mem) NativeInstance();
#else
    instance = new NativeInstance();
#endif
    nativeInstances.push(instance);

    bytesAllocated += nativeClassSize;

    totalNativeClasses++;

    return instance;
}

NativeStructInstance *InstancePool::createNativeStruct(uint32 structSize)
{
    NativeStructInstance *instance = nullptr;

#if USE_ARENA
    void *mem = arena.Allocate(nativeStructSize);
    instance = new (mem) NativeStructInstance();
#else
    instance = new NativeStructInstance();
#endif

    instance->data = aAlloc(structSize);
    std::memset(instance->data, 0, structSize);
    nativeStructInstances.push(instance);

    bytesAllocated += nativeStructSize + structSize;

    totalNativeStructs++;
    return instance;
}

// ============= FREE METHODS =============

void InstancePool::freeStruct(StructInstance *s)
{
    if (!s)
        return;

#if USE_ARENA
    s->~StructInstance();
    arena.Free(s, structSize);
#else
    delete s;
#endif
    bytesAllocated -= structSize;
    totalStructs--;
}

void InstancePool::freeArray(ArrayInstance *a)
{
    if (!a)
        return;

#if USE_ARENA
    a->~ArrayInstance();
    arena.Free(a, arraySize);
#else
    delete a;
#endif

    bytesAllocated -= arraySize;

    totalArrays--;
}

void InstancePool::freeMap(MapInstance *m)
{
    if (!m)
        return;

#if USE_ARENA
    m->~MapInstance();
    arena.Free(m, mapSize);
#else
    delete m;
#endif

    bytesAllocated -= mapSize;

    totalMaps--;
}

void InstancePool::freeClass(ClassInstance *c)
{
    if (!c)
        return;

#if USE_ARENA

    c->~ClassInstance();
    arena.Free(c, classSize);
#else
    delete c;
#endif

    bytesAllocated -= classSize;

    totalClasses--;
}

void InstancePool::freeNativeClass(NativeInstance *n)
{
    if (!n)
        return;

    if (n->klass && n->klass->destructor)
    {
        n->klass->destructor(this->interpreter, n->userData);
    }

#if USE_ARENA

    n->~NativeInstance();
    arena.Free(n, nativeClassSize);

#else
    delete n;
#endif

    bytesAllocated -= nativeClassSize;

    totalNativeClasses--;
}

void InstancePool::freeNativeStruct(NativeStructInstance *n)
{
    if (!n)
        return;

    if (n->data)
    {
        if (n->def && n->def->destructor)
        {
            n->def->destructor(interpreter, n->data);
        }
        aFree(n->data);
        n->data = nullptr;
    }

#if USE_ARENA
    n->~NativeStructInstance();
    arena.Free(n, nativeStructSize);
#else
    delete n;
#endif

    bytesAllocated -= nativeStructSize;

    totalNativeStructs--;
}

// ============= CLEANUP =============

void InstancePool::clear()
{
    Info("Instance pool clear");

    delete dummyDefStruct;

    for (size_t i = 0; i < structInstances.size(); i++)
    {
        StructInstance *s = structInstances[i];
        freeStruct(s);
    }
    structInstances.clear();

    for (size_t i = 0; i < arrayInstances.size(); i++)
    {
        ArrayInstance *a = arrayInstances[i];
        freeArray(a);
    }
    arrayInstances.clear();

    for (size_t i = 0; i < mapInstances.size(); i++)
    {
        MapInstance *m = mapInstances[i];
        freeMap(m);
    }
    mapInstances.clear();

    for (size_t i = 0; i < classesInstances.size(); i++)
    {
        ClassInstance *s = classesInstances[i];
        freeClass(s);
    }
    classesInstances.clear();

    for (size_t i = 0; i < nativeInstances.size(); i++)
    {
        NativeInstance *s = nativeInstances[i];
        freeNativeClass(s);
    }
    nativeInstances.clear();

    for (size_t i = 0; i < nativeStructInstances.size(); i++)
    {
        NativeStructInstance *a = nativeStructInstances[i];
        freeNativeStruct(a);
    }
    nativeStructInstances.clear();

    Info("Arrays %ld ", totalArrays);
    Info("Maps %ld ", totalMaps);
    Info("Structs %ld ", totalStructs);
    Info("Classes %ld ", totalClasses);
    Info("Native classes %ld ", totalNativeClasses);
    Info("Native structs %ld ", totalNativeStructs);
    Info("Total instances %ld ", totalArrays + totalMaps + totalStructs + totalClasses + totalNativeClasses + totalNativeStructs);
    Info("Bytes allocated %ld", bytesAllocated);

    bytesAllocated = 0;

    arena.Stats();
    arena.Clear();
}