#include "interpreter.hpp"
#include "compiler.hpp"
#include "instances.hpp"
#include "pool.hpp"
#include "opcode.hpp"
#include "debug.hpp"
#include <new>
#include <stdarg.h>
#include "string.hpp"

bool StringEq::operator()(String *a, String *b) const
{

    if (a == b)
        return true;
    if (a->length() != b->length())
        return false;
    return memcmp(a->chars(), b->chars(), a->length()) == 0;
}

size_t StringHasher::operator()(String *x) const
{
    return x->hash;
}

Interpreter::Interpreter()
{

    InstancePool::instance().setInterpreter(this);
    staticFree = createStaticString("free");
    staticToString = createStaticString("toString");
    compiler = new Compiler(this);
    setPrivateTable();

    // globals.set(createString("String"), Value::makeFunction(0));
}

Interpreter::~Interpreter()
{
    globals.destroy();
    currentFiber = nullptr;
    currentProcess = nullptr;

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

    ProcessPool::instance().clear();

    Info("Interpreter released");
 

    StringPool::instance().clear();
    InstancePool::instance().clear();

    InstancePool::instance().setInterpreter(nullptr);
    functionsMap.destroy();
    processesMap.destroy();

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

    // for (size_t i = 0; i < natives.size(); i++)
    // {
    //     NativeDef *native = natives[i];
    //     delete native;
    // }
    natives.clear();

    for (size_t i = 0; i < structs.size(); i++)
    {
        StructDef *a = structs[i];
        delete a;
    }
    structs.clear();

    for (size_t j = 0; j < classes.size(); j++)
    {
        ClassDef *proc = classes[j];
        delete proc;
    }

    for (size_t i = 0; i < nativeStructs.size(); i++)
    {
        NativeStructDef *a = nativeStructs[i];
        delete a;
    }
    nativeStructs.clear();
    for (size_t j = 0; j < nativeClasses.size(); j++)
    {
        NativeClassDef *proc = nativeClasses[j];
        delete proc;
    }

    classes.clear();

    for (size_t j = 0; j < processes.size(); j++)
    {
        ProcessDef *proc = processes[j];
        proc->release();
        delete proc;
    }
        processes.clear();

    // Info("Heap stats:");
    // heapAllocator.Stats();
    heapAllocator.Clear();

    // size_t lastMem = getMemoryUsage();
    // Info("Interpreter released used: %zu KB", lastMem);
}

void Interpreter::gc()
{
     
}

uint32 Interpreter::getTotalStrings()
{
    return (uint32)StringPool::instance().map.size();
}

uint32 Interpreter::getStringsBytes()
{
    return StringPool::instance().bytesAllocated;
}

uint32 Interpreter::getTotalBytes()
{
    return InstancePool::instance().bytesAllocated;

}

uint32 Interpreter::getTotalClasses()
{
    return InstancePool::instance().totalClasses;
}

uint32 Interpreter::getTotalStructs()
{
    return InstancePool::instance().totalStructs;
}

uint32 Interpreter::getTotalProcesses()
{
    return aliveProcesses.size();
}

uint32 Interpreter::getTotalArrays()
{
    return InstancePool::instance().totalArrays;
}

uint32 Interpreter::getTotalMaps()
{
    return InstancePool::instance().totalMaps;
}

uint32 Interpreter::getTotalNativeStructs()
{
    return InstancePool::instance().totalNativeStructs;
}

uint32 Interpreter::getTotalNativeClasses()
{
    return InstancePool::instance().totalNativeClasses;
}

bool Interpreter::addModule(const char *name)
{

    
    return true;
}

void Interpreter::setFileLoader(FileLoaderCallback loader, void *userdata)
{
    compiler->setFileLoader(loader, userdata);
}

NativeClassDef *Interpreter::registerNativeClass(const char *name, NativeConstructor ctor, NativeDestructor dtor, int argCount, const char *moduleName)
{
    NativeClassDef *klass = new NativeClassDef();
    klass->name = createStaticString(name);
    int id = nativeClasses.size();
    klass->id = id;
    klass->constructor = ctor;
    klass->destructor = dtor;
    klass->argCount = argCount;

    nativeClasses.push(klass);


        if (!globals.set(klass->name, Value::makeNativeClass(id)))
        {

            Error("Native class '%s' already exists", name);
            return nullptr;
        }

        globals.set(klass->name, Value::makeNativeClass(id));

        nativesClassesMap.set(klass->name, klass);

        return klass;
    
    
}

