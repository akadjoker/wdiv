#include "interpreter.hpp"
#include "compiler.hpp"
#include "instances.hpp"
#include "pool.hpp"
#include "opcode.hpp"
#include "debug.hpp"
#include <new>
#include <stdarg.h>

Interpreter::Interpreter()
{
    compiler = new Compiler(this);
    setPrivateTable();
}

Interpreter::~Interpreter()
{
    functionsMap.destroy();
    processesMap.destroy();
    nativesMap.destroy();
    delete compiler;
    for (size_t i = 0; i < functions.size(); i++)
    {
        Function *func = functions[i];
        delete func;
    }
    functions.clear();

    for (size_t i = 0; i < functionsClass.size(); i++)
    {
        Function *func = functionsClass[i];
        delete func;
    }
    functionsClass.clear();

    for (size_t j = 0; j < processes.size(); j++)
    {
        ProcessDef *proc = processes[j];
        proc->release();
        delete proc;
    }

    processes.clear();
    ProcessPool::instance().clear();

    for (size_t j = 0; j < cleanProcesses.size(); j++)
    {
        Process *proc = cleanProcesses[j];
        proc->release();
        ProcessPool::instance().free(proc);
    }
    cleanProcesses.clear();

    for (size_t i = 0; i < aliveProcesses.size(); i++)
    {
        Process *process = aliveProcesses[i];
        process->release();
        ProcessPool::instance().free(process);
    }

    aliveProcesses.clear();

    for (size_t i = 0; i < natives.size(); i++)
    {
        NativeDef &native = natives[i];
        if (native.name)
        {
            destroyString(native.name);
        }
    }
    natives.clear();

    for (size_t i = 0; i < structs.size(); i++)
    {
        StructDef *a = structs[i];
        destroyString(a->name);
        delete a;
    }
    structs.clear();

    for (size_t i = 0; i < structInstances.size(); i++)
    {
        StructInstance *a = structInstances[i];
        InstancePool::instance().freeStruct(a);
    }
    structInstances.clear();

    for (size_t i = 0; i < arrayInstances.size(); i++)
    {
        ArrayInstance *a = arrayInstances[i];
        InstancePool::instance().freeArray(a);
    }
    arrayInstances.clear();

    for (size_t i = 0; i < classesInstances.size(); i++)
    {
        ClassInstance *a = classesInstances[i];
        InstancePool::instance().freeClass(a);
    }
    classesInstances.clear();

    for (size_t j = 0; j < classes.size(); j++)
    {
        ClassDef *proc = classes[j];

        delete proc;
    }

    StringPool::instance().clear();
    InstancePool::instance().clear();
}

void Interpreter::setFileLoader(FileLoaderCallback loader, void *userdata)
{
    compiler->setFileLoader(loader, userdata);
}

void Interpreter::disassemble()
{

    printf("\n");
    printf("========================================\n");
    printf("         BYTECODE DUMP\n");
    printf("========================================\n\n");

    // ========== FUNCTIONS ==========
    printf(">>> FUNCTIONS: %zu\n\n", functions.size());

    for (size_t i = 0; i < functions.size(); i++)
    {
        Function *func = functions[i];
        if (!func)
            continue;

        printf("----------------------------------------\n");
        printf("Function #%zu: %s\n", i, func->name->chars());
        printf("  Arity: %d\n", func->arity);
        printf("  Has return: %s\n", func->hasReturn ? "yes" : "no");
        printf("----------------------------------------\n");

        // Constants
        printf("\nConstants (%zu):\n", func->chunk->constants.size());
        for (size_t j = 0; j < func->chunk->constants.size(); j++)
        {
            printf("  [%zu] = ", j);
            printValue(func->chunk->constants[j]);
            printf("\n");
        }

        // Bytecode usando Debug::disassembleInstruction
        printf("\nBytecode:\n");
        for (size_t offset = 0; offset < func->chunk->count;)
        {
            printf("  ");
            offset = Debug::disassembleInstruction(*func->chunk, offset);
        }

        printf("\n");
    }

    // ========== PROCESSES ==========
    printf("\n>>> PROCESSES: %zu\n\n", processes.size());

    for (size_t i = 1; i < processes.size(); i++)
    {
        ProcessDef *proc = processes[i];
        if (!proc)
            continue;

        printf("----------------------------------------\n");
        printf("Process #%zu: %s \n", i, proc->name->chars());
        printf("----------------------------------------\n");

        if (proc->fibers[0].frameCount > 0)
        {
            Function *func = proc->fibers[0].frames[0].func;

            printf("  Function: %s\n", func->name->chars());
            printf("  Arity: %d\n", func->arity);

            // Constants
            printf("\nConstants (%zu):\n", func->chunk->constants.size());
            for (size_t j = 0; j < func->chunk->constants.size(); j++)
            {
                printf("  [%zu] = ", j);
                printValue(func->chunk->constants[j]);
                printf("\n");
            }

            // Bytecode usando Debug::disassembleInstruction
            printf("\nBytecode:\n");
            for (size_t offset = 0; offset < func->chunk->count;)
            {
                printf("  ");
                offset = Debug::disassembleInstruction(*func->chunk, offset);
            }
        }

        printf("\n");
    }

    // ========== NATIVES ==========
    printf("\n>>> NATIVE FUNCTIONS: %zu\n\n", natives.size());

    for (size_t i = 0; i < natives.size(); i++)
    {
        printf("  [%2zu] %-20s (arity: %2d)\n",
               i,
               natives[i].name->chars(),
               natives[i].arity);
    }

    printf("\n========================================\n");
    printf("              END OF DUMP\n");
    printf("========================================\n\n");
}

