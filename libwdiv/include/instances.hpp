#pragma once
#include "config.hpp"
#include "vector.hpp"
#include "string.hpp"

struct StructInstance;
struct ArrayInstance;
struct MapInstance;
struct ClassInstance;
struct NativeInstance;
struct NativeStructInstance;
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

   
   friend class Interpreter;
   
   public:
   InstancePool();
   ~InstancePool() ;
   
   void setInterpreter(Interpreter *interpreter);
   Interpreter *getInterpreter();
    static InstancePool &instance()
    {
        static InstancePool pool;
        return pool;
    }

    StructInstance* createStruct( );
    void freeStruct(StructInstance *proc);

    ArrayInstance* createArray(int reserve=0);
    void freeArray(ArrayInstance *a);


    MapInstance* createMap();
    void freeMap(MapInstance *m);

    ClassInstance* creatClass();
    void freeClass(ClassInstance *c);

    NativeInstance* createNativeClass();
    void freeNativeClass(NativeInstance *n);

    NativeStructInstance* createNativeStruct(uint32 structSize);
    void freeNativeStruct(NativeStructInstance *n);

    void clear();
 
   

};
 