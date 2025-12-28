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
struct Value;
struct String;
 
class Interpreter;
 

class InstancePool
{
    HeapAllocator arena;
    Interpreter *interpreter;

    Vector<StructInstance *> structInstances;
    Vector<ArrayInstance *> arrayInstances;
    Vector<ClassInstance *> classesInstances;
    Vector<NativeInstance *> nativeInstances;
    Vector<NativeStructInstance *> nativeStructInstances;

    Vector<String *> strings;
  

    Vector<GCObject *> grayStack;

    size_t bytesAllocated = 0;   //  Tracking bytes
    size_t nextGC =1024;// 1024 * 1024; //  Trigger threshold

 
     uint32 totalStructs = 0;
     uint32 totalArrays = 0;
     uint32 totalMaps = 0;
     uint32 totalClasses = 0;
     uint32 totalNativeStructs = 0;
     uint32 totalNativeClasses = 0;

    // Private GC methods
    void markObject(GCObject *obj);
    void markValue(const Value &v);
    void markRoots();
    void traceReferences();
    void blackenObject(GCObject *obj);
    void sweep();

    friend class Interpreter; // Friend para aceder bytesAllocated, nextGC
    friend class StringPool;

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

    // Create methods
    StructInstance *createStruct();
    ArrayInstance *createArray(int reserve = 0);
    MapInstance *createMap();
    ClassInstance *creatClass();
    NativeInstance *createNativeClass();
    NativeStructInstance *createNativeStruct(uint32 structSize);

    // Free methods
    void freeStruct(StructInstance *proc);
    void freeArray(ArrayInstance *a);
    void freeMap(MapInstance *m);
    void freeClass(ClassInstance *c);
    void freeNativeClass(NativeInstance *n);
    void freeNativeStruct(NativeStructInstance *n);

 

    // Public GC trigger
    void gc();
    void checkGC(); // Verifica se deve trigger GC

    int getTotalBytes() { return bytesAllocated; }

    void clear();
};