void Interpreter::print(Value value)
{
    printValue(value);
}

Fiber *Interpreter::get_ready_fiber(Process *proc)
{
    int checked = 0;
    int totalFibers = proc->nextFiberIndex;

    if (totalFibers == 0)
        return nullptr;

    // printf("[get_ready_fiber] Checking %d fibers, time=%.3f\n", totalFibers,
    // currentTime);

    while (checked < totalFibers)
    {
        int idx = proc->currentFiberIndex;
        proc->currentFiberIndex = (proc->currentFiberIndex + 1) % totalFibers;

        Fiber *f = &proc->fibers[idx];

        // printf("  Fiber %d: state=%d, resumeTime=%.3f\n", idx, (int)f->state,
        // f->resumeTime);

        checked++;

        if (f->state == FiberState::DEAD)
        {
            //  printf("  -> DEAD, skip\n");
            continue;
        }

        if (f->state == FiberState::SUSPENDED)
        {
            if (currentTime >= f->resumeTime)
            {
                // printf("  -> RESUMING (%.3f >= %.3f)\n", currentTime, f->resumeTime);
                f->state = FiberState::RUNNING;
                return f;
            }
            // printf("  -> Still suspended (wait %.3fms)\n", (f->resumeTime -
            // currentTime) * 1000);
            continue;
        }

        if (f->state == FiberState::RUNNING)
        {
            //  printf("  -> RUNNING, execute\n");
            return f;
        }
    }

    //  printf("  -> No ready fiber\n");
    return nullptr;
}

float Interpreter::getCurrentTime() const
{
    return currentTime;
}

void Interpreter::runtimeError(const char *format, ...)
{
    hasFatalError_ = true;

    fprintf(stderr, "Runtime Error: ");
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

#ifdef WDIV_DEBUG
    if (currentFiber->frameCount > 0)
    {
        // CallFrame *frame = &currentFiber->frames[currentFiber->frameCount - 1];
        // Debug::dumpFunction(frame->func);
    }
#endif

    resetFiber();
}

void Interpreter::resetFiber()
{

    if (currentFiber)
    {
        currentFiber->stackTop = currentFiber->stack;
        currentFiber->frameCount = 0;
        currentFiber->state = FiberState::DEAD;
    }
    hasFatalError_ = false;
}

bool Interpreter::isTruthy(const Value &value)
{
    switch (value.type)
    {
    case ValueType::NIL:
        return false;
    case ValueType::BOOL:
        return value.asBool();
    case ValueType::INT:
        return value.asInt() != 0;
    case ValueType::DOUBLE:
        return value.asDouble() != 0.0;
    default:
        return true; // strings, functions são truthy
    }
}

bool Interpreter::isFalsey(Value value)
{
    return value.isNil() || (value.isBool() && !value.asBool());
}

Function *Interpreter::compile(const char *source)
{
    ProcessDef *proc = compiler->compile(source);
    Function *mainFunc = proc->fibers[0].frames[0].func;
    return mainFunc;
}

Function *Interpreter::compileExpression(const char *source)
{
    ProcessDef *proc = compiler->compileExpression(source);
    Function *mainFunc = proc->fibers[0].frames[0].func;
    return mainFunc;
}

bool Interpreter::run(const char *source, bool _dump)
{
    hasFatalError_ = false;
    ProcessDef *proc = compiler->compile(source);
    if (!proc)
    {
        return false;
    }

    // functionsMap.destroy();
    // processesMap.destroy();
    // nativesMap.destroy();

    if (_dump)
    {
        disassemble();
        // Function *mainFunc = proc->fibers[0].frames[0].func;
        //   Debug::dumpFunction(mainFunc);
    }

    mainProcess = spawnProcess(proc);
    currentProcess = mainProcess;

    Fiber *fiber = &mainProcess->fibers[0];

    //  Debug::disassembleChunk(*fiber->frames[0].func->chunk,"#main");

    run_fiber(fiber);

    return !hasFatalError_;
}
void Interpreter::reset()
{

    compiler->clear();
}
void Interpreter::setHooks(const VMHooks &h)
{
    hooks = h;
}

