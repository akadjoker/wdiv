#include "interpreter.hpp"
#include "pool.hpp"

Function::~Function()
{
    if (name)
    {
        destroyString(name);
    }
    if (chunk)
    {
        chunk->clear();
        delete chunk;
    }
}

Function *Interpreter::addFunction(const char *name, int arity)
{
    String *pName = createString(name);
    Function *existing = nullptr;

    if (functionsMap.get(pName, &existing))
    {
        destroyString(pName);
        return nullptr;
    }

<<<<<<< HEAD
 
=======
    // Function *func = (Function *)arena.Allocate(sizeof(Function));
>>>>>>> fdf5e2bc48c434b90d4163ab2d1fe87bbcdd4a33
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
    String *pName = createString(name);
    if (functionsMap.exist(pName))
    {
        destroyString(pName);
        *index = -1;
        return nullptr;
    }

    // Function *func = (Function *)arena.Allocate(sizeof(Function));
    Function *func = new Function();

    func->arity = arity;
    func->hasReturn = false;
    func->name = pName;
    func->chunk= new Code(16);

    functionsMap.set(pName, func);
    functions.push(func);
    *index = (int)(functions.size() - 1);
    return func;
}

bool Interpreter::functionExists(const char *name)
{
    String *pName = createString(name);
    bool exists = functionsMap.exist(pName);
    destroyString(pName);
    return exists;
}

int Interpreter::registerFunction(const char *name, Function *func)
{
    if (!func)
    {
        runtimeError("Cannot register null function");
        return -1;
    }
    String *pName = createString(name);
    if (functionsMap.exist(pName))
    {
        destroyString(pName);
        return -1;
    }
    functionsMap.set(pName, func);
    functions.push(func);
    uint32 index = (uint32)(functions.size() - 1);
    return index;
}

int Interpreter::registerNative(const char *name, NativeFunction func, int arity)
{
    String *nName = createString(name);
    NativeDef existing;
    if (nativesMap.get(nName, &existing))
    {
        destroyString(nName);
        return -1; // Já registrado
    }

    NativeDef def;
    def.name = nName;
    def.func = func;
    def.arity = arity;
    def.index = natives.size();

    nativesMap.set(nName, def);
    natives.push(def);

    Info("Registered native: %s (index=%d)", name, def.index);

    globals.set(nName, Value::makeNative(def.index));

    return def.index;
}

void Interpreter::destroyFunction(Function *func)
{
    if (!func)
        return;

    String *funcName = func->name;
    if (funcName)
    {
        Warning(" Remove Function %s", funcName->chars());

        destroyString(funcName);
    }

    func->chunk->clear();

    delete func;

    // Libera memória
}