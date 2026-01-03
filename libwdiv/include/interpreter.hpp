#pragma once
#include "arena.hpp"
#include "code.hpp"
#include "config.hpp"
#include "map.hpp"
#include "pool.hpp"
#include "string.hpp"
#include "types.hpp"
#include "vector.hpp"

#ifdef NDEBUG
#define WDIV_ASSERT(condition, ...) ((void)0)
#else
#define WDIV_ASSERT(condition, ...)                                      \
  do                                                                     \
  {                                                                      \
    if (!(condition))                                                    \
    {                                                                    \
      Error("ASSERT FAILED: %s:%d: %s", __FILE__, __LINE__, #condition); \
      assert(false);                                                     \
    }                                                                    \
  } while (0)
#endif

struct Function;
struct CallFrame;
struct Fiber;
struct Process;
class Interpreter;
class Compiler;

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
typedef Value (*NativeMethod)(Interpreter *vm, void *instance, int argCount,
                              Value *args);
typedef void *(*NativeConstructor)(Interpreter *vm, int argCount, Value *args);
typedef void (*NativeDestructor)(Interpreter *vm, void *instance);
typedef Value (*NativeGetter)(Interpreter *vm, void *instance);
typedef void (*NativeSetter)(Interpreter *vm, void *instance, Value value);
typedef void (*NativeStructCtor)(Interpreter *vm, void *buffer, int argc,
                                 Value *args);
typedef void (*NativeStructDtor)(Interpreter *vm, void *buffer);

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
  int index;
  int arity{-1};
  Code *chunk{nullptr};
  String *name{nullptr};
  bool hasReturn{false};
  ~Function();
};

struct StructDef
{
  int index;
  String *name;
  HashMap<String *, uint8, StringHasher, StringEq> names;
  uint8 argCount;
};

struct ClassDef
{
  int index;
  String *name{nullptr};
  String *parent{nullptr};
  bool inherited{false};
  int fieldCount;                 // Número de fields
  Function *constructor{nullptr}; // existe na tabela
  ClassDef *superclass;           // 1 nível herança

  HashMap<String *, Function *, StringHasher, StringEq> methods;
  HashMap<String *, uint8_t, StringHasher, StringEq>
      fieldNames; // field name → index
  Function *canRegisterFunction(const char *name);
  ~ClassDef();
};

struct NativeClassDef
{
  int index;
  String *name;
  NativeConstructor constructor;
  NativeDestructor destructor;

  HashMap<String *, NativeMethod, StringHasher, StringEq> methods;
  HashMap<String *, NativeProperty, StringHasher, StringEq> properties;

  ~NativeClassDef();

  int argCount; // Args do constructor
};

struct NativeInstance
{
  NativeClassDef *klass;
  void *userData; //  Ponteiro para struct C++
  int refCount;
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

struct NativeStructInstance
{
  NativeStructDef *def;
  void *data; // Malloc'd block (structSize bytes)
};

struct NativeFunctionDef
{
  NativeFunction ptr;
  int arity;
};

class ModuleDef
{
private:
  String *name;
  Vector<NativeFunctionDef> functions;
  HashMap<String *, uint16, StringHasher, StringEq> functionNames;
  Vector<Value> constants;
  HashMap<String *, uint16, StringHasher, StringEq> constantNames;
  friend class Interpreter;
public:
  ModuleDef(String *name);
  uint16 addFunction(const char *name, NativeFunction func, int arity);
  uint16 addConstant(const char *name, Value value);
  NativeFunctionDef *getFunction(uint16 id);
  Value *getConstant(uint16 id);

  bool getFunctionId(String *name, uint16 *outId);
  bool getConstantId(String *name, uint16 *outId);

  bool getFunctionName(uint16 id, String **outName);
  bool getConstantName(uint16 id, String **outName);

  String *getName() const { return name; }
};

class ModuleBuilder
{
private:
  ModuleDef *module;

public:
  ModuleBuilder(ModuleDef *module);
  ModuleBuilder &addFunction(const char *name, NativeFunction func, int arity);
  ModuleBuilder &addInt(const char *name, int value);
  ModuleBuilder &addFloat(const char *name, float value);
  ModuleBuilder &addDouble(const char *name, double value);
  ModuleBuilder &addBool(const char *name, bool value);
  ModuleBuilder &addString(const char *name, const char *value);
};

struct GCObject
{
  int index;
  GCObject();
  void grab();
  void release();
};

struct StructInstance : public GCObject
{
  StructDef *def;
  Vector<Value> values;
  StructInstance();
  ~StructInstance();
};

struct ArrayInstance : public GCObject
{
  Vector<Value> values;
  ArrayInstance();
  ~ArrayInstance();
};

struct MapInstance : public GCObject
{
  HashMap<String *, Value, StringHasher, StringEq> table;
  MapInstance();
  ~MapInstance();
};

struct ClassInstance : public GCObject
{
  ClassDef *klass;

