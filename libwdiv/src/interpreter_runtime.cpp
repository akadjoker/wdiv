#include "interpreter.hpp"
#include "pool.hpp"
#include "opcode.hpp"
#include "debug.hpp"
#include <cmath> // std::fmod
#include <new>

#define DEBUG_TRACE_EXECUTION 0 // 1 = ativa, 0 = desativa
#define DEBUG_TRACE_STACK 0     // 1 = mostra stack, 0 = esconde

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

    // if (frame->ip == nullptr)
    // {
    //     runtimeError("FATAL: frame->ip is NULL! Code was not initialized properly.");
    //     return {FiberResult::FIBER_DONE, 0, 0, 0};
    // }

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

#define READ_CONSTANT() (func->chunk->constants[READ_BYTE()])
    LOAD_FRAME();

    // printf("[DEBUG] Starting run_fiber: ip=%p, func=%s, offset=%ld\n",
    //        (void*)ip, func->name->chars(), ip - func->chunk->code);

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
        size_t offset = ip - func->chunk->code;
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
        {
            DROP();

            break;
        }

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
            Value value = PEEK();
            if (globals.set(name.asString(), value))
            {
            }
            else
            {
            }
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
                String *result = StringPool::instance().concat(a.asString(), b.asString());
                PUSH(Value::makeString(result));
                break;
            }

            if (a.isString() && b.isDouble())
            {
                String *right = StringPool::instance().toString(b.asDouble());
                String *result = StringPool::instance().concat(a.asString(), right);
                PUSH(Value::makeString(result));
                break;
            }
            if (a.isString() && b.isInt())
            {
                String *right = StringPool::instance().toString(b.asInt());
                String *result = StringPool::instance().concat(a.asString(), right);
                PUSH(Value::makeString(result));
                break;
            }

            if (a.isInt() && b.isInt())
            {
                PUSH(Value::makeInt(a.asInt() + b.asInt()));
                break;
            }

            if (a.isInt() && b.isDouble())
            {
                PUSH(Value::makeInt(a.asInt() + b.asDouble()));
                // PUSH(Value::makeDouble(a.asInt() + b.asDouble()));
                break;
            }

            if (a.isDouble() && b.isInt())
            {
                PUSH(Value::makeDouble(a.asDouble() + b.asInt()));
                break;
            }

            double da, db;
            if (!toNumberPair(a, b, da, db))
            {
                runtimeError("Operands 'add' must be numbers or strings");
                printf(" a: ");
                printValue(a);
                printf(" b: ");
                printValue(b);
                printf("\n");

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
            //  printValue(callee);
            //  printf(") count %d\n", argCount);

            if (callee.isFunction())
            {
                int index = callee.asFunctionId();

                Function *func = functions[index];
                if (!func)
                {
                    runtimeError("Invalid function");
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }

            //    Debug::dumpFunction(func);

                if (argCount != func->arity)
                {
                    runtimeError("Function %s expected %d arguments but got %d",func->name->chars(), func->arity, argCount);
                    for (int i = 0; i < argCount; i++)
                    {
                        printValue(NPEEK(i));
                        printf(" ");
                    }
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }

                if (fiber->frameCount >= FRAMES_MAX)
                {
                    runtimeError("Stack overflow");
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }

                CallFrame *newFrame = &fiber->frames[fiber->frameCount++];
                newFrame->func = func;
                newFrame->ip = func->chunk->code;
                newFrame->slots = fiber->stackTop - argCount ; // Argumentos começam aqui
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
                ProcessDef *blueprint = processes[index];

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
                        if (blueprint->argsNames.size() > 0)
                        {
                            uint8 index = blueprint->argsNames[i];
                            if (index != 255)
                            {
                                instance->privates[index] = procFiber->stack[i];
                            }
                        }
                    }

                    procFiber->stackTop = procFiber->stack + argCount;

                    // Os argumentos viram locals[0], locals[1]...
                }

                // Remove callee + args da stack atual
                fiber->stackTop -= (argCount + 1);

                if (currentProcess->id == 0)
                {
                }

                instance->privates[(int)PrivateIndex::ID] = Value::makeInt(instance->id);
                instance->privates[(int)PrivateIndex::FATHER] = Value::makeProcess(currentProcess->id);

                if (hooks.onStart)
                {
                    hooks.onStart(instance);
                }

                // Push ID do processo criado
                PUSH(Value::makeInt(instance->id));
            }
            else if (callee.isStruct())
            {
                int index = callee.as.integer;

                StructDef *def = structs[index];

                if (argCount != def->argCount)
                {
                    runtimeError("Struct '%s' expects %zu arguments, got %d", def->name->chars(), def->argCount, argCount);
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }

                Value value = Value::makeStructInstance();
                StructInstance *instance = value.as.sInstance;
                instance->def = def;
                structInstances.push(instance);
                instance->values.reserve(argCount);
                for (int i = argCount - 1; i >= 0; i--)
                {
                    instance->values[i] = POP();
                }
                POP();
                PUSH(value);
                break;
            }
            else if (callee.isClass())
            {
                int classId = callee.asClassId();
                ClassDef *klass = classes[classId];

                Value value = Value::makeClassInstance();
                ClassInstance *instance = value.asClassInstance();
                instance->klass = klass;
                instance->fields.reserve(klass->fieldCount);

                // Inicializa fields com nil
                for (int i = 0; i < klass->fieldCount; i++)
                {
                    instance->fields.push(Value::makeNil());
                }

             
                // Substitui class por instance na stack
                fiber->stackTop[-argCount - 1] = value;

                if (klass->constructor)
                {
                    if (argCount != klass->constructor->arity)
                    {
                        runtimeError("init() expects %d arguments, got %d", klass->constructor->arity, argCount);
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    if (currentFiber->frameCount >= FRAMES_MAX)
                    {
                        runtimeError("Stack overflow");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    CallFrame *newFrame = &currentFiber->frames[currentFiber->frameCount];
                    newFrame->func = klass->constructor;
                    newFrame->ip = klass->constructor->chunk->code;
                    newFrame->slots = currentFiber->stackTop - argCount - 1;

                    currentFiber->frameCount++;

                    STORE_FRAME();
                    LOAD_FRAME();
                }
                else
                {
                    // Sem init - pop args
                    fiber->stackTop -= argCount;
                }

                break;
            }
            else if (callee.isNativeClass())
            {
                int classId = callee.asClassNativeId();
                NativeClassDef *klass = nativeClasses[classId];

                if (argCount != klass->argCount)
                {
                    runtimeError("Native class expects %d args, got %d", klass->argCount, argCount);
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }

                Value *args = fiber->stackTop - argCount;
                void *userData = klass->constructor(this, argCount, args);

                if (!userData)
                {
                    runtimeError("Failed to create native '%s' instance", klass->name->chars());
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }
                Value literal = Value::makeNativeClassInstance();
                // Cria instance wrapper
                NativeInstance *instance = literal.as.sClassInstance;
                nativeInstances.push(instance);
                instance->klass = klass;
                instance->userData = userData;
                instance->refCount = 1;
                // Remove args + callee, push instance
                fiber->stackTop -= (argCount + 1);
                PUSH(literal);

                break;
            }

            else if (callee.isNativeStruct())
            {
                int structId = callee.asNativeStructId();
                NativeStructDef *def = nativeStructs[structId];

                void *data = heapAllocator.Allocate(def->structSize);
                std::memset(data, 0, def->structSize);
                if (def->constructor)
                {
                    Value *args = fiber->stackTop - argCount;
                    def->constructor(this, data, argCount, args);
                }

                Value literal = Value::makeNativeStructInstance();
                // Cria instance wrapper
                NativeStructInstance *instance = literal.as.sNativeStruct;
                nativeStructInstances.push(instance);
                instance->def = def;
                instance->data = data;

                // Remove args + callee, push instance
                fiber->stackTop -= (argCount + 1);
                PUSH(literal);

                break;
            }
            else
            {

                runtimeError("Can only call functions");
                printf("> ");
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

            fiber->frameCount--;

            if (fiber->frameCount == 0)
            {
                fiber->stackTop = fiber->stack;
                *fiber->stackTop++ = result;

                fiber->state = FiberState::DEAD;

                if (fiber == &currentProcess->fibers[0])
                {
                    for (int i = 0; i < currentProcess->nextFiberIndex; i++)
                    {
                        currentProcess->fibers[i].state = FiberState::DEAD;
                    }
                    currentProcess->state = FiberState::DEAD;
                }

                STORE_FRAME();
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            //  Função nested - retorna para onde estava a chamada
            CallFrame *finished = &fiber->frames[fiber->frameCount];
            
            

            fiber->stackTop = finished->slots;
            *fiber->stackTop++ = result;
            //fiber->stackTop[-1] = result;

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
            frame->ip = func->chunk->code;
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

            // printf("\nGet Object: '");
            // printValue(object);
            // printf("'\nName : '");
            // printValue(nameValue);
            // printf("'\n");

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

                int processId = object.asProcessId();

                Process *proc = aliveProcesses[processId];
                if (!proc)
                {
                    runtimeError("Process '%i' is dead or invalid", processId);
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }
                int privateIdx = getProcessPrivateIndex(name);
                if (privateIdx != -1)
                {
                    PUSH(proc->privates[privateIdx]);
                }
                else
                {
                    runtimeError("Proces  does not support '%s' property access", name);
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }

                break;
            }

            if (object.isStructInstance())
            {

                StructInstance *inst = object.asStructInstance();
                if (!inst)
                {
                    runtimeError("Struct is null");
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }
                uint8 value = 0;
                if (inst->def->names.get(nameValue.asString(), &value))
                {

                    DROP();
                    PUSH(inst->values[value]);
                }
                else
                {
                    runtimeError("Struct '%s' has no field '%s'", inst->def->name->chars(), name);
                    PUSH(Value::makeNil());
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }
                break;
            }

            if (object.isClassInstance())
            {
                ClassInstance *instance = object.asClassInstance();

                // bool inherited = instance->klass->inherited;

                uint8_t fieldIdx;
                if (instance->klass->fieldNames.get(nameValue.asString(), &fieldIdx))
                {
                    DROP();
                    PUSH(instance->fields[fieldIdx]);
                    break;
                }
                runtimeError("Undefined property '%s'", name);
                PUSH(Value::makeNil());
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            if (object.isNativeClassInstance())
            {

                NativeInstance *instance = object.asNativeClassInstance();
                NativeClassDef *klass = instance->klass;
                NativeProperty prop;
                if (instance->klass->properties.get(nameValue.asString(), &prop))
                {
                    DROP(); // Remove object

                    //  Chama getter
                    Value result = prop.getter(this, instance->userData);
                    PUSH(result);
                    break;
                }

                runtimeError("Undefined property '%s' on native class '%s", nameValue.asStringChars(), klass->name->chars());
                DROP();
                PUSH(Value::makeNil());
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            if (object.isNativeStructInstance())
            {

                NativeStructInstance *inst = object.asNativeStructInstance();
                NativeStructDef *def = inst->def;

                NativeFieldDef field;

                if (!def->fields.get(nameValue.asString(), &field))
                {
                    runtimeError("Undefined field '%s' on native struct '%s", nameValue.asStringChars(), def->name->chars());
                    DROP();
                    PUSH(Value::makeNil());
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }
                char *base = (char *)inst->data;
                char *ptr = base + field.offset;

                Value result;
                switch (field.type)
                {
                case FieldType::INT:
                    result = Value::makeInt(*(int *)ptr);
                    break;

                case FieldType::FLOAT:
                case FieldType::DOUBLE:
                    result = Value::makeDouble(*(double *)ptr);
                    break;

                case FieldType::BOOL:
                    result = Value::makeBool(*(bool *)ptr);
                    break;

                case FieldType::POINTER:
                    result = Value::makePointer(*(void **)ptr);
                    break;

                case FieldType::STRING:
                {
                    String *str = *(String **)ptr;
                    result = str ? Value::makeString(str) : Value::makeNil();
                    break;
                }
                }

                DROP(); // Remove object
                PUSH(result);
                break;
            }

            runtimeError("Type does not support 'set' property access");

            printf("[Object: '");
            printValue(object);
            printf("' Property : '");
            printValue(nameValue);

            printf("']\n");

            PUSH(Value::makeNil());
            return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
        }
        case OP_SET_PROPERTY:
        {
            // Stack: [object, value]
            Value value = PEEK();
            Value object = PEEK2();
            Value nameValue = READ_CONSTANT();

            // printf("Set Value: '");
            // printValue(value);
            // printf("'\nObject: '");
            // printValue(object);
            // printf("'\nName : '");
            // printValue(nameValue);
            // printf("'\n");

            if (!nameValue.isString())
            {
                runtimeError("Property name must be string");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            String *propName = nameValue.asString();
            const char *name = propName->chars();

            // === STRINGS (read-only) ===
            if (object.isString())
            {
                runtimeError("Cannot set property on string (immutable)");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            // === PROCESS PRIVATES (external write) ===
            if (object.isProcess())
            {
                int processId = object.asProcessId();
                Process *proc = aliveProcesses[processId];

                if (!proc) // || proc->state == FiberState::DEAD)
                {
                    runtimeError("Process '%i' is dead or invalid", processId);
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }

                // Lookup private pelo nome
                int privateIdx = getProcessPrivateIndex(name);
                if (privateIdx != -1)
                {
                    if ((privateIdx == (int)PrivateIndex::ID) || (privateIdx == (int)PrivateIndex::FATHER))
                    {
                        runtimeError("Property '%s' is readonly", name);
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }
                    proc->privates[privateIdx] = value;
                    DROP();      // Remove value
                    DROP();      // Remove process
                    PUSH(value); // Assignment retorna valor
                    break;
                }

                runtimeError("Process has no property '%s'", name);
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            if (object.isStructInstance())
            {

                StructInstance *inst = object.asStructInstance();
                if (!inst)
                {
                    runtimeError("Struct is null ");
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }

                uint8 valueIndex = 0;
                if (inst->def->names.get(nameValue.asString(), &valueIndex))
                {
                    inst->values[valueIndex] = value;
                }
                else
                {
                    runtimeError("Struct '%s' has no field '%s'", inst->def->name->chars(), name);
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }

                // Pop object, keep value
                DROP(); // object

                break;
            }

            if (object.isClassInstance())
            {
                ClassInstance *instance = object.asClassInstance();

                uint8_t fieldIdx;
                if (instance->klass->fieldNames.get(nameValue.asString(), &fieldIdx))
                {
                    instance->fields[fieldIdx] = value;
                    // Pop object, mantém value
                    DROP(); // object
                    break;
                }

                runtimeError("Undefined property '%s'", name);
                DROP(); // object
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            if (object.isNativeClassInstance())
            {

                NativeInstance *instance = object.asNativeClassInstance();
                NativeClassDef *klass = instance->klass;
                NativeProperty prop;
                if (instance->klass->properties.get(nameValue.asString(), &prop))
                {
                    if (!prop.setter)
                    {
                        runtimeError("Property '%s' from class '%s' is read-only", nameValue.asStringChars(), klass->name->chars());
                        DROP();
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    // Chama setter
                    prop.setter(this, instance->userData, value);
                    DROP(); // Remove object
                    break;
                }
            }

            if (object.isNativeStructInstance())
            {
                NativeStructInstance *inst = object.asNativeStructInstance();
                NativeStructDef *def = inst->def;

                NativeFieldDef field;
                if (!def->fields.get(nameValue.asString(), &field))
                {
                    runtimeError("Undefined field '%s' in struct '%s", nameValue.asStringChars(), def->name->chars());
                    DROP();
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }

                // Read-only check
                if (field.readOnly)
                {
                    runtimeError("Field '%s' is read-only in struct '%s", nameValue.asStringChars(), def->name->chars());
                    DROP();
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }
                char *base = (char *)inst->data;
                char *ptr = base + field.offset;
                switch(field.type)
                {
                case FieldType::INT:
                    if (!value.isInt())
                    {
                        runtimeError("Field expects int");
                        DROP();
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }
                    *(int *)ptr = value.asInt();
                    break;

                case FieldType::FLOAT:
                case FieldType::DOUBLE:
                    if (!value.isDouble() && !value.isInt())
                    {
                        runtimeError("Field expects number");
                        DROP();
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }
                    *(double *)ptr = value.isDouble() ? value.asDouble() : (double)value.asInt();
                    break;

                case FieldType::BOOL:
                    if (!value.isBool())
                    {
                        runtimeError("Field expects bool");
                        DROP();
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }
                    *(bool *)ptr = value.asBool();
                    break;

                case FieldType::POINTER:
                    if (!value.isPointer() && !value.isNil())
                    {
                        runtimeError("Field expects pointer");
                        DROP();
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }
                    *(void **)ptr = value.isPointer() ? value.asPointer() : nullptr;
                    break;

                case FieldType::STRING:
                {
                    if (!value.isString() && !value.isNil())
                    {
                        runtimeError("Field expects string");
                        DROP();
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }
                    String **fieldPtr = (String **)ptr;
                    *fieldPtr = value.isString() ? value.asString() : nullptr;
                    break;
                }
                }

                DROP(); // Remove object
                break;
            }

            runtimeError("Cannot 'set' property on this type");
            printf("[Object: '");
            printValue(object);
            printf("' Property : '");
            printValue(nameValue);

            printf("']\n");

            return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};

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

            // === ARRAY METHODS ===
            if (receiver.isArray())
            {
                ArrayInstance *arr = receiver.asArray();
                uint32 size = arr->values.size();
                if (strcmp(name, "push") == 0)
                {
                    if (argCount != 1)
                    {
                        runtimeError("push() expects 1 argument");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }
                    Value item = PEEK();
                    arr->values.push(item);
                    ARGS_CLEANUP();
                    PUSH(Value::makeNil());
                    break;
                }
                else if (strcmp(name, "pop") == 0)
                {
                    if (argCount != 0)
                    {
                        runtimeError("pop() expects 0 arguments");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    if (size == 0)
                    {
                        Warning("Cannot pop from empty array");
                        ARGS_CLEANUP();
                        PUSH(Value::makeNil());
                        break;
                    }
                    else
                    {
                        Value result = arr->values.back();
                        arr->values.pop();
                        ARGS_CLEANUP();
                        PUSH(result);
                    }
                    break;
                }
                else if (strcmp(name, "back") == 0)
                {
                    if (argCount != 0)
                    {
                        runtimeError("back() expects 0 arguments");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    if (size == 0)
                    {
                        Warning("Cannot pop from empty array");
                        ARGS_CLEANUP();
                        PUSH(Value::makeNil());
                        break;
                    }
                    else
                    {
                        Value result = arr->values.back();
                        ARGS_CLEANUP();
                        PUSH(result);
                    }
                    break;
                }
                else if (strcmp(name, "len") == 0 || strcmp(name, "length") == 0)
                {
                    if (argCount != 0)
                    {
                        runtimeError("len() expects 0 arguments");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    ARGS_CLEANUP();
                    PUSH(Value::makeInt(size));
                    break;
                }
                else if (strcmp(name, "clear") == 0)
                {
                    if (argCount != 0)
                    {
                        runtimeError("clear() expects 0 arguments");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }
                    arr->values.clear();
                    ARGS_CLEANUP();
                    PUSH(Value::makeNil());
                    break;
                }
                else
                {
                    runtimeError("Array has no method '%s'", name);
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }
            }

            // === MAP METHODS ===
            if (receiver.isMap())
            {
                MapInstance *map = receiver.asMap();

                if (strcmp(name, "has") == 0)
                {
                    if (argCount != 1)
                    {
                        runtimeError("has() expects 1 argument");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    Value key = PEEK();

                    if (!key.isString())
                    {
                        runtimeError("Map key must be string");
                        ARGS_CLEANUP();
                        PUSH(Value::makeBool(false));
                        break;
                    }

                    bool exists = map->table.exist(key.asString());
                    ARGS_CLEANUP();
                    PUSH(Value::makeBool(exists));
                    break;
                }
                else if (strcmp(name, "remove") == 0)
                {
                    if (argCount != 1)
                    {
                        runtimeError("remove() expects 1 argument");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    Value key = PEEK();

                    if (!key.isString())
                    {
                        runtimeError("Map key must be string");
                        ARGS_CLEANUP();
                        PUSH(Value::makeNil());
                        break;
                    }

                    //  HashMap não tem remove, mas podes setar para nil
                    map->table.set(key.asString(), Value::makeNil());
                    ARGS_CLEANUP();
                    PUSH(Value::makeNil());
                    break;
                }
                else if (strcmp(name, "clear") == 0)
                {
                    if (argCount != 0)
                    {
                        runtimeError("clear() expects 0 arguments");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    map->table.destroy();
                    ARGS_CLEANUP();
                    PUSH(Value::makeNil());
                    break;
                }
                else if (strcmp(name, "len") == 0 || strcmp(name, "length") == 0)
                {
                    if (argCount != 0)
                    {
                        runtimeError("len() expects 0 arguments");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }
                    ARGS_CLEANUP();
                    PUSH(Value::makeInt(map->table.count));
                    break;
                }
                else if (strcmp(name, "keys") == 0)
                {
                    if (argCount != 0)
                    {
                        runtimeError("keys() expects 0 arguments");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    // Retorna array de keys
                    Value keys = Value::makeArray();
                    ArrayInstance *keysInstance = keys.asArray();

                    map->table.forEach([&](String *key, Value value)
                                       { keysInstance->values.push(Value::makeString(key)); });

                    ARGS_CLEANUP();
                    PUSH(keys);
                    break;
                }
                else if (strcmp(name, "values") == 0)
                {
                    if (argCount != 0)
                    {
                        runtimeError("values() expects 0 arguments");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    // Retorna array de values
                    Value values = Value::makeArray();
                    ArrayInstance *valueInstance = values.asArray();

                    map->table.forEach([&](String *key, Value value)
                                       { valueInstance->values.push(value); });

                    ARGS_CLEANUP();
                    PUSH(values);
                    break;
                }
            }

            // === CLASS INSTANCE METHODS ===
            if (receiver.isClassInstance())
            {
                ClassInstance *instance = receiver.asClassInstance();
                // printValueNl(receiver);
                // printValueNl(nameValue);

                Function *method;
                if (instance->getMethod(nameValue.asString(), &method))
                {
                    if (argCount != method->arity)
                    {
                        runtimeError("Method '%s' expects %d arguments, got %d", name, method->arity, argCount);
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    //  Debug::dumpFunction(method);
                    fiber->stackTop[-argCount - 1] = receiver;

                    // Setup call frame
                    if (currentFiber->frameCount >= FRAMES_MAX)
                    {
                        runtimeError("Stack overflow in method!");
                        return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                    }

                    CallFrame *newFrame = &currentFiber->frames[currentFiber->frameCount];
                    newFrame->func = method;
                    newFrame->ip = method->chunk->code;
                    newFrame->slots = currentFiber->stackTop - argCount -1;

                    currentFiber->frameCount++;

                    STORE_FRAME();
                    LOAD_FRAME();

                    break;
                }
                runtimeError("Instance '%s' has no method '%s'", instance->klass->name->chars(), name);
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            if (receiver.isNativeClassInstance())
            {
                printValueNl(receiver);
                printValueNl(nameValue);

                NativeInstance *instance = receiver.asNativeClassInstance();
                NativeClassDef *klass = instance->klass;

                NativeMethod method;
                if (!instance->klass->methods.get(nameValue.asString(), &method))
                {
                    runtimeError("Native class '%s' has no method '%s'", klass->name->chars(), name);
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }

                Value *args = fiber->stackTop - argCount;
                Value result = method(this, instance->userData, argCount, args);
                fiber->stackTop -= (argCount + 1);
                PUSH(result);

                break;
            }

            runtimeError("Type does not support method calls");

            printf(": ");
            printValue(receiver);
            printf("\n");

            return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
        }

        case OP_SUPER_INVOKE:
        {
            uint8_t nameIdx = READ_BYTE();
            uint8_t argCount = READ_BYTE();

            Value nameValue = func->chunk->constants[nameIdx];
            String *methodName = nameValue.asString();

            Value self = NPEEK(argCount); // Pega self ANTES dos args

            // printf("Super member: ");
            // printValueNl(nameValue);
            // printf(" Super class: ");
            // printValueNl(self);
            // printf(" args %d \n",argCount);

            if (!self.isClassInstance())
            {
                runtimeError("'super' requires an instance");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            ClassInstance *instance = self.asClassInstance();

            if (!instance->klass->superclass)
            {
                runtimeError("Class has no superclass");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            // Procura método no SUPERCLASS )
            Function *method;
            if (!instance->klass->superclass->methods.get(methodName, &method))
            {
                runtimeError("Undefined method '%s' in superclass", methodName->chars());
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            if (argCount != method->arity)
            {
                runtimeError("Method '%s' expects %d arguments, got %d",
                             methodName->chars(), method->arity, argCount);
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }
            // Setup call frame
            if (fiber->frameCount >= FRAMES_MAX)
            {
                runtimeError("Stack overflow");
                return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            }

            CallFrame *newFrame = &fiber->frames[fiber->frameCount];
            newFrame->func = method;
            newFrame->ip = method->chunk->code;
            newFrame->slots = fiber->stackTop - argCount -1; //  Aponta para self

            fiber->frameCount++;

            STORE_FRAME();
            LOAD_FRAME();
            break;
        }

        case OP_GOSUB:
        {
            int16 off = (int16)READ_SHORT(); // lê u16 mas cast para signed
            if (fiber->gosubTop >= GOSUB_MAX)
                runtimeError("gosub stack overflow");
            fiber->gosubStack[fiber->gosubTop++] = ip; // retorno
            ip += off;                                 // forward/back
            break;
        }

        case OP_RETURN_SUB:
        {
            if (fiber->gosubTop > 0)
            {
                ip = fiber->gosubStack[--fiber->gosubTop];
                break;
            }
            return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
        }
        case OP_DEFINE_ARRAY:
        {
            uint8_t count = READ_BYTE();
            Value array = Value::makeArray();
            ArrayInstance *instance = array.asArray();
            arrayInstances.push(instance);
            instance->values.resize(count);
            for (int i = count - 1; i >= 0; i--)
            {
                instance->values[i] = POP();
            }
            PUSH(array);
            break;
        }
        case OP_DEFINE_MAP:
        {
            uint8_t count = READ_BYTE();

            Value map = Value::makeMap();
            MapInstance *inst = map.asMap();

            for (int i = 0; i < count; i++)
            {
                Value value = POP();
                Value key = POP();

                if (!key.isString())
                {
                    runtimeError("Map key must be string");
                    PUSH(Value::makeNil());
                    break;
                }

                inst->table.set(key.asString(), value);
            }

            PUSH(map);
            break;
        }
        case OP_SET_INDEX:
        {
            Value value = POP();
            Value index = POP();
            Value container = POP();

            // printValue(value);
            // printf(" value \n");
            // printValue(index);
            // printf(" index \n");
            // printValue(container);
            // printf(" container \n");

            if (container.isArray())
            {
                if (!index.isInt())
                {
                    runtimeError("Array index must be integer");
                    PUSH(value);
                    break;
                }

                ArrayInstance *arr = container.asArray();
                int i = index.asInt();
                uint32 size = arr->values.size();

                // Negative index
                if (i < 0)
                    i += size;

                if (i < 0 || i >= size)
                {
                    runtimeError("Array index %d out of bounds (size=%d)", i, size);
                }
                else
                {
                    arr->values[i] = value;
                }

                PUSH(value); // Assignment returns value
                break;
            }

            // === MAP  ===
            if (container.isMap())
            {
                if (!index.isString())
                {
                    runtimeError("Map key must be string");
                    PUSH(value);
                    break;
                }

                MapInstance *map = container.asMap();
                map->table.set(index.asString(), value);

                PUSH(value); // Assignment returns value
                break;
            }

            // STRING é imutável!
            if (container.isString())
            {
                runtimeError("Strings are immutable");
                PUSH(value);
                break;
            }

            runtimeError("Cannot index assign this type");
            PUSH(value);
            return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
            break;
        }
        case OP_GET_INDEX:
        {
            Value index = POP();
            Value container = POP();

            // printValue(index);
            // printf("\n");
            // printValue(container);
            // printf("\n");

            // === ARRAY ===
            if (container.isArray())
            {
                if (!index.isInt())
                {
                    runtimeError("Array index must be integer");
                    PUSH(Value::makeNil());
                    break;
                }

                ArrayInstance *arr = container.asArray();
                int i = index.asInt();
                uint32 size = arr->values.size();

                //  Negative index (Python-style)
                if (i < 0)
                    i += size;

                if (i < 0 || i >= size)
                {
                    runtimeError("Array index %d out of bounds (size=%d)", i, size);
                    PUSH(Value::makeNil());
                }
                else
                {
                    PUSH(arr->values[i]);
                }
                break;
            }

            // === STRING ===
            if (container.isString())
            {
                if (!index.isInt())
                {
                    runtimeError("String index must be integer");
                    PUSH(Value::makeNil());
                    return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
                }

                String *str = container.asString();
                String *result = StringPool::instance().at(str, index.asInt());
                PUSH(Value::makeString(result));
                break;
            }

            // === MAP   ===
            if (container.isMap())
            {
                if (!index.isString())
                {
                    runtimeError("Map key must be string");
                    PUSH(Value::makeNil());
                    break;
                }

                MapInstance *map = container.asMap();
                Value result;

                if (map->table.get(index.asString(), &result))
                {
                    PUSH(result);
                }
                else
                {
                    // Key não existe - retorna nil
                    PUSH(Value::makeNil());
                }
                break;
            }

            runtimeError("Cannot index this type");
            PUSH(Value::makeNil());
            return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
        }
        default:
        {
            runtimeError("Unknown opcode %d", instruction);
            return {FiberResult::FIBER_DONE, instructionsRun, 0, 0};
        }
        }
    }

    // Cleanup macros

#undef READ_BYTE
#undef READ_SHORT
}
