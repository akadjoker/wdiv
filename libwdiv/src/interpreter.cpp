#include "interpreter.hpp"
#include "compiler.hpp"
#include "pool.hpp"
#include "opcode.hpp"
#include "debug.hpp"
#include <new>
#include <stdarg.h>
#include <cmath> // std::fmod

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
}

Interpreter::~Interpreter()
{
    delete compiler;
    for (size_t i = 0; i < functions.size(); i++)
    {
        Function *func = functions[i];
        if (func->name)
        {
            destroyString(func->name);
        }
        func->chunk.clear();
        // arena.Free(func, sizeof(Function));
        delete func;
    }

    for (size_t j = 0; j < processes.size(); j++)
    {
        Process *proc = processes[j];
        proc->release();
        delete proc;
    }

    processes.clear();

    for (size_t j = 0; j < cleanProcesses.size(); j++)
    {
        Process *proc = cleanProcesses[j];
        proc->release();
        arena.Free(proc, sizeof(Process));
    }
    cleanProcesses.clear();

    for (size_t i = 0; i < natives.size(); i++)
    {
        NativeDef &native = natives[i];
        if (native.name)
        {
            destroyString(native.name);
        }
    }

    for (size_t i = 0; i < aliveProcesses.size(); i++)
    {
        Process *process = aliveProcesses[i];
        process->release();
        arena.Free(process, sizeof(Process));
    }

    aliveProcesses.clear();

    arena.Clear();
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
        printf("\nConstants (%zu):\n", func->chunk.constants.size());
        for (size_t j = 0; j < func->chunk.constants.size(); j++)
        {
            printf("  [%zu] = ", j);
            printValue(func->chunk.constants[j]);
            printf("\n");
        }

        // Bytecode usando Debug::disassembleInstruction
        printf("\nBytecode:\n");
        for (size_t offset = 0; offset < func->chunk.count;)
        {
            printf("  ");
            offset = Debug::disassembleInstruction(func->chunk, offset);
        }

        printf("\n");
    }

    // ========== PROCESSES ==========
    printf("\n>>> PROCESSES: %zu\n\n", processes.size());

    for (size_t i = 1; i < processes.size(); i++)
    {
        Process *proc = processes[i];
        if (!proc)
            continue;

        printf("----------------------------------------\n");
        printf("Process #%zu: %s (id=%u)\n", i, proc->name->chars(), proc->id);
        printf("----------------------------------------\n");

        if (proc->fibers[0].frameCount > 0)
        {
            Function *func = proc->fibers[0].frames[0].func;

            printf("  Function: %s\n", func->name->chars());
            printf("  Arity: %d\n", func->arity);

            // Constants
            printf("\nConstants (%zu):\n", func->chunk.constants.size());
            for (size_t j = 0; j < func->chunk.constants.size(); j++)
            {
                printf("  [%zu] = ", j);
                printValue(func->chunk.constants[j]);
                printf("\n");
            }

            // Bytecode usando Debug::disassembleInstruction
            printf("\nBytecode:\n");
            for (size_t offset = 0; offset < func->chunk.count;)
            {
                printf("  ");
                offset = Debug::disassembleInstruction(func->chunk, offset);
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
    return compiler->compile(source, this);
}

Function *Interpreter::compileExpression(const char *source)
{
    return compiler->compileExpression(source, this);
}
bool Interpreter::run(const char *source, bool _dump)
{
    hasFatalError_ = false;
    Process *proc = compiler->compile(source);
    if (!proc)
    {
        return false;
    }

     if (_dump)
    {
      //  disassemble();
        // Function *mainFunc = proc->fibers[0].frames[0].func;
        // Debug::dumpFunction(mainFunc);
    }
 for (size_t i = 0; i < functions.size(); i++)
    {
        Function *func = functions[i];
        if (!func)
            continue;
        func->chunk.freeze();
    }

    mainProcess =  spawnProcess(proc);
    currentProcess = mainProcess;

    Fiber *fiber = &mainProcess->fibers[0];

    
    Info("Execute");
   // run_fiber(fiber);

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

    fiber->ip = func->chunk.code;

    fiber->frameCount = 1;
    fiber->frames[0].func = func;
    fiber->frames[0].ip = func->chunk.code;
    fiber->frames[0].slots = fiber->stack; // Base da stack
}

bool toNumberPair(const Value &a, const Value &b, double &da, double &db)
{
    if (!(a.isInt() || a.isDouble()))
        return false;
    if (!(b.isInt() || b.isDouble()))
        return false;

    da = a.isInt() ? static_cast<double>(a.asInt()) : a.asDouble();
    db = b.isInt() ? static_cast<double>(b.asInt()) : b.asDouble();
    return true;
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

FiberResult Interpreter::run_fiber(Fiber *fiber)
{

    currentFiber = fiber;

    CallFrame *frame;
    Value *stackStart;
    uint8 *ip;
    Function *func;

    int instructionsRun = 0;

#define DROP() (fiber->stackTop--)
#define PEEK() (*(fiber->stackTop - 1))
#define PEEK2() (*(fiber->stackTop - 2))

#define POP() (*(--fiber->stackTop))
#define PUSH(value) (*fiber->stackTop++ = value)
#define NPEEK(n) (fiber->stackTop[-1 - (n)])

#define READ_BYTE() (*ip++)
#define READ_SHORT() (ip += 2, (uint16)((ip[-2] << 8) | ip[-1]))

#define BINARY_OP_PREP()           \
    Value b = fiber->stackTop[-1]; \
    Value a = fiber->stackTop[-2]; \
    fiber->stackTop -= 2

#define STORE_FRAME() frame->ip = ip

#define LOAD_FRAME()                                   \
    do                                                 \
    {                                                  \
        assert(currentFiber->frameCount > 0);          \
        frame = &fiber->frames[fiber->frameCount - 1]; \
        stackStart = frame->slots;                     \
        ip = frame->ip;                                \
        func = frame->func;                            \
    } while (false)

#define READ_CONSTANT() (func->chunk.constants[READ_BYTE()])
    LOAD_FRAME();

    // printf("[DEBUG] Starting run_fiber: ip=%p, func=%s, offset=%ld\n",
    //        (void*)ip, func->name->chars(), ip - func->chunk.code);

    // ===== LOOP PRINCIPAL =====

    for (;;)
    {

#if DEBUG_TRACE_STACK
        // Mostra stack
        printf("          ");
        for (Value *slot = fiber->stack; slot < fiber->stackTop; slot++)
        {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
#endif

#if DEBUG_TRACE_EXECUTION
        // Mostra instrução
        size_t offset = ip - func->chunk.code;
        Debug::disassembleInstruction(func->chunk, offset);
#endif

        uint8 instruction = READ_BYTE();

        switch (instruction)
        {
            // ========== CONSTANTS ==========

        case OP_CONSTANT:
        {
            Value constant = READ_CONSTANT();
            PUSH(constant);
            break;
        }

        case OP_NIL:
            PUSH(Value::makeNil());
            break;
        case OP_TRUE:
            PUSH(Value::makeBool(true));
            break;
        case OP_FALSE:
            PUSH(Value::makeBool(false));
            break;

        case OP_DUP:
        {
            Value top = PEEK();
            PUSH(top);
            break;
        }

            // ========== STACK MANIPULATION ==========

        case OP_POP:
            DROP();
            break;

            // ========== VARIABLES ==========

        case OP_GET_LOCAL:
        {
            uint8 slot = READ_BYTE();
            PUSH(stackStart[slot]);
            break;
        }

        case OP_SET_LOCAL:
        {
            uint8 slot = READ_BYTE();
            stackStart[slot] = PEEK();
            break;
        }

        case OP_GET_PRIVATE:
        {
            uint8 index = READ_BYTE();
            PUSH(currentProcess->privates[index]);
            break;
        }

        case OP_SET_PRIVATE:
        {
            uint8 index = READ_BYTE();
            currentProcess->privates[index] = PEEK();
            break;
        }

        case OP_GET_GLOBAL:
        {

            Value name = READ_CONSTANT();
            Value value;

            if (!globals.get(name.asString(), &value))
            {
                runtimeError("Undefined variable '%s'", name.asString()->chars());
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }
            PUSH(value);
            break;
        }

        case OP_SET_GLOBAL:
        {

            Value name = READ_CONSTANT();
            globals.set(name.asString(), PEEK());
            break;
        }

        case OP_DEFINE_GLOBAL:
        {

            Value name = READ_CONSTANT();
            globals.set(name.asString(), POP());
            break;
        }

            // ========== ARITHMETIC ==========

        case OP_ADD:
        {
            BINARY_OP_PREP();

            if (a.isString() && b.isString())
            {
                String *result = concatString(a.asString(), b.asString());
                PUSH(Value::makeString(result));
                break;
            }

            if (a.isInt() && b.isInt())
            {
                PUSH(Value::makeInt(a.asInt() + b.asInt()));
                break;
            }

            double da, db;
            if (!toNumberPair(a, b, da, db))
            {
                runtimeError("Operands must be numbers or strings");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            PUSH(Value::makeDouble(da + db));
            break;
        }

        case OP_SUBTRACT:
        {
            BINARY_OP_PREP();

            // Fast-path PRIMEIRO int - int -> int
            if (a.isInt() && b.isInt())
            {
                PUSH(Value::makeInt(a.asInt() - b.asInt()));
                break;
            }

            // Só converte se necessário
            double da, db;
            if (!toNumberPair(a, b, da, db))
            {
                runtimeError("Operands must be numbers");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            PUSH(Value::makeDouble(da - db));
            break;
        }

        case OP_MULTIPLY:
        {
            BINARY_OP_PREP();

            // Fast-path PRIMEIRO int - int -> int
            if (a.isInt() && b.isInt())
            {
                PUSH(Value::makeInt(a.asInt() * b.asInt()));
                break;
            }
            double da, db;
            if (!toNumberPair(a, b, da, db))
            {
                runtimeError("Operands must be numbers");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            PUSH(Value::makeDouble(da * db));
            break;
        }

        case OP_DIVIDE:
        {
            BINARY_OP_PREP();
            if (a.isInt() && b.isInt())
            {
                long valueB = b.asInt();
                if (valueB == 0)
                {
                    runtimeError("Division by zero");
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }
                PUSH(Value::makeInt(a.asInt() / valueB));
            }
            else
            {

                double da, db;
                if (!toNumberPair(a, b, da, db))
                {
                    runtimeError("Operands must be numbers");
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }

                if (db == 0.0)
                {
                    runtimeError("Division by zero");
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }

                PUSH(Value::makeDouble(da / db));
            }
            break;
        }

            //===== MODULO =====

        case OP_MODULO:
        {
            BINARY_OP_PREP();

            // int % int  -> int
            if (a.isInt() && b.isInt())
            {
                if (b.asInt() == 0)
                {
                    runtimeError("Division by zero in modulo");
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }
                PUSH(Value::makeInt(a.asInt() % b.asInt()));
                break;
            }

            // Double / int / double -> double (fmod)
            double da, db;
            if (!toNumberPair(a, b, da, db))
            {
                runtimeError("Operands must be numbers");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            if (db == 0.0)
            {
                runtimeError("Division by zero in modulo");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            PUSH(Value::makeDouble(std::fmod(da, db)));
            break;
        }

            //======== LOGICAL =====

        case OP_NEGATE:
        {
            Value a = POP();
            if (a.isInt())
            {
                PUSH(Value::makeInt(-a.asInt()));
            }
            else if (a.isDouble())
            {
                PUSH(Value::makeDouble(-a.asDouble()));
            }
            else
            {
                runtimeError("Operand must be a number");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }
            break;
        }

        case OP_EQUAL:
        {
            BINARY_OP_PREP();
            PUSH(Value::makeBool(valuesEqual(a, b)));

            break;
        }

        case OP_NOT:
        {
            Value v = POP();
            PUSH(Value::makeBool(!isTruthy(v)));
            break;
        }

        case OP_NOT_EQUAL:
        {
            BINARY_OP_PREP();
            PUSH(Value::makeBool(!valuesEqual(a, b)));
            break;
        }

        case OP_GREATER:
        {
            BINARY_OP_PREP();

            double da, db;
            if (!toNumberPair(a, b, da, db))
            {
                runtimeError("Operands must be numbers");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            PUSH(Value::makeBool(da > db));
            break;
        }

        case OP_GREATER_EQUAL:
        {
            BINARY_OP_PREP();

            double da, db;
            if (!toNumberPair(a, b, da, db))
            {
                runtimeError("Operands must be numbers");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }
            PUSH(Value::makeBool(da >= db));
            break;
        }

        case OP_LESS:
        {
            BINARY_OP_PREP();

            double da, db;
            if (!toNumberPair(a, b, da, db))
            {
                runtimeError("Operands must be numbers");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }
            PUSH(Value::makeBool(da < db));
            break;
        }

        case OP_LESS_EQUAL:
        {
            BINARY_OP_PREP();
            double da, db;
            if (!toNumberPair(a, b, da, db))
            {
                runtimeError("Operands must be numbers");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }
            PUSH(Value::makeBool(da <= db));
            break;
        }

            // ======= BITWISE =====

        case OP_BITWISE_AND:
        {
            BINARY_OP_PREP();
            if (!a.isInt() || !b.isInt())
            {
                runtimeError("Bitwise AND requires integers");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }
            PUSH(Value::makeInt(a.asInt() & b.asInt()));
            break;
        }

        case OP_BITWISE_OR:
        {
            BINARY_OP_PREP();
            if (!a.isInt() || !b.isInt())
            {
                runtimeError("Bitwise OR requires integers");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }
            PUSH(Value::makeInt(a.asInt() | b.asInt()));
            break;
        }

        case OP_BITWISE_XOR:
        {
            BINARY_OP_PREP();
            if (!a.isInt() || !b.isInt())
            {
                runtimeError("Bitwise XOR requires integers");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }
            PUSH(Value::makeInt(a.asInt() ^ b.asInt()));
            break;
        }

        case OP_BITWISE_NOT:
        {
            Value a = POP();
            if (!a.isInt())
            {
                runtimeError("Bitwise NOT requires integer");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }
            PUSH(Value::makeInt(~a.asInt()));
            break;
        }

        case OP_SHIFT_LEFT:
        {
            BINARY_OP_PREP();
            if (!a.isInt() || !b.isInt())
            {
                runtimeError("Shift left requires integers");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }
            PUSH(Value::makeInt(a.asInt() << b.asInt()));
            break;
        }

        case OP_SHIFT_RIGHT:
        {
            BINARY_OP_PREP();
            if (!a.isInt() || !b.isInt())
            {
                runtimeError("Shift right requires integers");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }
            PUSH(Value::makeInt(a.asInt() >> b.asInt()));
            break;
        }

            // ========== CONTROL FLOW ==========

        case OP_JUMP:
        {
            uint16 offset = READ_SHORT();
            ip += offset;
            break;
        }

        case OP_JUMP_IF_FALSE:
        {
            uint16 offset = READ_SHORT();
            if (isFalsey(PEEK()))
                ip += offset;
            break;
        }

        case OP_LOOP:
        {
            uint16 offset = READ_SHORT();

            ip -= offset;

            break;
        }

            // ========== FUNCTIONS ==========

        case OP_CALL:
        {
            uint8 argCount = READ_BYTE();

            STORE_FRAME();

            Value callee = NPEEK(argCount);

            // printf("Call : (");
            // printValue(callee);
            // printf(") count %d\n", argCount);

            if (callee.isFunction())
            {
                int index = callee.asFunctionId();

                Function *func = functions[index];
                if (!func)
                {
                    runtimeError("Invalid function");
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }

                if (argCount != func->arity)
                {
                    runtimeError("Expected %d arguments but got %d", func->arity, argCount);
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }

                if (fiber->frameCount >= FRAMES_MAX)
                {
                    runtimeError("Stack overflow");
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }

                CallFrame *newFrame = &fiber->frames[fiber->frameCount++];
                newFrame->func = func;
                newFrame->ip = func->chunk.code;
                newFrame->slots = fiber->stackTop - argCount; // Argumentos começam aqui
            }
            else if (callee.isNative())
            {
                int index = callee.asNativeId();
                NativeDef nativeFunc = natives[index];
                if (nativeFunc.arity != -1 && argCount != nativeFunc.arity)
                {
                    runtimeError("Function %s expected %d arguments but got %d",
                                 nativeFunc.name->chars(), nativeFunc.arity, argCount);
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }

                Value result = nativeFunc.func(this, argCount, fiber->stackTop - argCount);

                // Remove args + callee da stack
                fiber->stackTop -= (argCount + 1);

                // push resultado
                PUSH(result);
            }
            else if (callee.isProcess())
            {

                int index = callee.asProcessId();
                Process *blueprint = processes[index];

                if (!blueprint)
                {
                    runtimeError("Invalid process");
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }

                Function *processFunc = blueprint->fibers[0].frames[0].func;

                // Verifica arity
                if (argCount != processFunc->arity)
                {
                    runtimeError("Process expected %d arguments but got %d",
                                 processFunc->arity, argCount);
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }

                // SPAWN - clona blueprint
                Process *instance = spawnProcess(blueprint);

                // Se tem argumentos, inicializa locals da fiber
                if (argCount > 0)
                {
                    Fiber *procFiber = &instance->fibers[0];

                    for (int i = 0; i < argCount; i++)
                    {
                        procFiber->stack[i] = fiber->stackTop[-(argCount - i)];
                    }

                    procFiber->stackTop = procFiber->stack + argCount;

                    // Os argumentos viram locals[0], locals[1]...
                }

                // Remove callee + args da stack atual
                fiber->stackTop -= (argCount + 1);

                // Push ID do processo criado
                PUSH(Value::makeInt(instance->id));
            }
            else
            {

                runtimeError("Can only call functions");
                printValue(callee);
                printf("\n");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            LOAD_FRAME();
            break;
        }

            // case OP_RETURN:
            // {
            //     Value result = POP();

            //     Warning("Function return");

            //     // frame atual
            //     CallFrame *finished = &fiber->frames[fiber->frameCount - 1];

            //     // slot do callee no caller
            //     Value *resultSlot = finished->slots - 1;

            //     // remove frame
            //     fiber->frameCount--;

            //     // coloca resultado exatamente onde estava o callee
            //     fiber->stackTop = resultSlot;
            //     *fiber->stackTop++ = result;

            //     if (fiber->frameCount == 0)
            //     {
            //         fiber->state = FiberState::DEAD;
            //         STORE_FRAME();
            //         return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            //     }

            //     LOAD_FRAME();
            //     break;
            // }
        case OP_RETURN:
        {
            Value result = POP();

            CallFrame *finished = &fiber->frames[fiber->frameCount - 1];
            Value *resultSlot = finished->slots - 1;

            fiber->frameCount--;

            fiber->stackTop = resultSlot;
            *fiber->stackTop++ = result;

            if (fiber->frameCount == 0)
            {
                fiber->state = FiberState::DEAD;

                if (fiber == &currentProcess->fibers[0])
                {
                    // Mata TODAS as fibers do processo
                    for (int i = 0; i < currentProcess->nextFiberIndex; i++)
                    {
                        currentProcess->fibers[i].state = FiberState::DEAD;
                    }

                    // Marca processo como morto
                    currentProcess->state = FiberState::DEAD;

                    printf("          ");
                    for (Value *slot = fiber->stack; slot < fiber->stackTop; slot++)
                    {
                        printf("[ ");
                        printValue(*slot);
                        printf(" ]");
                    }
                    printf("\n");

                    // Chama hook onDestroy
                    // if (hooks.onDestroy)
                    // {
                    //     hooks.onDestroy(currentProcess, currentProcess->exitCode);
                    // }
                }

                STORE_FRAME();
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            LOAD_FRAME();
            break;
        }
            // ========== PROCESS/FIBER CONTROL ==========

        case OP_YIELD:
        {
            Value value = POP();
            float ms = value.isInt()
                           ? (float)value.asInt()
                           : (float)value.asDouble();

            STORE_FRAME();
            return {FiberResult::FIBER_YIELD, instructionsRun, ms, 0};
        }

        case OP_FRAME:
        {
            Value value = POP();
            int percent = value.isInt() ? value.asInt() : (int)value.asDouble();

            STORE_FRAME();
            return {FiberResult::PROCESS_FRAME, instructionsRun, 0, percent};
        }

        case OP_EXIT:
        {
            Value exitCode = POP();

            // Define exit code (int ou 0)
            currentProcess->exitCode = exitCode.isInt() ? exitCode.asInt() : 0;

            // Mata o processo
            currentProcess->state = FiberState::DEAD;

            // Mata todas as fibers (incluindo a atual)
            for (int i = 0; i < MAX_FIBERS; i++)
            {
                Fiber *f = &currentProcess->fibers[i];
                f->state = FiberState::DEAD;
                f->frameCount = 0;
                f->ip = nullptr;
                f->stackTop = f->stack; // stack limpa
            }

            // (Opcional) deixa o exitCode no topo da fiber atual para debug
            fiber->stackTop = fiber->stack;
            *fiber->stackTop++ = exitCode;

            STORE_FRAME();
            return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
        }
        case OP_SPAWN:
        {
            uint8 argCount = READ_BYTE();
            Value callee = NPEEK(argCount);

            if (!currentProcess)
            {
                runtimeError("No current process for spawn");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            if (currentProcess->nextFiberIndex >= MAX_FIBERS)
            {
                runtimeError("Too many fibers in process (max %d)", MAX_FIBERS);
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            if (!callee.isFunction())
            {
                runtimeError("fiber expects a function");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            int funcIndex = callee.asFunctionId();
            Function *func = functions[funcIndex];

            if (!func)
            {
                runtimeError("Invalid function");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            if (argCount != func->arity)
            {
                runtimeError("Expected %d arguments but got %d", func->arity, argCount);
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            int fiberIdx = currentProcess->nextFiberIndex++;
            Fiber *newFiber = &currentProcess->fibers[fiberIdx];

            newFiber->state = FiberState::RUNNING;
            newFiber->resumeTime = 0;
            newFiber->stackTop = newFiber->stack;
            newFiber->frameCount = 0;

            for (int i = 0; i < argCount; i++)
            {
                newFiber->stack[i] = fiber->stackTop[-(argCount - i)];
            }
            newFiber->stackTop = newFiber->stack + argCount;

            CallFrame *frame = &newFiber->frames[newFiber->frameCount++];
            frame->func = func;
            frame->ip = func->chunk.code;
            frame->slots = newFiber->stack; // Argumentos começam aqui

            fiber->stackTop -= (argCount + 1);

            PUSH(Value::makeInt(fiberIdx));

            break;
        }

            // ========== DEBUG ==========

        case OP_PRINT:
        {
            Value value = POP();
            printValue(value);
            printf("\n");
            break;
        }

            // ========== PROPERTY ACCESS ==========

        case OP_GET_PROPERTY:
        {
            Value object = PEEK();
            Value nameValue = READ_CONSTANT();

            printf("\nGet Object: '");
            printValue(object);
            printf("'\nName : '");
            printValue(nameValue);
            printf("'\n");

            if (!nameValue.isString())
            {
                runtimeError("Property name must be string");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            const char *name = nameValue.asStringChars();

            // === STRING METHODS ===
            if (object.isString())
            {

                if (strcmp(name, "length") == 0)
                {

                    DROP();
                    PUSH(Value::makeInt(object.asString()->length()));
                }
                else
                {

                    runtimeError("String has no property '%s'", name);
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }
            }

            // === PROCESS PRIVATES (external access) ===
            if (object.isProcess())
            {
                // uint32 processId = object.asProcessId();

                // // Busca processo na lista de vivos
                // Process *proc = nullptr;
                // for (size_t i = 0; i < aliveProcesses.size(); i++)
                // {
                //     if (aliveProcesses[i]->id == processId)
                //     {
                //         proc = aliveProcesses[i];
                //         break;
                //     }
                // }

                // if (!proc || proc->state == FiberState::DEAD)
                // {
                //     runtimeError("Process is dead or invalid");
                //     return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                // }

                // // Lookup private pelo nome
                // int privateIdx = getPrivateIndex(name);
                // if (privateIdx != -1)
                // {
                //     DROP(); // Remove process ID
                //     PUSH(proc->privates[privateIdx]);
                //     break;
                // }

                // runtimeError("Process has no property '%s'", name);
                // return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            // === OUTROS TIPOS (futuro) ===
            // runtimeError("Type does not support property access");
            // return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            break;
        }
        case OP_SET_PROPERTY:
        {
            // Stack: [object, value]
            Value value = PEEK();
            Value object = PEEK2();
            Value nameValue = READ_CONSTANT();

            printf("Set Value: '");
            printValue(value);
            printf("'\nObject: '");
            printValue(object);
            printf("'\nName : '");
            printValue(nameValue);
            printf("'\n");

            if (!nameValue.isString())
            {
                runtimeError("Property name must be string");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            // String *propName = nameValue.asString();
            // const char *name = propName->chars();

            // // === STRINGS (read-only) ===
            // if (object.isString())
            // {
            //     runtimeError("Cannot set property on string (immutable)");
            //     return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            // }

            // // === PROCESS PRIVATES (external write) ===
            // if (object.isProcess())
            // {
            //     uint32 processId = object.asProcessId();

            //     // Busca processo
            //     Process *proc = nullptr;
            //     for (size_t i = 0; i < aliveProcesses.size(); i++)
            //     {
            //         if (aliveProcesses[i]->id == processId)
            //         {
            //             proc = aliveProcesses[i];
            //             break;
            //         }
            //     }

            //     if (!proc || proc->state == FiberState::DEAD)
            //     {
            //         runtimeError("Process is dead or invalid");
            //         return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            //     }

            //     // Lookup private pelo nome
            //     int privateIdx = getPrivateIndex(name);
            //     if (privateIdx != -1)
            //     {
            //         proc->privates[privateIdx] = value;
            //         DROP();      // Remove value
            //         DROP();      // Remove process
            //         PUSH(value); // Assignment retorna valor
            //         break;
            //     }

            //     runtimeError("Process has no property '%s'", name);
            //     return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            // }

            // runtimeError("Cannot set property on this type");
            // return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};

            break;
        }
        case OP_INVOKE:
        {
            Value nameValue = READ_CONSTANT();
            uint8_t argCount = READ_BYTE();

            if (!nameValue.isString())
            {
                runtimeError("Method name must be string");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            const char *name = nameValue.asStringChars();
            Value receiver = NPEEK(argCount);

            //             printf("\n=== OP_INVOKE DEBUG ===\n");
            // printf("Method: %s\n", name);
            // printf("ArgCount: %d\n", argCount);
            // printf("Stack size: %d\n", (int)(fiber->stackTop - fiber->stack));
            // printf("Receiver (NPEEK(%d)): ", argCount);
            // printValue(receiver);
            // printf("\n");
            // printf("Type: ");
            // if (receiver.isString()) printf("STRING");
            // else if (receiver.isInt()) printf("INT");
            // else if (receiver.isDouble()) printf("DOUBLE");
            // else if (receiver.isNil()) printf("NIL");
            // else printf("UNKNOWN");
            // printf("\n");

            // // Mostra o stack todo
            // printf("Full stack:\n");
            // for (int i = 0; i < (fiber->stackTop - fiber->stack); i++) {
            //     printf("  [%d] ", i);
            //     printValue(fiber->stack[i]);
            //     printf("\n");
            // }
            // printf("=======================\n");
#define ARGS_CLEANUP() fiber->stackTop -= (argCount + 1)

            // === STRING METHODS ===
            if (receiver.isString())
            {
                String *str = receiver.asString();

                if (strcmp(name, "length") == 0)
                {
                    int len = str->length();
                    ARGS_CLEANUP();
                    PUSH(Value::makeInt(len));
                }
                else if (strcmp(name, "upper") == 0)
                {
                    ARGS_CLEANUP();
                    PUSH(Value::makeString(StringPool::instance().upper(str)));
                }
                else if (strcmp(name, "lower") == 0)
                {
                    ARGS_CLEANUP();
                    PUSH(Value::makeString(StringPool::instance().lower(str)));
                }
                else if (strcmp(name, "concat") == 0)
                {
                    if (argCount != 1)
                    {
                        runtimeError("concat() expects 1 argument");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    Value arg = PEEK();
                    if (!arg.isString())
                    {
                        runtimeError("concat() expects string argument");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    String *result = StringPool::instance().concat(str, arg.asString());
                    ARGS_CLEANUP();
                    PUSH(Value::makeString(result));
                }
                else if (strcmp(name, "sub") == 0)
                {
                    if (argCount != 2)
                    {
                        runtimeError("sub() expects 2 arguments");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    Value start = PEEK2();
                    Value end = PEEK();

                    if (!start.isNumber() || !end.isNumber())
                    {
                        runtimeError("sub() expects 2 number arguments");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    String *result = StringPool::instance().substring(
                        str,
                        (uint32_t)start.asNumber(),
                        (uint32_t)end.asNumber());
                    ARGS_CLEANUP();
                    PUSH(Value::makeString(result));
                }
                else if (strcmp(name, "replace") == 0)
                {
                    if (argCount != 2)
                    {
                        runtimeError("replace() expects 2 arguments");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    Value oldStr = PEEK2();
                    Value newStr = PEEK();

                    if (!oldStr.isString() || !newStr.isString())
                    {
                        runtimeError("replace() expects 2 string arguments");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    String *result = StringPool::instance().replace(
                        str,
                        oldStr.asStringChars(),
                        newStr.asStringChars());
                    ARGS_CLEANUP();
                    PUSH(Value::makeString(result));
                }
                else if (strcmp(name, "at") == 0)
                {
                    if (argCount != 1)
                    {
                        runtimeError("at() expects 1 argument");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    Value index = PEEK();
                    if (!index.isNumber())
                    {
                        runtimeError("at() expects number argument");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    String *result = StringPool::instance().at(str, (int)index.asNumber());
                    ARGS_CLEANUP();
                    PUSH(Value::makeString(result));
                }

                else if (strcmp(name, "contains") == 0)
                {
                    if (argCount != 1)
                    {
                        runtimeError("contains() expects 1 argument");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    Value substr = PEEK();
                    if (!substr.isString())
                    {
                        runtimeError("contains() expects string argument");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    bool result = StringPool::instance().contains(str, substr.asString());
                    ARGS_CLEANUP();
                    PUSH(Value::makeBool(result));
                }

                else if (strcmp(name, "trim") == 0)
                {
                    String *result = StringPool::instance().trim(str);
                    ARGS_CLEANUP();
                    PUSH(Value::makeString(result));
                }

                else if (strcmp(name, "starts_with") == 0 || strcmp(name, "startsWith") == 0)
                {
                    if (argCount != 1)
                    {
                        runtimeError("starts_with() expects 1 argument");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    Value prefix = PEEK();
                    if (!prefix.isString())
                    {
                        runtimeError("starts_with() expects string argument");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    bool result = StringPool::instance().startsWith(str, prefix.asString());
                    ARGS_CLEANUP();
                    PUSH(Value::makeBool(result));
                }

                else if (strcmp(name, "ends_with") == 0 || strcmp(name, "endsWith") == 0)
                {
                    if (argCount != 1)
                    {
                        runtimeError("ends_with() expects 1 argument");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    Value suffix = PEEK();
                    if (!suffix.isString())
                    {
                        runtimeError("ends_with() expects string argument");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    bool result = StringPool::instance().endsWith(str, suffix.asString());
                    ARGS_CLEANUP();
                    PUSH(Value::makeBool(result));
                }

                else if (strcmp(name, "index_of") == 0 || strcmp(name, "indexOf") == 0)
                {
                    if (argCount < 1 || argCount > 2)
                    {
                        runtimeError("index_of() expects 1 or 2 arguments");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    Value substr;
                    int startIndex = 0;

                    if (argCount == 1)
                    {
                        // index_of(substr)
                        substr = PEEK();
                    }
                    else
                    {
                        // index_of(substr, startIndex)
                        Value startVal = PEEK();
                        substr = PEEK2();

                        if (!startVal.isNumber())
                        {
                            runtimeError("index_of() startIndex must be number");
                            return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                        }

                        startIndex = (int)startVal.asNumber();
                    }

                    if (!substr.isString())
                    {
                        runtimeError("index_of() expects string argument");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    int result = StringPool::instance().indexOf(
                        str,
                        substr.asString(),
                        startIndex);
                    ARGS_CLEANUP();
                    PUSH(Value::makeInt(result));
                }
                else if (strcmp(name, "repeat") == 0)
                {
                    if (argCount != 1)
                    {
                        runtimeError("repeat() expects 1 argument");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    Value count = PEEK();
                    if (!count.isNumber())
                    {
                        runtimeError("repeat() expects number argument");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    String *result = StringPool::instance().repeat(str, (int)count.asNumber());
                    ARGS_CLEANUP();
                    PUSH(Value::makeString(result));
                }
                else
                {
                    runtimeError("String has no method '%s'", name);
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }
                break;
            }

            runtimeError("Type does not support method calls");
            return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
        }
        default:
        {
            runtimeError("Unknown opcode %d", instruction);
            break;
        }
        }
    }

    // Cleanup macros

#undef READ_BYTE
#undef READ_SHORT
}