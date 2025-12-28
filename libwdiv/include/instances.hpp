#pragma once
#include "config.hpp"
#include "vector.hpp"
#include "types.hpp"
#include "arena.hpp"

struct StructInstance;
struct ArrayInstance;
struct MapInstance;
struct ClassInstance;

struct NativeInstance;
struct NativeStructInstance;

struct StructDef;
struct ClassDef;
struct NativeStructDef;
struct NativeClassDef;
struct ArrayInstance;
struct MapInstance;
struct Value;
struct String;

class Interpreter;

class InstancePool
{
    HeapAllocator arena;
    Interpreter *interpreter;

    Vector<StructInstance *> structInstances;
    Vector<ArrayInstance *> arrayInstances;
    Vector<MapInstance *> mapInstances;
    Vector<ClassInstance *> classesInstances;
    Vector<NativeInstance *> nativeInstances;
    Vector<NativeStructInstance *> nativeStructInstances;

    size_t bytesAllocated = 0; //  Tracking bytes
    uint32 totalStructs = 0;
    uint32 totalArrays = 0;
    uint32 totalMaps = 0;
    uint32 totalClasses = 0;
    uint32 totalNativeStructs = 0;
    uint32 totalNativeClasses = 0;

    size_t structSize = 0;
    size_t arraySize = 0;
    size_t mapSize = 0;
    size_t classSize = 0;
    size_t nativeStructSize = 0;
    size_t nativeClassSize = 0;

    friend class Interpreter;  
    friend class StringPool;

    StructDef *dummyDefStruct = nullptr;
    StructInstance *dummyStructInstance = nullptr;

    ClassDef *dummyDefClass = nullptr;
    ClassInstance *dummyClassInstance = nullptr;

    ArrayInstance *dummyArrayInstance = nullptr;
    MapInstance *dummyMapInstance = nullptr;

    NativeInstance *dummyNativeInstance = nullptr;
    NativeStructInstance *dummyNativeStructInstance = nullptr;

public:
    InstancePool();
    ~InstancePool();

    void setInterpreter(Interpreter *interpreter);
    Interpreter *getInterpreter();

    static InstancePool &instance()
    {
        static InstancePool pool;

        return pool;
    }

    StructInstance *getStruct(int id);
    ClassInstance *getClass(int id);

    ArrayInstance *getArray(int id);
    MapInstance *getMap(int id);
     NativeInstance *getNativeClass(int id);
    NativeStructInstance *getNativeStruct(int id);

    // Create methods
    StructInstance *createStruct();
    ArrayInstance *createArray(int reserve = 0);
    MapInstance *createMap();
    ClassInstance *createClass();
    NativeInstance *createNativeClass();
    NativeStructInstance *createNativeStruct(uint32 structSize);

    // Free methods
    void freeStruct(StructInstance *proc);
    void freeArray(ArrayInstance *a);
    void freeMap(MapInstance *m);
    void freeClass(ClassInstance *c);
    void freeNativeClass(NativeInstance *n);
    void freeNativeStruct(NativeStructInstance *n);

    int getTotalBytes() { return bytesAllocated; }

    void clear();
};