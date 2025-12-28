#include "interpreter.hpp"
#include "pool.hpp"

#define PRINT_GC 0




GCObject::GCObject()
{
    type = GC_NONE;
    index=0;
}

GCObject::~GCObject()
{
}

String::String()
{
    hash = 0;
    length_and_flag = 0;
    index=0;

    // #if PRINT_GC
    // Info("create string");
    // #endif
}

String::~String()
{
    // #if PRINT_GC
    // Info("Delete string");
    // #endif
    // Info("String destructor called %d times with refCount %d", destructorCallCount,refCount);
}

StructInstance::StructInstance() : GCObject(), def(nullptr)
{
    #if PRINT_GC
    Info("create struct");
    #endif
    type = GC_STRUCT;
}

StructInstance::~StructInstance()
{
    #if PRINT_GC
    Info("Delete struct");
    #endif
}

void StructInstance::release()
{
}

ArrayInstance::ArrayInstance() : GCObject()
{
    type = GC_ARRAY;
    
    #if PRINT_GC
    Info("create array");
    #endif
}

ArrayInstance::~ArrayInstance()
{
    #if PRINT_GC
    Info("Delete array");
    #endif
}

void ArrayInstance::release()
{
}

MapInstance::MapInstance() : GCObject()
{
    type = GC_MAP;

    #if PRINT_GC
    Info("create map");
    #endif
}

MapInstance::~MapInstance()
{
    #if PRINT_GC
    Info("Delete map");
    #endif
}

void MapInstance::release()
{
    // table.forEach([&](String *key, Value value)
    //               {
    //     (void)key;
    //     value.release(); });
    // table.destroy();
    // InstancePool::instance().freeMap(this);
}

ClassInstance::ClassInstance() : GCObject()
{
  
    type = GC_CLASS;
    klass = nullptr;

    #if PRINT_GC
    Info("create class");
    #endif
}

ClassInstance::~ClassInstance()
{
    #if PRINT_GC
    Info("Delete class");
    #endif
}

bool ClassInstance::getMethod(String *name, Function **out)
{
    ClassDef *current = klass;

    while (current)
    {
        if (current->methods.get(name, out))
        {
            return true;
        }
        current = current->superclass;
    }

    return false;
}

void ClassInstance::release()
{
}

NativeStructInstance::NativeStructInstance() : GCObject(), def(nullptr)
{
    #if PRINT_GC
    Info("create native struct");
    #endif
    type = GC_NATIVESTRUCT;
}

NativeStructInstance::~NativeStructInstance()
{
    #if PRINT_GC
    Info("Delete native struct");
    #endif
}

void NativeStructInstance::release()
{
    
}

NativeInstance::NativeInstance() : GCObject()
{
    type = GC_NATIVECLASS;
    klass = nullptr;
    userData = nullptr;
}

NativeInstance::~NativeInstance()
{
    #if PRINT_GC
    Info("Delete native instance");
    #endif

}

void NativeInstance::release()
{
    
}