void Interpreter::initFiber(Fiber *fiber, Function *func)
{
    fiber->state = FiberState::RUNNING;
    fiber->resumeTime = 0.0f;

    fiber->stackTop = fiber->stack;

    fiber->ip = func->chunk->code;

    fiber->frameCount = 1;
    fiber->frames[0].func = func;
    fiber->frames[0].ip = nullptr;
    fiber->frames[0].slots = fiber->stack; // Base da stack
}

void Interpreter::setPrivateTable()
{
    privateIndexMap.set("x", 0);
    privateIndexMap.set("y", 1);
    privateIndexMap.set("z", 2);
    privateIndexMap.set("graph", 3);
    privateIndexMap.set("angle", 4);
    privateIndexMap.set("size", 5);
    privateIndexMap.set("flags", 6);
    privateIndexMap.set("id", 7);
    privateIndexMap.set("father", 8);
}

StructDef *Interpreter::addStruct(String *name, int *id)
{

    if (structsMap.exist(name))
    {
        return nullptr;
    }
    StructDef *proc = new StructDef();
    structsMap.set(name, proc);
    *id = (int)structs.size();
    structs.push(proc);
    return proc;
}

StructDef *Interpreter::registerStruct(String *name, int *id)
{
    if (structsMap.exist(name))
    {
        return nullptr;
    }

    StructDef *proc = new StructDef();

    proc->name = name;
    proc->argCount = 0;
    structsMap.set(name, proc);

    *id = (int)structs.size();

    structs.push(proc);

    return proc;
}

ClassDef *Interpreter::registerClass(String *name, int *id)
{
    if (structsMap.exist(name))
    {
        return nullptr;
    }

    ClassDef *proc = new ClassDef();
    proc->name = name;
    classesMap.set(name, proc);

    *id = (int)classes.size();

    classes.push(proc);

    return proc;
}

bool Interpreter::containsClassDefenition(String *name)
{
    return classesMap.exist(name);
}

bool Interpreter::getClassDefenition(String *name, ClassDef *result)
{
    return classesMap.get(name, &result);
}

bool Interpreter::tryGetClassDefenition(const char *name, ClassDef **out)
{
    String *pName = createString(name);
    bool result = false;
    if (classesMap.get(pName, out))
    {
        result = true;
    }
    return result;
}

int Interpreter::addGlobal(const char *name, Value value)
{
    String *pName = createString(name);
    if (globals.exist(pName))
    {
        destroyString(pName);
        return -1;
    }
    globals.set(pName, value);
    globalList.push(value);

    return (int)(globalList.size() - 1);
}

String *Interpreter::addGlobalEx(const char *name, Value value)
{
    String *pName = createString(name);
    if (globals.exist(pName))
    {
        destroyString(pName);
        return nullptr;
    }
    globals.set(pName, value);
    globalList.push(value);

    return pName;
}

Value Interpreter::getGlobal(uint32 index)
{
    if (index >= globalList.size())
        return Value::makeNil();
    return globalList[index];
}

bool Interpreter::tryGetGlobal(const char *name, Value *value)
{
    String *pName = createString(name);
    bool result = false;
    if (globals.get(pName, value))
    {
        result = true;
    }
    destroyString(pName);
    return result;
}

void Interpreter::addFiber(Process *proc, Function *func)
{
    if (proc->nextFiberIndex >= MAX_FIBERS)
    {
        runtimeError("Too many fibers in process");
        return;
    }

    int index = proc->nextFiberIndex++;
    initFiber(&proc->fibers[index], func);
}

void Interpreter::addFunctionsClasses(Function *fun)
{
    functionsClass.push(fun);
}

StructInstance::StructInstance() : GCObject(), def(nullptr)
{
}

StructInstance::~StructInstance()
{
    Info("destroy struct %d", refCount);
}

GCObject::GCObject() : refCount(1)
{
}

void GCObject::grab()
{
    refCount++;
}

void GCObject::release()
{
    refCount--;
}

ArrayInstance::ArrayInstance() : GCObject()
{
}

ArrayInstance::~ArrayInstance()
{
}

MapInstance::MapInstance() : GCObject()
{
}

MapInstance::~MapInstance()
{
}

ClassInstance::ClassInstance() : GCObject()
{
}

ClassInstance::~ClassInstance()
{
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

// bool ClassInstance::getMethod(String *name, Function **out)
// {
//     if(klass->methods.get(name, out))
//     {
//         return true;
//     }

//     if (klass->inherited)
//     {
//         if(klass->superclass->methods.get(name,out));
//         {
//             return true;
//         }
//     }
//     return false;
// }

Function *ClassDef::canRegisterFunction(const char *name)
{
    String *pName = createString(name);
    if (methods.exist(pName))
    {
        destroyString(pName);
        return nullptr;
    }
    Function *func = new Function();
    func->arity = 0;
    func->hasReturn = false;
    func->name = pName;
    func->chunk = new Code(16);
    methods.set(pName, func);
    return func;
}

ClassDef::~ClassDef()
{
    methods.destroy();
}
