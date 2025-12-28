#pragma once
#include "config.hpp"
#include "map.hpp"
#include "vector.hpp"
#include "string.hpp"
#include "arena.hpp"
#include "code.hpp"
#include "types.hpp"
#include "pool.hpp"

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

struct Function;
struct CallFrame;
struct Fiber;
struct Process;
class Interpreter;
class Compiler;

struct IntEq
{
    bool operator()(int a, int b) const { return a == b; }
};

struct StringEq
{
    bool operator()(String *a, String *b) const;
};

struct StringHasher
{
    size_t operator()(String *x) const;
};

enum class FieldType : uint8_t
{
    BYTE,    // byte
    INT,     // int
    UINT,    // uint
    FLOAT,   // float
    DOUBLE,  // double
    BOOL,    // bool
    POINTER, // void*
    STRING,  // String*
};

typedef Value (*NativeFunction)(Interpreter *vm, int argCount, Value *args);
typedef Value (*NativeMethod)(Interpreter *vm, void *instance, int argCount, Value *args);
typedef void *(*NativeConstructor)(Interpreter *vm, int argCount, Value *args);
typedef void (*NativeDestructor)(Interpreter *vm, void *instance);
typedef Value (*NativeGetter)(Interpreter *vm, void *instance);
typedef void (*NativeSetter)(Interpreter *vm, void *instance, Value value);
typedef void (*NativeStructCtor)(Interpreter *vm, void *buffer, int argc, Value *args);
typedef void (*NativeStructDtor)(Interpreter *vm, void *buffer);

struct SymbolTable
{
    String *name;
    HashMap<String *, Value, StringHasher, StringEq> symbols;
};

struct NativeProperty
{
    NativeGetter getter;
    NativeSetter setter; // null = read-only
};

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

struct StructDef
{
    String *name;
    HashMap<String *, uint8, StringHasher, StringEq> names;
    uint8 argCount;
};

struct ClassDef
{
    String *name{nullptr};
    String *parent{nullptr};
    bool inherited{false};
    int fieldCount;                 // Número de fields
    Function *constructor{nullptr}; // existe na tabela
    ClassDef *superclass;           // 1 nível herança

    HashMap<String *, Function *, StringHasher, StringEq> methods;
    HashMap<String *, uint8_t, StringHasher, StringEq> fieldNames; // field name → index
    Function *canRegisterFunction(const char *name);
    ~ClassDef();
};

struct NativeClassDef
{
    String *name;

    NativeConstructor constructor;
    NativeDestructor destructor;

    HashMap<String *, NativeMethod, StringHasher, StringEq> methods;
    HashMap<String *, NativeProperty, StringHasher, StringEq> properties;

    ~NativeClassDef();

    int argCount; // Args do constructor
    int id;
};

struct NativeFieldDef
{
    size_t offset;
    FieldType type;
    bool readOnly;
};

struct NativeStructDef
{
    int id;
    String *name;
    size_t structSize;
    HashMap<String *, NativeFieldDef, StringHasher, StringEq> fields;
    NativeStructCtor constructor; // nullable
    NativeStructDtor destructor;  // nullable
};

struct NativeStructInstance : public GCObject
{

    NativeStructDef *def;
    void *data; // Malloc'd block (structSize bytes)
    NativeStructInstance();
    ~NativeStructInstance();

    NativeStructInstance(const NativeStructInstance &) = delete;
    NativeStructInstance &operator=(const NativeStructInstance &) = delete;

    void release() override;
};
struct StructInstance : public GCObject
{
    StructDef *def;
    Vector<Value> values;
    StructInstance();
    ~StructInstance();
    StructInstance(const StructInstance &) = delete;
    StructInstance &operator=(const StructInstance &) = delete;

    void release() override;
};

struct ArrayInstance : public GCObject
{
    Vector<Value> values;
    ArrayInstance();

    ~ArrayInstance();

    ArrayInstance(const ArrayInstance &) = delete;
    ArrayInstance &operator=(const ArrayInstance &) = delete;

    void release() override;
};

struct NativeInstance : public GCObject
{
    NativeClassDef *klass;
    void *userData; //  Ponteiro para struct C++
    int refCount;
    NativeInstance();
    ~NativeInstance();

    NativeInstance(const NativeInstance &) = delete;
    NativeInstance &operator=(const NativeInstance &) = delete;

    void release() override;
};

struct MapInstance : public GCObject
{
    HashMap<String *, Value, StringHasher, StringEq> table;
    MapInstance();
    ~MapInstance();

    MapInstance(const MapInstance &) = delete;
    MapInstance &operator=(const MapInstance &) = delete;

    void release() override;
};

struct ClassInstance : public GCObject
{
    ClassDef *klass;

    Vector<Value> fields;
    ClassInstance();
    ~ClassInstance();

    ClassInstance(const ClassInstance &) = delete;
    ClassInstance &operator=(const ClassInstance &) = delete;

    bool getMethod(String *name, Function **func);

    void release() override;
 
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
    uint8_t *gosubStack[GOSUB_MAX];
    int gosubTop{0};

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

class Interpreter
{

    HashMap<String *, Function *, StringHasher, StringEq> functionsMap;
    HashMap<String *, ProcessDef *, StringHasher, StringEq> processesMap;
    HashMap<String *, StructDef *, StringHasher, StringEq> structsMap;
    HashMap<String *, ClassDef *, StringHasher, StringEq> classesMap;
    HashMap<const char *, int, CStringHash, CStringEq> privateIndexMap;

