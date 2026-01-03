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

ModuleDef::ModuleDef(String *name) : name(name)
{
}

uint16 ModuleDef::addFunction(const char *name, NativeFunction func, int arity)
{
    String *nameStr = createString(name);

    uint16 existingId;
    if (functionNames.get(nameStr, &existingId))
    {
        Warning("Function '%s' already exists in module '%s', replacing",name, this->name);
        return existingId;
    }
 
    uint16 id = (uint16)functions.size();

  
    if (id >= 4096)
    {
        Error("Too many functions in module! Max 4096");
        return 0;
    }
    

    functions.push({func, arity});
    functionNames.set(nameStr, id);

    return id;
}

uint16 ModuleDef::addConstant(const char *name, Value value)
{
    String *nameStr = createString(name);

   
    uint16 existingId;
    if (constantNames.get(nameStr, &existingId))
    {
        Warning("Constant '%s' already exists in module '%s', replacing",name, this->name);
        constants[existingId] = value;
        return existingId;
    }

 
    uint16 id = (uint16)constants.size();

    if (id >= 4096)
    {
        Error("Too many constants in module! Max 4096");
        return 0;
    }

    constants.push(value);
    constantNames.set(nameStr, id);

    return id;
}

NativeFunctionDef *ModuleDef::getFunction(uint16 id)
{
    return &functions[id];
}

Value *ModuleDef::getConstant(uint16 id)
{
    return &constants[id];
}

bool ModuleDef::getFunctionId(String *name, uint16 *outId)
{
     return functionNames.get(name, outId);
}

bool ModuleDef::getConstantId(String *name, uint16 *outId)
{
     return constantNames.get(name, outId);
}

bool ModuleDef::getFunctionName(uint16 id, String **outName)
{
    
    functionNames.forEachWhile([&](String *key, uint16 value) 
    {
        if (value == id)
        {
            *outName = key;
            return false;
        }
        return true;
    });
    return false;
}

bool ModuleDef::getConstantName(uint16 id, String **outName)
{
    
    constantNames.forEachWhile([&](String *key, uint16 value) 
    {
        if (value == id)
        {
            *outName = key;
            return false;
        }
        return true;
    });
    return false;
}

ModuleBuilder::ModuleBuilder(ModuleDef *module) : module(module)
{
}

ModuleBuilder &ModuleBuilder::addFunction(const char *name, NativeFunction func, int arity)
{
    module->addFunction(name,func, arity);
    return *this;
}

ModuleBuilder &ModuleBuilder::addInt(const char *name, int value)
{
    module->addConstant(name,Value::makeInt(value));
    return *this;
}

ModuleBuilder &ModuleBuilder::addFloat(const char *name, float value)
{
    module->addConstant(name,Value::makeFloat(value));
    return *this;
}

ModuleBuilder &ModuleBuilder::addDouble(const char *name, double value)
{
    module->addConstant(name,Value::makeDouble(value));
    return *this;
}

ModuleBuilder &ModuleBuilder::addBool(const char *name, bool value)
{
    module->addConstant(name,Value::makeBool(value));
    return *this;
}

ModuleBuilder &ModuleBuilder::addString(const char *name, const char *value)
{
    module->addConstant(name,Value::makeString(value));
    return *this;
}

uint16 Interpreter::defineModule(const char *name)
{
    String *nameStr = createString(name);

    uint16 existingId;
    if (moduleNames.get(nameStr, &existingId))
    {
        Warning("Module '%s' already defined, returning existing ID %d",
                name, existingId);
        return existingId;
    }

    ModuleDef *mod = new ModuleDef(nameStr);
    uint16 id = (uint16)modules.size();

    if (id >= 4096)
    {
        Error("Too many modules! Max 4096");
        delete mod;
        return 0;
    }

    modules.push(mod);
    moduleNames.set(nameStr, id);
    return id;
}

ModuleBuilder Interpreter::addModule(const char *name)
{
    uint16 id = defineModule(name);
    return ModuleBuilder(modules[id]);
}

ModuleDef *Interpreter::getModule(uint16 id)
{
    if (id >= modules.size())
        return nullptr;
    return modules[id];
}

bool Interpreter::getModuleId(String *name, uint16 *outId)
{
    return moduleNames.get(name, outId);
}
