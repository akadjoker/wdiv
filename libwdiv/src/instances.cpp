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

#define GC_HEAP_GROW_FACTOR 2

void InstancePool::setInterpreter(Interpreter *interpreter)
{
    this->interpreter = interpreter;
}

Interpreter *InstancePool::getInterpreter()
{
    return interpreter;
}

// ============= GC MARKING =============

void InstancePool::markValue(const Value &v)
{

     if (v.type == ValueType::STRING && v.as.string)
     {
        Info("Marking string %s", v.as.string->chars());
        v.as.string->marked = true;
        strings.push(v.as.string);
     }
     else if (v.type == ValueType::CLASSINSTANCE && v.as.sClass)
        markObject((GCObject *)v.as.sClass);
    else if (v.type == ValueType::ARRAY && v.as.array)
        markObject((GCObject *)v.as.array);
    else if (v.type == ValueType::MAP && v.as.map)
        markObject((GCObject *)v.as.map);
    else if (v.type == ValueType::STRUCTINSTANCE && v.as.sInstance)
        markObject((GCObject *)v.as.sInstance);
    else if (v.type == ValueType::NATIVESTRUCTINSTANCE && v.as.sNativeStruct)
        markObject((GCObject *)v.as.sNativeStruct);
    else if (v.type == ValueType::NATIVECLASSINSTANCE && v.as.sClassInstance)
        markObject((GCObject *)v.as.sClassInstance);

}

void InstancePool::markObject(GCObject *obj)
{
    if (obj == nullptr)
        return;
    if (obj->marked)
        return;

    obj->marked = true;
    grayStack.push(obj);
    
}

void InstancePool::markRoots()
{

    if (!interpreter)
        return;

    Fiber *fiber = interpreter->currentFiber;
    if (fiber)
    {
      //  Info("Marking fiber");
        // Mark stack
        for (Value *v = fiber->stack; v < fiber->stackTop; v++)
        {
            markValue(*v);
           // printValueNl(*v);
        }
    }
    // Mark globals

    interpreter->globals.forEach([this](String *key, Value val)
    { 
       // (void)key;
     //   Info("Marking global %s", key->chars());
       // printValueNl(val);
        markValue(val); 
    });

 //   Info("Marking current process");
    // Mark process privates
    if(!interpreter->currentProcess) return;
    for (size_t i = 0; i < MAX_PRIVATES; i++)
        markValue(interpreter->currentProcess->privates[i]);
}

// ============= GC TRACING =============

void InstancePool::traceReferences()
{
    Info("Tracing references");
    while (grayStack.size() > 0)
    {
        GCObject *obj = grayStack.back();
        grayStack.pop();
        blackenObject(obj);
    }
}

void InstancePool::blackenObject(GCObject *obj)
{
    if (!obj)
        return;

    switch (obj->type)
    {
    case GC_NONE:
    {
        Warning("GCObject of type GC_NONE");
        break;
    }
    case GC_STRING:
    {
        String *str = (String *)obj;
        str->marked = true;

        break;
    }
    case GC_CLASSINSTANCE:
    {
        ClassInstance *inst = (ClassInstance *)obj;
        for (size_t i = 0; i < inst->fields.size(); i++)
        {
            markValue(inst->fields[i]);
        }
        break;
    }

    case GC_ARRAY:
    {
        ArrayInstance *arr = (ArrayInstance *)obj;
        for (size_t i = 0; i < arr->values.size(); i++)
        {
            markValue(arr->values[i]);
        }
        break;
    }

    case GC_MAP:
    {
        MapInstance *map = (MapInstance *)obj;
        map->table.forEach([this](String *key, Value val)
                           {
             (void) key;
             markValue(val); });
        break;
    }

    case GC_STRUCTINSTANCE:
    {
        StructInstance *inst = (StructInstance *)obj;
        for (size_t i = 0; i < inst->values.size(); i++)
        {
            markValue(inst->values[i]);
        }
        break;
    }

    case GC_NATIVESTRUCTINSTANCE:
    case GC_NATIVECLASSINSTANCE:
        // Native instances sem fields, ignore
        break;
    }
}

