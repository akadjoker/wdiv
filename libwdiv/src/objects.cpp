#include "interpreter.hpp"
#include "pool.hpp"

#define PRINT_GC 0




GCObject::GCObject()
{
    type = GC_NONE;
    marked = false;
}

GCObject::~GCObject()
{
}

String::String()
{
    hash = 0;
    length_and_flag = 0;
    type = GC_STRING;
    marked = true;

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
    type = GC_STRUCTINSTANCE;
}

StructInstance::~StructInstance()
{
    #if PRINT_GC
    Info("Delete struct");
    #endif
}

void StructInstance::drop()
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

void ArrayInstance::drop()
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

void MapInstance::drop()
{
    // table.forEach([&](String *key, Value value)
    //               {
    //     (void)key;
    //     value.drop(); });
    // table.destroy();
    // InstancePool::instance().freeMap(this);
}

ClassInstance::ClassInstance() : GCObject()
{
  
    type = GC_CLASSINSTANCE;
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

void ClassInstance::drop()
{
}

NativeStructInstance::NativeStructInstance() : GCObject(), def(nullptr)
{
    #if PRINT_GC
    Info("create native struct");
    #endif
    type = GC_NATIVESTRUCTINSTANCE;
}

NativeStructInstance::~NativeStructInstance()
{
    #if PRINT_GC
    Info("Delete native struct");
    #endif
}

void NativeStructInstance::drop()
{
    
}

NativeInstance::NativeInstance() : GCObject()
{
    type = GC_NATIVECLASSINSTANCE;
    klass = nullptr;
    userData = nullptr;
}

NativeInstance::~NativeInstance()
{
    #if PRINT_GC
    Info("Delete native instance");
    #endif

}

void NativeInstance::drop()
{
    
}
