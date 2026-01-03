#include "compiler.hpp"
#include "code.hpp"
#include "interpreter.hpp"
#include "platform.hpp"
#include "opcode.hpp"
#include "pool.hpp"
#include "value.hpp"
#include <cstdio>
#include <cstdlib>
#include <stdarg.h>

ModuleDef *ModuleDef::add(const char *funcName, NativeFunction ptr, int arity)
{
    String *key = createString(funcName);
    FunctionDef def;
    def.ptr = ptr;
    def.arity = arity;
    if(!functions.set(key, def))
    {
        Warning("Function %s already exists in module %s", funcName,    
                name->chars());
    }
    return this;
}

ModuleDef *ModuleDef::addInt(const char *name, int value)
{
    String *key = createString(name);
    ConstantDef def;
    def.value = Value::makeInt(value);
    if(!constants.set(key, def))
    {
        Warning("Constant %s already exists in module %s", name,    name);
    }
    return this;
}

bool ModuleDef::getFunction(String *funcName, FunctionDef *out)
{
    return functions.get(funcName, out);
}

bool ModuleDef::getConstant(String *constName, ConstantDef *out)
{
    return constants.get(constName, out);
}

ModuleDef *Interpreter::defineModule(const char *moduleName)
{
    String* name = createString(moduleName);
    ModuleDef *def = new ModuleDef(name);
    if(!availableModules.set(name, def))
    {
        Warning("Module %s already exists", moduleName);
        delete def;
        return nullptr;
    }
    return def;
}

bool Interpreter::getModuleDef(String *name, ModuleDef **out)
{
   return availableModules.get(name, out);
}
