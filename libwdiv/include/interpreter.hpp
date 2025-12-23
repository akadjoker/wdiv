#pragma once
#include "config.hpp"
#include "map.hpp"
#include "vector.hpp"
#include "string.hpp"
#include "arena.hpp"
#include "code.hpp"

static constexpr int MAX_PRIVATES = 16;
static constexpr int MAX_FIBERS = 8;
static constexpr int STACK_MAX = 256;
static constexpr int FRAMES_MAX = 32;

enum class InterpretResult : uint8
{
    OK,
    COMPILE_ERROR,
    RUNTIME_ERROR
};

enum class FiberState : uint8
{
    RUNNING,
    SUSPENDED,
    DEAD
};

struct Function;
struct CallFrame;
struct Fiber;
struct Process;
class Interpreter;
class Compiler;
typedef Value (*NativeFunction)(Interpreter *vm, int argCount, Value *args);

struct NativeDef
{
    String *name{nullptr};
    NativeFunction func;
    int arity{0};
    uint32 index{0};
};

struct Function
{
    int arity{-1};
    Code *chunk{nullptr};
    String *name{nullptr};
    bool hasReturn{false};
    ~Function();
};

struct CallFrame
{
    Function *func{nullptr};
    uint8 *ip{nullptr};
    Value *slots{nullptr};
};

struct VMHooks
{
    void (*onStart)(Process *p) = nullptr;
    void (*onUpdate)(Process *p, float dt) = nullptr;
    void (*onRender)(Process *p) = nullptr;
    void (*onDestroy)(Process *p, int exitCode) = nullptr;
};

struct FiberResult
{
    enum Reason : uint8
    {
        FIBER_YIELD,   // yield N
        PROCESS_FRAME, // frame(N)
        FIBER_DONE,    // return/end
        ERROR
    };

    Reason reason;
    int instructionsRun;
    float yieldMs;    // Se FIBER_YIELD
    int framePercent; // Se PROCESS_FRAME
};

struct Fiber
{

    FiberState state; // Estado da FIBER (yield)
    float resumeTime; // Quando acorda (yield)

    uint8 *ip;
    Value stack[STACK_MAX];
    Value *stackTop;
    CallFrame frames[FRAMES_MAX];
    int frameCount;

    Fiber()
        : state(FiberState::DEAD), resumeTime(0), ip(nullptr), stackTop(stack), frameCount(0)
    {
    }
};
enum class PrivateIndex : uint8
{
    X = 0,
    Y = 1,
    Z = 2,
    GRAPH = 3,
    ANGLE = 4,
    SIZE = 5,
    FLAGS = 6,
    ID = 7,
    FATHER = 8,

};

struct ProcessDef
{
    Vector<uint8> argsNames;
    String *name{nullptr};
    Fiber fibers[MAX_FIBERS];
    Fiber *current;
    Value privates[MAX_PRIVATES];
    int nextFiberIndex;
    void finalize();
    void release();
};

struct Process
{

    String *name{nullptr};
    uint32 id;

    FiberState state;        //  Estado do PROCESSO (frame)
    float resumeTime = 0.0f; // Quando acorda (frame)

    Fiber fibers[MAX_FIBERS];
    int nextFiberIndex;
    int currentFiberIndex;
    Fiber *current;

    Value privates[MAX_PRIVATES];

    int exitCode = 0;

    bool initialized = false;

    void release();
    void finalize();
};

struct IntEq
{
    bool operator()(int a, int b) const { return a == b; }
};

struct StringEq
{
    bool operator()(String *a, String *b) const
    {
        if (a == b)
            return true;
        if (a->length() != b->length())
            return false;
        return memcmp(a->chars(), b->chars(), a->length()) == 0;
    }
};

struct StringHasher
{
    size_t operator()(String *x) const { return x->hash; }
};

struct CStringHash
{
    size_t operator()(const char *str) const
    {
        // FNV-1a hash
        size_t hash = 2166136261u;
        while (*str)
        {
            hash ^= (unsigned char)*str++;
            hash *= 16777619u;
        }
        return hash;
    }
};

// Eq para const char*
struct CStringEq
{
    bool operator()(const char *a, const char *b) const
    {
        return strcmp(a, b) == 0;
    }
};

class Interpreter
{

    HashMap<String *, Function *, StringHasher, StringEq> functionsMap;
    HashMap<String *, ProcessDef *, StringHasher, StringEq> processesMap;
    HashMap<String *, NativeDef, StringHasher, StringEq> nativesMap;
    HashMap<const char*, int, CStringHash, CStringEq> privateIndexMap;

    Vector<Function *> functions;
    Vector<ProcessDef *> processes;
    Vector<NativeDef> natives;
    Vector<Value> globalList;

    HashMap<String *, Value, StringHasher, StringEq> globals;

    HeapAllocator arena;

    Vector<Process *> aliveProcesses;
    Vector<Process *> cleanProcesses;

    float currentTime;
    float lastFrameTime;
    float accumulator = 0.0f;
    const float FIXED_DT = 1.0f / 60.0f;

    Fiber *currentFiber;
    Process *currentProcess;
    Process *mainProcess;
    bool hasFatalError_;

    bool isTruthy(const Value &value);
    bool isFalsey(Value value);

    Compiler *compiler;

    VMHooks hooks;

    // const Value &peek(int distance = 0);

    Fiber *get_ready_fiber(Process *proc);
    void resetFiber();
    void initFiber(Fiber *fiber, Function *func);
    void setPrivateTable();
public:
    Interpreter();
    ~Interpreter();
    void update(float deltaTime);

    int getProcessPrivateIndex(const char *name);

    uint32 liveProcess();

    ProcessDef *addProcess(const char *name, Function *func);
    void destroyProcess(Process *proc);
    Process *spawnProcess(ProcessDef *proc);

    uint32 getTotalProcesses() const;
    uint32 getTotalAliveProcesses() const;

    void destroyFunction(Function *func);
    void addFiber(Process *proc, Function *func);

    int registerNative(const char *name, NativeFunction func, int arity);

    void print(Value value);

    Function *addFunction(const char *name, int arity = 0);
    Function *canRegisterFunction(const char *name, int arity, int *index);
    bool functionExists(const char *name);
    int registerFunction(const char *name, Function *func);

    void run_process_step(Process *proc);
    FiberResult run_fiber(Fiber *fiber);

    float getCurrentTime() const;

    void runtimeError(const char *format, ...);

    bool callValue(Value callee, int argCount);

    Function *compile(const char *source);
    Function *compileExpression(const char *source);
    bool run(const char *source, bool dump = false);

    void reset();

    void setHooks(const VMHooks &h);

    void render();

    void disassemble();

    int addGlobal(const char *name, Value value);
    String *addGlobalEx(const char *name, Value value);
    Value getGlobal(const char *name);
    Value getGlobal(uint32 index);

    // ===== STACK API   =====
    const Value &peek(int index); // -1 = topo, 0 = base
    void push(Value value);
    Value pop();

    void pushInt(int n);
    void pushDouble(double d);
    void pushString(const char *s);
    void pushBool(bool b);
    void pushNil();

    int toInt(int index);
    double toDouble(int index);
    const char *toString(int index);
    bool toBool(int index);

    // ===== STACK INFO =====
    int getTop();
    void setTop(int index);

    // ===== TYPE CHECKING =====
    ValueType getType(int index);
    bool isInt(int index);
    bool isDouble(int index);
    bool isString(int index);
    bool isBool(int index);
    bool isFunction(int index);
    bool isNil(int index);
};