// ============= GC SWEEP =============

void InstancePool::sweep()
{

    //strings 
    for (int i = (int)strings.size() - 1; i >= 0; i--)
    {
        if (!strings[i]->marked && !strings[i]->persistent)
        {
            StringPool::instance().deallocString(strings[i]);
            strings.swapAndPop(i);
        }
        else
        {
            strings[i]->marked = false;
        }
    }

    // Classes
    for (int i = (int)classesInstances.size() - 1; i >= 0; i--)
    {
        if (!classesInstances[i]->marked)
        {
         
            freeClass(classesInstances[i]);
            classesInstances.swapAndPop(i);
        }
        else
        {
            classesInstances[i]->marked = false;
        }
    }

    // Arrays
    for (int i = (int)arrayInstances.size() - 1; i >= 0; i--)
    {
        if (!arrayInstances[i]->marked)
        {
     
            freeArray(arrayInstances[i]);
            arrayInstances.swapAndPop(i);
        }
        else
        {
            arrayInstances[i]->marked = false;
        }
    }

    // Maps
    for (int i = (int)classesInstances.size() - 1; i >= 0; i--)
    {
        // TODO: Iterate through mapInstances vector when available
    }

    // Structs
    for (int i = (int)structInstances.size() - 1; i >= 0; i--)
    {
        if (!structInstances[i]->marked)
        {
         
            freeStruct(structInstances[i]);
            structInstances.swapAndPop(i);
        }
        else
        {
            structInstances[i]->marked = false;
        }
    }

    // Native Instances
    for (int i = (int)nativeInstances.size() - 1; i >= 0; i--)
    {
        if (!nativeInstances[i]->marked)
        {
          
            freeNativeClass(nativeInstances[i]);
            nativeInstances.swapAndPop(i);
        }
        else
        {
            nativeInstances[i]->marked = false;
        }
    }

    // Native Structs
    for (int i = (int)nativeStructInstances.size() - 1; i >= 0; i--)
    {
        if (!nativeStructInstances[i]->marked)
        {
     
            freeNativeStruct(nativeStructInstances[i]);
            nativeStructInstances.swapAndPop(i);
        }
        else
        {
            nativeStructInstances[i]->marked = false;
        }
    }

   

}

// ============= GC TRIGGER =============

void InstancePool::checkGC()
{
   // Info("Check GC: %ld bytes allocated, next at %ld bytes and %ld strings", interpreter->gcTotalBytes(), nextGC,strings.size());
    if (interpreter->gcTotalBytes() > nextGC)
    {
        gc();
    }
}

void InstancePool::gc()
{
    size_t before = bytesAllocated;

    grayStack.clear();
    markRoots();
    traceReferences();
    sweep();

    nextGC = bytesAllocated * GC_HEAP_GROW_FACTOR;

    Warning("GC Collected %zu bytes (from %zu to %zu) next at %zu", before - bytesAllocated, before, bytesAllocated, nextGC);

    // // ← Mínimo de 256KB + 2x do que sobrou
    // size_t minThreshold = 1024 * 256;  // 256KB mínimo
    // nextGC = Max(minThreshold, bytesAllocated * 2);

    // size_t aBytesAllocated = bytesAllocated / 1024;
    // size_t aNextGC = nextGC / 1024;
    // Info("GC end: %ld Kb live, next trigger at %ld Kb", aBytesAllocated, aNextGC);
}

// ============= CONSTRUCTORS/DESTRUCTORS =============

InstancePool::InstancePool() : interpreter(nullptr), bytesAllocated(0)
{

    totalStructs = 0;
    totalArrays = 0;
    totalMaps = 0;
    totalClasses = 0;
    totalNativeStructs = 0;
    totalNativeClasses = 0;
    nextGC=1024 ;
}

InstancePool::~InstancePool()
{
}

// ============= CREATE METHODS =============

StructInstance *InstancePool::createStruct()
{
    //checkGC();
    void *mem = arena.Allocate(sizeof(StructInstance));
    StructInstance *instance = new (mem) StructInstance();
    structInstances.push(instance);

    bytesAllocated += sizeof(StructInstance);

    totalStructs++;

    return instance;
}

