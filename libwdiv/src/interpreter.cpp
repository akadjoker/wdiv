#include "interpreter.hpp"
#include "compiler.hpp"
#include "pool.hpp"
#include "opcode.hpp"
#include "debug.hpp"
#include <new>
#include <stdarg.h>


#define DEBUG_TRACE_EXECUTION 0 // 1 = ativa, 0 = desativa
#define DEBUG_TRACE_STACK 0     // 1 = mostra stack, 0 = esconde

#ifdef NDEBUG
#define WDIV_ASSERT(condition, ...) ((void)0)
#else
#define WDIV_ASSERT(condition, ...)                                            \
    do                                                                         \
    {                                                                          \
        if (!(condition))                                                      \
        {                                                                      \
            Error("ASSERT FAILED: %s:%d: %s", __FILE__, __LINE__, #condition); \
            assert(false);                                                     \
        }                                                                      \
    } while (0)
#endif

Interpreter::Interpreter()
{
    compiler = new Compiler(this);
    setPrivateTable();
}

Interpreter::~Interpreter()
{
    delete compiler;
    for (size_t i = 0; i < functions.size(); i++)
    {
        Function *func = functions[i];
        delete func;
    }
    functions.clear();

    for (size_t j = 0; j < processes.size(); j++)
    {
        ProcessDef *proc = processes[j];
        proc->release();
        delete proc;
    }
    const bool showStats = false;

    processes.clear();
    ProcessPool::instance().clear();

    if (showStats)
        arena.Stats();

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

    // arena.Clear();
    StringPool::instance().clear();
}

void Interpreter::setFileLoader(FileLoaderCallback loader, void *userdata)
{
    compiler->setFileLoader(loader,userdata);
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

    functionsMap.destroy();
    processesMap.destroy();
    nativesMap.destroy();

    if (_dump)
    {
        disassemble();
        // Function *mainFunc = proc->fibers[0].frames[0].func;
        // Debug::dumpFunction(mainFunc);
    }

    mainProcess = spawnProcess(proc);
    currentProcess = mainProcess;

    Fiber *fiber = &mainProcess->fibers[0];

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



// ===== STACK API =====

const Value &Interpreter::peek(int index)
{
    WDIV_ASSERT(currentFiber != nullptr, "No current fiber");

    int top = getTop();
    int realIndex;

    if (index < 0)
        realIndex = top + index; // -1 → top-1, -2 → top-2
    else
        realIndex = index; // 0 → 0, 1 → 1

    if (realIndex < 0 || realIndex >= top)
    {
        runtimeError("Stack index %d out of bounds (size=%d)", index, top);
        static Value null = Value::makeNil();
        return null;
    }

    return currentFiber->stack[realIndex];
}

int Interpreter::getTop()
{
    WDIV_ASSERT(currentFiber != nullptr, "No current fiber");
    return static_cast<int>(currentFiber->stackTop - currentFiber->stack);
}

void Interpreter::setTop(int index)
{
    WDIV_ASSERT(currentFiber != nullptr, "No current fiber");
    if (index < 0 || index > STACK_MAX)
    {
        runtimeError("Invalid stack index");
        return;
    }
    currentFiber->stackTop = currentFiber->stack + index;
}

void Interpreter::push(Value value)
{

    if (currentFiber->stackTop >= currentFiber->stack + STACK_MAX)
    {
        runtimeError("Stack overflow");
        return;
    }
    *currentFiber->stackTop++ = value;
}

Value Interpreter::pop()
{

    if (currentFiber->stackTop <= currentFiber->stack)
    {
        runtimeError("Stack underflow");
        return Value::makeNil();
    }
    return *--currentFiber->stackTop;
}

//  const Value &Interpreter::peek(int distance)
// {

//     //    int stackSize = currentFiber->stackTop - currentFiber->stack;
//     ptrdiff_t stackSize = currentFiber->stackTop - currentFiber->stack;

//     if (distance < 0 || distance >= stackSize)
//     {
//         runtimeError("Stack peek out of bounds: distance=%d, size=%d",
//                      distance, stackSize);
//         static const Value null = Value::makeNil();
//         return null;
//     }
//     return currentFiber->stackTop[-1 - distance];
// }

// Type checking
ValueType Interpreter::getType(int index)
{
    return peek(index).type;
}

bool Interpreter::isInt(int index)
{
    return peek(index).type == ValueType::INT;
}

bool Interpreter::isDouble(int index)
{
    return peek(index).type == ValueType::DOUBLE;
}

bool Interpreter::isString(int index)
{
    return peek(index).type == ValueType::STRING;
}

bool Interpreter::isBool(int index)
{
    return peek(index).type == ValueType::BOOL;
}

bool Interpreter::isNil(int index)
{
    return peek(index).type == ValueType::NIL;
}

bool Interpreter::isFunction(int index)
{
    return peek(index).type == ValueType::FUNCTION;
}

void Interpreter::pushInt(int n)
{
    push(Value::makeInt(n));
}

void Interpreter::pushDouble(double d)
{
    push(Value::makeDouble(d));
}

void Interpreter::pushString(const char *s)
{
    push(Value::makeString(s));
}

void Interpreter::pushBool(bool b)
{
    push(Value::makeBool(b));
}

void Interpreter::pushNil()
{
    push(Value::makeNil());
}

// Type conversions
int Interpreter::toInt(int index)
{
    Value v = peek(index);
    if (!v.isInt())
    {
        runtimeError("Expected int at index %d", index);
        return 0;
    }
    return v.asInt();
}

double Interpreter::toDouble(int index)
{
    Value v = peek(index);

    if (v.isDouble())
    {
        return v.asDouble();
    }
    else if (v.isInt())
    {
        return (double)v.asInt();
    }

    runtimeError("Expected number at index %d", index);
    return 0.0;
}

const char *Interpreter::toString(int index)
{
    const Value &v = peek(index);
    if (!v.isString())
    {
        runtimeError("Expected string at index %d", index);
        return "";
    }
    return v.asString()->chars();
}

bool Interpreter::toBool(int index)
{
    Value v = peek(index);
    return isTruthy(v);
}