void Interpreter::addNativeMethod(NativeClassDef *klass, const char *methodName, NativeMethod method)
{
    String *name = createStaticString(methodName);
    klass->methods.set(name, method);
}

void Interpreter::addNativeProperty(
    NativeClassDef *klass,
    const char *propName,
    NativeGetter getter,
    NativeSetter setter)
{
    String *name = createStaticString(propName);

    NativeProperty prop;
    prop.getter = getter;
    prop.setter = setter;

    klass->properties.set(name, prop);
}

int Interpreter::registerNative(const char *name, NativeFunction func, int arity, const char *module)
{

    NativeDef def;
    def.name = createStaticString(name);
    def.func = func;
    def.arity = arity;
    def.index = natives.size();

 
        if (!globals.set(def.name, Value::makeNative(def.index)))
        {

            Error("Native '%s' already exists", name);
            return -1;
        }

        globals.set(def.name, Value::makeNative(def.index));
        natives.push(def);
        return def.index;
    
}

NativeStructDef *Interpreter::registerNativeStruct(const char *name, size_t structSize, NativeStructCtor ctor, NativeStructDtor dtor, const char *moduleName)
{
    NativeStructDef *klass = new NativeStructDef();
    klass->name = createStaticString(name);
    klass->constructor = ctor;
    klass->destructor = dtor;
    klass->structSize = structSize;
    int id = nativeStructs.size();
    lastInstanceId = id;
    klass->id = id;
    nativeStructs.push(klass);

   
        if (!globals.set(klass->name, Value::makeNativeStruct(id)))
        {

            Error("Struct '%s' already exists", name);
            return nullptr;
        }

        globals.set(klass->name, Value::makeNativeStruct(id));

        return klass;
   
}

void Interpreter::addStructField(NativeStructDef *def, const char *fieldName, size_t offset, FieldType type, bool readOnly)
{
    String *name = createStaticString(fieldName);
    NativeFieldDef field;
    field.offset = offset;
    field.type = type;
    field.readOnly = readOnly;
    if (def->fields.exist(name))
    {
        Warning("Field %s already exists in struct %s", fieldName, def->name->chars());
    }
    def->fields.set(name, field);
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

Value Interpreter::createNativeStruct(int structId, int argc, Value *args)
{
    NativeStructDef *def = nativeStructs[structId];
    Value literal = Value::makeNativeStructInstance(def->structSize);
    NativeStructInstance *instance = literal.asNativeStructInstance();
    instance->def = def;
    if (def->constructor)
    {
        def->constructor(this, instance->data, argc, args);
    }
    return literal;
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
    {
        Warning("No ready fiber");
        return nullptr;
    }

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

    // resetFiber();
}

void Interpreter::resetFiber()
{

    if (currentFiber)
    {
        currentFiber->stackTop = currentFiber->stack;
        currentFiber->frameCount = 0;
        currentFiber->state = FiberState::DEAD;
    }
    // hasFatalError_ = false;
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
    String *pName = createStaticString(name);
    bool result = false;
    if (classesMap.get(pName, out))
    {
        result = true;
    }
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

String *Interpreter::internString(const char *str)
{
    String *pStr = createStaticString(str);
    return pStr;
}

void Interpreter::addFunctionsClasses(Function *fun)
{
    functionsClass.push(fun);
}

bool Interpreter::importModule(const char *name)
{

    
    bool imported = true;
    
    return imported;
}

bool Interpreter::getGlobal(String *name, Value *out) const
{
    // if (globalTable->get(name, out))
    //     return true;
    // for (uint32 i = 0; i < importedModules.size(); i++)
    // {
    //     if (importedModules[i]->get(name, out))
    //         return true;
    // }
    // return false;

    return globals.get(name, out);
}

bool Interpreter::setGlobal(String *name, Value val)
{
    return globals.set(name, val);
}

Function *ClassDef::canRegisterFunction(const char *name)
{
    String *pName = createStaticString(name);
    if (methods.exist(pName))
    {

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

NativeClassDef::~NativeClassDef()
{
    methods.destroy();
    properties.destroy();
}
