#pragma once
#include "config.hpp"
#include "vector.hpp"
#include "string.hpp"

struct StructInstance;
struct ArrayInstance;
struct MapInstance;

class InstancePool 
{
    HeapAllocator arena;
 
public:
    InstancePool();
    ~InstancePool() ;

    static InstancePool &instance()
    {
        static InstancePool pool;
        return pool;
    }

    StructInstance* createStruct(String *name);
    void freeStruct(StructInstance *proc);

    ArrayInstance* createArray();
    void freeArray(ArrayInstance *a);


    MapInstance* createMap();
    void freeMap(MapInstance *m);

    void clear();
 
   

};
 