ArrayInstance *InstancePool::createArray(int reserve)
{
    //checkGC();
    void *mem = arena.Allocate(sizeof(ArrayInstance));
    ArrayInstance *instance = new (mem) ArrayInstance();
    if (reserve != 0)
        instance->values.reserve(reserve);
    arrayInstances.push(instance);

    bytesAllocated += sizeof(ArrayInstance);

    totalArrays++;

    return instance;
}

MapInstance *InstancePool::createMap()
{
    //checkGC();
    void *mem = arena.Allocate(sizeof(MapInstance));
    MapInstance *instance = new (mem) MapInstance();

    bytesAllocated += sizeof(MapInstance);

    totalMaps++;
    return instance;
}

ClassInstance *InstancePool::creatClass()
{
    //checkGC();

    void *mem = arena.Allocate(sizeof(ClassInstance));
    ClassInstance *instance = new (mem) ClassInstance();
    classesInstances.push(instance);

    bytesAllocated += sizeof(ClassInstance);

    totalClasses++;
    return instance;
}

NativeInstance *InstancePool::createNativeClass()
{

    //checkGC();
    void *mem = arena.Allocate(sizeof(NativeInstance));
    NativeInstance *instance = new (mem) NativeInstance();
    nativeInstances.push(instance);

    bytesAllocated += sizeof(NativeInstance);

    totalNativeClasses++;

    return instance;
}

NativeStructInstance *InstancePool::createNativeStruct(uint32 structSize)
{
    //checkGC();

    void *mem = arena.Allocate(sizeof(NativeStructInstance));
    NativeStructInstance *instance = new (mem) NativeStructInstance();


    instance->data =aAlloc(structSize); 
    std::memset(instance->data, 0, structSize);
    nativeStructInstances.push(instance);

    bytesAllocated += sizeof(NativeStructInstance) + structSize;

    totalNativeStructs++;
    return instance;
}

// ============= FREE METHODS =============

void InstancePool::freeStruct(StructInstance *s)
{
    if (!s)
        return;
    s->~StructInstance();
    arena.Free(s, sizeof(StructInstance));

    bytesAllocated -= sizeof(StructInstance);

    totalStructs--;
}

void InstancePool::freeArray(ArrayInstance *a)
{
    if (!a)
        return;
    a->~ArrayInstance();
    arena.Free(a, sizeof(ArrayInstance));

    bytesAllocated -= sizeof(ArrayInstance);

    totalArrays--;
}

void InstancePool::freeMap(MapInstance *m)
{
    if (!m)
        return;
    m->~MapInstance();
    arena.Free(m, sizeof(MapInstance));

    bytesAllocated -= sizeof(MapInstance);

    totalMaps--;
}

void InstancePool::freeClass(ClassInstance *c)
{
    if (!c)
        return;
    c->~ClassInstance();
    arena.Free(c, sizeof(ClassInstance));

    bytesAllocated -= sizeof(ClassInstance);

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

    n->~NativeInstance();
    arena.Free(n, sizeof(NativeInstance));

    bytesAllocated -= sizeof(NativeInstance);

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

    n->~NativeStructInstance();
    arena.Free(n, sizeof(NativeStructInstance));

    bytesAllocated -= sizeof(NativeStructInstance);

    totalNativeStructs--;
}


// ============= CLEANUP =============

void InstancePool::clear()
{
    Info("Instance pool clear");


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

    grayStack.clear();
    bytesAllocated = 0;

  
    Info("Structs %ld ", totalStructs);
    Info("Arrays %ld ", totalArrays);
    Info("Classes %ld ", totalClasses);
    Info("Native classes %ld ", totalNativeClasses);
    Info("Native structs %ld ", totalNativeStructs);

    // DEBUG_BREAK_IF(totalStructs != 0);
    // DEBUG_BREAK_IF(totalArrays != 0);
    // DEBUG_BREAK_IF(totalClasses != 0);
    // DEBUG_BREAK_IF(totalNativeClasses != 0);
    // DEBUG_BREAK_IF(totalNativeStructs != 0);

    arena.Stats();
    arena.Clear();
}