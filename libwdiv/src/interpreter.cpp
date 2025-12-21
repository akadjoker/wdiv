#include "interpreter.hpp"
#include "compiler.hpp"
#include "pool.hpp"
#include "opcode.hpp"
#include "debug.hpp"
#include <new>
#include <stdarg.h>
#include <cmath> // std::fmod

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
    // for (size_t j = 0; j < cleanProcesses.size(); j++)
    // {
    //     Process* proc = cleanProcesses[j];
    //     arena.Free(proc, sizeof(Process));

    // }
    // cleanProcesses.clear();

    arena.Clear();
}

void Interpreter::print(Value value)
{
    printValue(value);
}

Fiber* Interpreter::get_ready_fiber(Process* proc)
{
    int checked = 0;
    int totalFibers = proc->nextFiberIndex;
    
    if (totalFibers == 0)
        return nullptr;
    
    //printf("[get_ready_fiber] Checking %d fibers, time=%.3f\n", totalFibers, currentTime);
    
    while (checked < totalFibers)
    {
        int idx = proc->currentFiberIndex;
        proc->currentFiberIndex = (proc->currentFiberIndex + 1) % totalFibers;
        
        Fiber* f = &proc->fibers[idx];
        
        //printf("  Fiber %d: state=%d, resumeTime=%.3f\n", idx, (int)f->state, f->resumeTime);
        
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
                //printf("  -> RESUMING (%.3f >= %.3f)\n", currentTime, f->resumeTime);
                f->state = FiberState::RUNNING;
                return f;
            }
            //printf("  -> Still suspended (wait %.3fms)\n", (f->resumeTime - currentTime) * 1000);
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

    // #ifdef WDIV_DEBUG
    CallFrame *frame = &currentFiber->frames[currentFiber->frameCount - 1];
    Debug::dumpFunction(frame->func);
    // #endif

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

bool Interpreter::callValue(Value callee, int argCount)
{
    if (!callee.isFunction())
    {
        runtimeError("Can only call functions");
        return false;
    }

    int index = callee.asFunctionId();

    Function *func = functions[index];
    if (!func)
        return false;

    if (argCount != func->arity)
    {
        runtimeError("Expected %d arguments but got %d",
                     func->arity, argCount);
        return false;
    }

    if (currentFiber->frameCount >= FRAMES_MAX)
    {
        runtimeError("Stack overflow");
        return false;
    }

    CallFrame *frame = &currentFiber->frames[currentFiber->frameCount++];
    frame->func = func;
    frame->ip = func->chunk.code;
    frame->slots = currentFiber->stackTop - argCount - 1;

    currentFiber->ip = func->chunk.code;

    return true;
}

Function *Interpreter::compile(const char *source)
{
    return compiler->compile(source, this);
}

Function *Interpreter::compileExpression(const char *source)
{
    return compiler->compileExpression(source, this);
}
bool Interpreter::run(const char *source)
{
    Process *proc = compiler->compile(source);
    if (!proc)
    {
        return false;
    }
    mainProcess = spawnProcess(proc);
    currentProcess = mainProcess;

    Fiber *fiber = &mainProcess->fibers[0];

    run_fiber(fiber, 30);

    //     Debug::dumpFunction(fiber->frames[0].func);

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

void Interpreter::render()
{
    if (!hooks.onRender)
        return;

    for (size_t i = 0; i < aliveProcesses.size(); i++)
    {
        Process *p = aliveProcesses[i];
        if (p->state == FiberState::DEAD)
            continue;
        hooks.onRender(p);
    }
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

FiberResult Interpreter::run_fiber(Fiber *fiber, int maxInstructions)
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
        // Limite de instruções (time slice)
        if (++instructionsRun > maxInstructions)
        {
            STORE_FRAME();
            fiber->ip = ip;
            fiber->stackTop = fiber->stackTop;
            return {FiberResult::FIBER_YIELD, instructionsRun, 0, 0};
        }

        // Debug: mostra stack e instrução
        // printf("          ");
        // for (Value *slot = fiber->stack; slot < fiber->stackTop; slot++)
        // {
        //     printf("[ ");
        //     printValue(*slot);
        //     printf(" ]");
        // }
        // printf("\n");

        //  size_t offset = ip - func->chunk.code;
        //  printf("%04zu ", offset);
        //  Debug::disassembleInstruction(func->chunk, offset);

        uint8 instruction = READ_BYTE();

        // printf("[DEBUG] IP offset=%ld, opcode=%d\n",
        //        (ip-1) - func->chunk.code, instruction);

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

                    // Copia argumentos da stack atual para a stack do processo
                    for (int i = 0; i < argCount; i++)
                    {
                        procFiber->stack[i] = fiber->stackTop[-(argCount - i)];
                    }

                    // Ajusta stackTop da nova fiber
                    procFiber->stackTop = procFiber->stack + argCount;

                    // Os argumentos viram locals[0], locals[1]...
                    // quando a função do processo executar
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

        case OP_RETURN:
        {
            Value result = POP();

            // frame atual
            CallFrame *finished = &fiber->frames[fiber->frameCount - 1];

            // slot do callee no caller
            Value *resultSlot = finished->slots - 1;

            // remove frame
            fiber->frameCount--;

            // coloca resultado exatamente onde estava o callee
            fiber->stackTop = resultSlot;
            *fiber->stackTop++ = result;

            if (fiber->frameCount == 0)
            {
                fiber->state = FiberState::DEAD;
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
            int percent = value.isInt()
                              ? value.asInt()
                              : (int)value.asDouble();

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

            // ✅ Pega função pelo ID do callee
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

            // ✅ Cria nova fiber no processo
            int fiberIdx = currentProcess->nextFiberIndex++;
            Fiber *newFiber = &currentProcess->fibers[fiberIdx];

            // ✅ Inicializa fiber
            newFiber->state = FiberState::RUNNING;
            newFiber->resumeTime = 0;
            newFiber->stackTop = newFiber->stack;
            newFiber->frameCount = 0;

            // ✅ Copia argumentos para a stack da nova fiber
            for (int i = 0; i < argCount; i++)
            {
                newFiber->stack[i] = fiber->stackTop[-(argCount - i)];
            }
            newFiber->stackTop = newFiber->stack + argCount;

            // ✅ Cria call frame inicial
            CallFrame *frame = &newFiber->frames[newFiber->frameCount++];
            frame->func = func;
            frame->ip = func->chunk.code;
            frame->slots = newFiber->stack; // Argumentos começam aqui

            // ✅ Remove callee + args da stack atual
            fiber->stackTop -= (argCount + 1);

            // ✅ Push fiber index como resultado (opcional)
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