  Vector<Value> fields;
  ClassInstance();
  ~ClassInstance();

  bool getMethod(String *name, Function **func);
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
      : state(FiberState::DEAD), resumeTime(0), ip(nullptr), stackTop(stack),
        frameCount(0) {}
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
  int index;
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
  HashMap<String *, NativeDef, StringHasher, StringEq> nativesMap;
  HashMap<String *, StructDef *, StringHasher, StringEq> structsMap;
  HashMap<String *, ClassDef *, StringHasher, StringEq> classesMap;
  HashMap<const char *, int, CStringHash, CStringEq> privateIndexMap;

  Vector<ModuleDef *> modules;                                   // Array de módulos!
  HashMap<String *, uint16, StringHasher, StringEq> moduleNames; // Nome  ID

  Vector<Function *> functions;
  Vector<Function *> functionsClass;
  Vector<ProcessDef *> processes;
  Vector<NativeDef> natives;
  Vector<StructDef *> structs;
  Vector<ClassDef *> classes;

  Vector<StructInstance *> structInstances;
  Vector<ArrayInstance *> arrayInstances;

  Vector<NativeClassDef *> nativeClasses;
  Vector<NativeInstance *> nativeInstances;

  Vector<NativeStructDef *> nativeStructs;
  Vector<NativeStructInstance *> nativeStructInstances;

  HashMap<String *, Value, StringHasher, StringEq> globals;

  Vector<Process *> aliveProcesses;
  Vector<Process *> cleanProcesses;

  HeapAllocator heapAllocator;
  bool asEnded = false;

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

  friend class Compiler;

  void dumpAllFunctions(FILE *f);
  void dumpAllClasses(FILE *f);

public:
  Interpreter();
  ~Interpreter();
  void update(float deltaTime);

  int getProcessPrivateIndex(const char *name);

  void dumpToFile(const char *filename);
  uint32 liveProcess();

  void setFileLoader(FileLoaderCallback loader, void *userdata = nullptr);

  NativeClassDef *registerNativeClass(const char *name, NativeConstructor ctor,
                                      NativeDestructor dtor, int argCount);
  void addNativeMethod(NativeClassDef *klass, const char *methodName,
                       NativeMethod method);
  void addNativeProperty(NativeClassDef *klass, const char *propName,
                         NativeGetter getter,
                         NativeSetter setter = nullptr // null = read-only
  );

  Value createNativeStruct(int structId, int argc, Value *args);
  NativeStructDef *registerNativeStruct(const char *name, size_t structSize,
                                        NativeStructCtor ctor = nullptr,
                                        NativeStructDtor dtor = nullptr);

  void addStructField(NativeStructDef *def, const char *fieldName,
                      size_t offset, FieldType type, bool readOnly = false);

  ProcessDef *addProcess(const char *name, Function *func);
  void destroyProcess(Process *proc);
  Process *spawnProcess(ProcessDef *proc);

  StructDef *addStruct(String *nam, int *id);

  StructDef *registerStruct(String *name);
  ClassDef *registerClass(String *nam);

  bool containsClassDefenition(String *name);
  bool getClassDefenition(String *name, ClassDef *result);
  bool tryGetClassDefenition(const char *name, ClassDef **result);

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

  bool callFunction(Function *func, int argCount);
  bool callFunction(const char *name, int argCount);

  Process *callProcess(ProcessDef *proc, int argCount);
  Process *callProcess(const char *name, int argCount);

  Function *compile(const char *source);
  Function *compileExpression(const char *source);
  bool run(const char *source, bool dump = false);
  bool compile(const char *source, bool dump);

  void reset();

  void setHooks(const VMHooks &h);

  void render();

  uint16 defineModule(const char *name);
  ModuleBuilder addModule(const char *name);
  ModuleDef *getModule(uint16 id);
  bool getModuleId(String *name, uint16 *outId);

  void disassemble();

  int addGlobal(const char *name, Value value);
  String *addGlobalEx(const char *name, Value value);
  Value getGlobal(const char *name);
  Value getGlobal(uint32 index);
  bool tryGetGlobal(const char *name, Value *value);

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