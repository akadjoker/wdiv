#pragma once
#include "config.hpp"
#include "vector.hpp"
#include "string.hpp"

struct StructInstance;
struct ArrayInstance;
struct MapInstance;
struct ClassInstance;

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

    StructInstance* createStruct( );
    void freeStruct(StructInstance *proc);

    ArrayInstance* createArray();
    void freeArray(ArrayInstance *a);


    MapInstance* createMap();
    void freeMap(MapInstance *m);

    ClassInstance* creatClass();
    void freeClass(ClassInstance *c);

    void clear();
 
   

};
 