    Vector<Function *> functions;
    Vector<Function *> functionsClass;
    Vector<ProcessDef *> processes;
    Vector<NativeDef> natives;
    Vector<StructDef *> structs;
    Vector<ClassDef *> classes;
    Vector<Value> globalList;

    Vector<NativeClassDef *> nativeClasses;
    HashMap<String *, NativeClassDef *, StringHasher, StringEq> nativesClassesMap;
    // Vector<NativeInstance *> nativeInstances;

    Vector<NativeStructDef *> nativeStructs;
    HashMap<String *, NativeStructDef *, StringHasher, StringEq> nativeStructNames;
    // Vector<NativeStructInstance *> nativeStructInstances;
    int lastInstanceId = 0;

    HashMap<String *, Value, StringHasher, StringEq> globals;
 
    Vector<Process *> aliveProcesses;
    Vector<Process *> cleanProcesses;

    HeapAllocator heapAllocator;

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
    void checkType(int index, ValueType expected, const char *funcName);

    void addFunctionsClasses(Function *fun);

    bool getGlobal(String *name, Value *out) const;
    bool setGlobal(String *name, Value val);

    friend class Compiler;
    friend class InstancePool;

    String *staticFree;
    String *staticToString;

public:
    Interpreter();
    ~Interpreter();
    void update(float deltaTime);

    int getProcessPrivateIndex(const char *name);

    uint32 liveProcess();

    void gc();
    uint32 getTotalStrings();
    uint32 getStringsBytes();
    uint32 getTotalBytes();

    uint32 getTotalClasses();
    uint32 getTotalStructs();
    uint32 getTotalFunctions();
    uint32 getTotalProcesses();
    uint32 getTotalFibers();
    uint32 getTotalArrays();
    uint32 getTotalMaps();
    uint32 getTotalNativeStructs();
    uint32 getTotalNativeClasses();
    

    bool addModule(const char *name);

    void setFileLoader(FileLoaderCallback loader, void *userdata = nullptr);

    bool importModule(const char *name);

    int registerNative(const char *name, NativeFunction func, int arity, const char *moduleName = nullptr);

    NativeClassDef *registerNativeClass(
        const char *name,
        NativeConstructor ctor,
        NativeDestructor dtor,
        int argCount, const char *moduleName = nullptr);
    void addNativeMethod(
        NativeClassDef *klass,
        const char *methodName,
        NativeMethod method);
    void addNativeProperty(
        NativeClassDef *klass,
        const char *propName,
        NativeGetter getter,
        NativeSetter setter = nullptr // null = read-only
    );

    NativeStructDef *registerNativeStruct(const char *name,
                                          size_t structSize,
                                          NativeStructCtor ctor = nullptr,
                                          NativeStructDtor dtor = nullptr,
                                          const char *moduleName = nullptr);

    void addStructField(
        NativeStructDef *def,
        const char *fieldName,
        size_t offset,
        FieldType type,
        bool readOnly = false);

    int getLastRegisteredInstanceId() const { return lastInstanceId; }

    ProcessDef *addProcess(const char *name, Function *func);
    void destroyProcess(Process *proc);
    Process *spawnProcess(ProcessDef *proc);

    StructDef *addStruct(String *nam, int *id);

    StructDef *registerStruct(String *nam, int *id);
    ClassDef *registerClass(String *nam, int *id);

    bool containsClassDefenition(String *name);
    bool getClassDefenition(String *name, ClassDef *result);
    bool tryGetClassDefenition(const char *name, ClassDef **result);

    uint32 getTotalProcesses() const;
    uint32 getTotalAliveProcesses() const;

    void destroyFunction(Function *func);
    void addFiber(Process *proc, Function *func);

    String *internString(const char *str);

    void print(Value value);

    Function *addFunction(const char *name, int arity = 0);
    Function *canRegisterFunction(const char *name, int arity, int *index);
    bool functionExists(const char *name);
    int registerFunction(const char *name, Function *func);

    void run_process_step(Process *proc);
    FiberResult run_fiber(Fiber *fiber);

    float getCurrentTime() const;

    void runtimeError(const char *format, ...);

    bool callFunction(Function *func, int argCount);
    bool callFunction(const char *name, int argCount);

    Process *callProcess(ProcessDef *proc, int argCount);
    Process *callProcess(const char *name, int argCount);

    Function *compile(const char *source);
    Function *compileExpression(const char *source);
    bool run(const char *source, bool dump = false);

    void reset();

    void setHooks(const VMHooks &h);

    void render();

    void disassemble();

    Value createNativeStruct(int structId, int argc, Value *args);

    // int addGlobal(const char *name, Value value, const char *module = nullptr);
    // String *addGlobalEx(const char *name, Value value, const char *module = nullptr);
    // Value getGlobal(const char *name, const char *module = nullptr);
    // Value getGlobal(uint32 index, const char *module = nullptr);
    // bool tryGetGlobal(const char *name, Value *value, const char *module = nullptr);

    // ===== STACK API   =====
    const Value &peek(int index); // -1 = topo, 0 = base
    void push(Value value);
    Value pop();

    // ===== PUSH HELPERS =====

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
    bool checkStack(int extra);

    // ===== STACK MANIPULATION =====
    void insert(int index);      // Insere topo no index
    void remove(int index);      // Remove index
    void replace(int index);     // Substitui index pelo topo
    void copy(int from, int to); // Copia from → to
    void rotate(int idx, int n); // Roda n elementos

    // ===== TYPE CHECKING =====
    ValueType getType(int index);
    bool isInt(int index);
    bool isDouble(int index);
    bool isString(int index);
    bool isBool(int index);
    bool isFunction(int index);
    bool isNil(int index);
};