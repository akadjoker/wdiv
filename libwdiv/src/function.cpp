#include "interpreter.hpp"
#include "pool.hpp"

Function::~Function()
{
    if (name)
    {
        
    }
    if (chunk)
    {
        chunk->clear();
        delete chunk;
    }
}

Function *Interpreter::addFunction(const char *name, int arity)
{
    String *pName = createStaticString(name);
    Function *existing = nullptr;

    if (functionsMap.get(pName, &existing))
    {
       
        return nullptr;
    }

    // Function *func = (Function *)arena.Allocate(sizeof(Function));
    Function *func = new Function();

    func->arity = arity;
    func->hasReturn = false;
    func->name = pName;
    func->chunk = new Code(16);

    functionsMap.set(pName, func);
    functions.push(func);

    return func;
}

Function *Interpreter::canRegisterFunction(const char *name, int arity, int *index)
{
    String *pName = createStaticString(name);
    if (functionsMap.exist(pName))
    {
        
        *index = -1;
        return nullptr;
    }

    // Function *func = (Function *)arena.Allocate(sizeof(Function));
    Function *func = new Function();

    func->arity = arity;
    func->hasReturn = false;
    func->name = pName;
    func->chunk = new Code(16);

    functionsMap.set(pName, func);
    functions.push(func);
    *index = (int)(functions.size() - 1);
    return func;
}

bool Interpreter::functionExists(const char *name)
{
    String *pName = createString(name);
    bool exists = functionsMap.exist(pName);
 
    return exists;
}

int Interpreter::registerFunction(const char *name, Function *func)
{
    if (!func)
    {
        runtimeError("Cannot register null function");
        return -1;
    }
    String *pName = createStaticString(name);
    if (functionsMap.exist(pName))
    {
     
        return -1;
    }
    functionsMap.set(pName, func);
    functions.push(func);
    uint32 index = (uint32)(functions.size() - 1);
    return index;
}


void Interpreter::destroyFunction(Function *func)
{
    if (!func)
        return;

    String *funcName = func->name;
    if (funcName)
    {
        Warning(" Remove Function %s", funcName->chars());

 
    }

    func->chunk->clear();

    delete func;

    // Libera memória
}